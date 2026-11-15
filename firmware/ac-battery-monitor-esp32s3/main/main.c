#include "battery.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

enum { MAX_FRAME = 512, READ_INTERVAL_MS = 5000, LINKS = 2 };
static const char *TAG = "ac_battery";
static const ble_uuid128_t service_id = BLE_UUID128_INIT(
    0xce, 0x4a, 0x86, 0x91, 0x43, 0xe8, 0xe6, 0xa8, 0x29, 0x45, 0x7e, 0x4f, 0x3d, 0x44, 0x09, 0xcb);
static const ble_uuid128_t identity_id = BLE_UUID128_INIT(
    0x24, 0xe6, 0x22, 0xc9, 0x99, 0x91, 0xcc, 0x80, 0xc7, 0x4e, 0x6c, 0xc3, 0xe3, 0x38, 0x1a, 0xa8);
static const ble_uuid128_t sample_id = BLE_UUID128_INIT(
    0xd9, 0xb0, 0x46, 0x48, 0xfe, 0xbc, 0x1f, 0x89, 0x8a, 0x42, 0x31, 0x38, 0x1e, 0x64, 0x86, 0x96);

static uint8_t identity_frame[MAX_FRAME];
static uint8_t sample_frame[VHOS_HEADER_SIZE + BATTERY_PAYLOAD_SIZE];
static size_t identity_length;
static bool sample_available;
static uint8_t address_type;
static unsigned connected;
static portMUX_TYPE sample_lock = portMUX_INITIALIZER_UNLOCKED;

static int read_attribute(uint16_t conn, uint16_t handle, struct ble_gatt_access_ctxt *context, void *arg)
{
    struct ble_gap_conn_desc peer;
    if (ble_gap_conn_find(conn, &peer) != 0 || !peer.sec_state.encrypted) {
        return BLE_ATT_ERR_INSUFFICIENT_ENC;
    }
    if (ble_uuid_cmp(context->chr->uuid, &identity_id.u) == 0) {
        return os_mbuf_append(context->om, identity_frame, identity_length) == 0
            ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    uint8_t snapshot[sizeof(sample_frame)];
    portENTER_CRITICAL(&sample_lock);
    const bool available = sample_available;
    memcpy(snapshot, sample_frame, sizeof(snapshot));
    portEXIT_CRITICAL(&sample_lock);
    if (!available) return BLE_ATT_ERR_UNLIKELY;
    return os_mbuf_append(context->om, snapshot, sizeof(snapshot)) == 0
        ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const struct ble_gatt_svc_def services[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &service_id.u, .characteristics = (struct ble_gatt_chr_def[]) {
        {.uuid = &identity_id.u, .access_cb = read_attribute,
         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC},
        {.uuid = &sample_id.u, .access_cb = read_attribute,
         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC},
        {0},
    }},
    {0},
};

static void advertise(void);

static int gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ++connected;
        }
        advertise();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        if (connected) --connected;
        advertise();
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        return BLE_GAP_REPEAT_PAIRING_IGNORE;
    default:
        break;
    }
    return 0;
}

static void advertise(void)
{
    if (connected >= LINKS) return;
    const struct ble_gap_adv_params parameters = {
        .conn_mode = BLE_GAP_CONN_MODE_UND, .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    const int result = ble_gap_adv_start(address_type, NULL, BLE_HS_FOREVER, &parameters, gap_event, NULL);
    if (result != 0 && result != BLE_HS_EALREADY) ESP_LOGE(TAG, "BLE advertising failed: %d", result);
}

static void ble_ready(void)
{
    if (ble_hs_util_ensure_addr(0) || ble_hs_id_infer_auto(0, &address_type)) {
        ESP_LOGE(TAG, "BLE address unavailable");
        return;
    }
    connected = 0;
    const struct ble_hs_adv_fields fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .uuids128 = (ble_uuid128_t *)&service_id, .num_uuids128 = 1, .uuids128_is_complete = 1,
    };
    if (ble_gap_adv_set_fields(&fields) == 0) advertise();
}

static void ble_task(void *arg)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void sample_task(void *arg)
{
    uint64_t sequence = 1;
    while (true) {
        const battery_snapshot_t sample = battery_poll();
        uint8_t payload[BATTERY_PAYLOAD_SIZE], frame[sizeof(sample_frame)];
        battery_payload(&sample, payload);
        const uint64_t time_us = (uint64_t)esp_timer_get_time();
        battery_frame(VHOS_AC_BATTERY, ++sequence, time_us, payload, sizeof(payload), frame, sizeof(frame));
        portENTER_CRITICAL(&sample_lock);
        memcpy(sample_frame, frame, sizeof(frame));
        sample_available = true;
        portEXIT_CRITICAL(&sample_lock);
        ESP_LOGI(TAG, "sample=%" PRIu64 " flags=%u result=%u raw_v=%u raw_i=%d raw_soc=%u",
                 sequence, sample.flags, sample.result, sample.voltage, (int16_t)sample.current,
                 sample.state_of_charge);
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

void ble_store_config_init(void);

void app_main(void)
{
    uint8_t mac[6];
    uint32_t boot[2];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    esp_fill_random(boot, sizeof(boot));
    char identity[320];
    const int size = snprintf(identity, sizeof(identity),
        "{\"contract\":\"sensor.node.battery.handshake\",\"contract_version\":\"1.0.0\","
        "\"role\":\"AC_SENSOR\",\"source_id\":\"ac-%02x%02x%02x%02x%02x%02x\","
        "\"hardware_revision\":\"AC_R8\",\"firmware_build_id\":\"" VHOS_BUILD_ID "\","
        "\"protocol_version\":\"1.0\",\"capabilities\":[\"battery.max17260\",\"battery.pack_ntc\"],"
        "\"boot_id\":\"%08" PRIx32 "%08" PRIx32 "\"}",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], boot[0], boot[1]);
    if (size <= 0 || (size_t)size >= sizeof(identity)) return;
    identity_length = battery_frame(VHOS_HANDSHAKE, 1, (uint64_t)esp_timer_get_time(), (const uint8_t *)identity,
                                   size, identity_frame, sizeof(identity_frame));

    const esp_err_t bus = battery_bus_start();
    if (bus != ESP_OK) ESP_LOGE(TAG, "Battery I2C setup failed: %s", esp_err_to_name(bus));

    // A failed NVS initialization leaves recovery data intact.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nimble_port_init());
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_hs_cfg.sync_cb = ble_ready;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_store_config_init();
    ESP_ERROR_CHECK(ble_gatts_count_cfg(services));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(services));
    if (xTaskCreate(sample_task, "pack", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not start battery measurement");
        return;
    }
    nimble_port_freertos_init(ble_task);
}
