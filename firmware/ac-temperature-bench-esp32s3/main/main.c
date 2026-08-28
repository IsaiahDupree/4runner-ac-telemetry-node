#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "temperature_channels.h"

#ifndef VHOS_BUILD_ID
#define VHOS_BUILD_ID "unknown"
#endif

#define VHOS_FIRMWARE_VERSION "0.2.0-dev.1"
#define VHOS_BOOTSTRAP_CONTRACT "sensor.node.bootstrap"
#define VHOS_BOOTSTRAP_CONTRACT_VERSION "0.2.0"
#define VHOS_EXPECTED_FLASH_BYTES (16U * 1024U * 1024U)
#define VHOS_EXPECTED_PSRAM_BYTES (8U * 1024U * 1024U)

typedef struct {
    char device_id[32];
    uint32_t flash_bytes;
    size_t psram_bytes;
    bool nvs_ready;
    bool flash_ready;
    bool psram_ready;
    bool running_partition_ready;
    bool hardware_self_test_passed;
    const esp_partition_t *running_partition;
    esp_ota_img_states_t ota_state;
    bool ota_state_available;
} platform_status_t;

static const char *bool_json(bool value)
{
    return value ? "true" : "false";
}

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_POWERON:
        return "POWER_ON";
    case ESP_RST_EXT:
        return "EXTERNAL_PIN";
    case ESP_RST_SW:
        return "SOFTWARE";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INTERRUPT_WATCHDOG";
    case ESP_RST_TASK_WDT:
        return "TASK_WATCHDOG";
    case ESP_RST_WDT:
        return "OTHER_WATCHDOG";
    case ESP_RST_DEEPSLEEP:
        return "DEEP_SLEEP";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_SDIO:
        return "SDIO";
    case ESP_RST_USB:
        return "USB";
    case ESP_RST_JTAG:
        return "JTAG";
    case ESP_RST_EFUSE:
        return "EFUSE";
    case ESP_RST_PWR_GLITCH:
        return "POWER_GLITCH";
    case ESP_RST_CPU_LOCKUP:
        return "CPU_LOCKUP";
    case ESP_RST_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

static const char *ota_state_name(const platform_status_t *status)
{
    if (!status->ota_state_available) {
        return "UNAVAILABLE";
    }
    switch (status->ota_state) {
    case ESP_OTA_IMG_NEW:
        return "NEW";
    case ESP_OTA_IMG_PENDING_VERIFY:
        return "PENDING_VERIFY";
    case ESP_OTA_IMG_VALID:
        return "VALID";
    case ESP_OTA_IMG_INVALID:
        return "INVALID";
    case ESP_OTA_IMG_ABORTED:
        return "ABORTED";
    case ESP_OTA_IMG_UNDEFINED:
    default:
        return "UNDEFINED";
    }
}

static bool initialize_nvs_non_destructive(void)
{
    return nvs_flash_init() == ESP_OK;
}

static platform_status_t inspect_platform(void)
{
    platform_status_t status = {0};
    uint8_t mac[6] = {0};

    status.nvs_ready = initialize_nvs_non_destructive();
    if (esp_efuse_mac_get_default(mac) == ESP_OK) {
        snprintf(status.device_id, sizeof(status.device_id),
                 "ac-node-%02x%02x%02x%02x%02x%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        snprintf(status.device_id, sizeof(status.device_id),
                 "ac-node-identity-unavailable");
    }

    status.flash_ready = esp_flash_get_size(NULL, &status.flash_bytes) == ESP_OK;
    status.psram_bytes = esp_psram_get_size();
    status.psram_ready = status.psram_bytes >= VHOS_EXPECTED_PSRAM_BYTES;
    status.running_partition = esp_ota_get_running_partition();
    status.running_partition_ready = status.running_partition != NULL;
    if (status.running_partition_ready) {
        status.ota_state_available =
            esp_ota_get_state_partition(status.running_partition, &status.ota_state) == ESP_OK;
    }
    return status;
}

static void confirm_pending_image_if_safe(platform_status_t *status)
{
    if (!status->hardware_self_test_passed || !status->ota_state_available ||
        status->ota_state != ESP_OTA_IMG_PENDING_VERIFY) {
        return;
    }
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        status->ota_state = ESP_OTA_IMG_VALID;
    }
}

static void emit_boot_post(const platform_status_t *status,
                           const vhos_temperature_context_t *temperature)
{
    esp_chip_info_t chip_info = {0};
    const esp_app_desc_t *app = esp_app_get_description();
    esp_chip_info(&chip_info);

    printf(
        "{\"contract\":\"%s\",\"contract_version\":\"%s\",\"message\":\"post\","
        "\"device_id\":\"%s\",\"firmware_version\":\"%s\",\"build_id\":\"%s\","
        "\"software_profile\":\"TEMPERATURE_BENCH\",\"idf_version\":\"%s\","
        "\"board_claim\":\"ESP32-S3 runtime-detected\",\"chip_model\":\"ESP32-S3\","
        "\"chip_revision\":%u,\"cpu_cores\":%u,\"flash_bytes\":%" PRIu32 ","
        "\"psram_bytes\":%zu,\"reset_reason\":\"%s\",\"running_partition\":\"%s\","
        "\"ota_state\":\"%s\",\"hardware_self_test_passed\":%s,"
        "\"nvs_ready\":%s,\"flash_ready\":%s,\"psram_ready\":%s,"
        "\"board_temperature_status\":\"%s\","
        "\"board_temperature_semantic_role\":\"BOARD_DIE_HEALTH_ONLY\","
        "\"external_temperature_status\":\"HARDWARE_NOT_SELECTED\","
        "\"pressure_status\":\"UNAVAILABLE\",\"storage_status\":\"NOT_MOUNTED\","
        "\"wifi_state\":\"DISABLED\",\"ble_state\":\"DISABLED\","
        "\"state\":\"%s\"}\n",
        VHOS_BOOTSTRAP_CONTRACT, VHOS_BOOTSTRAP_CONTRACT_VERSION, status->device_id,
        VHOS_FIRMWARE_VERSION, VHOS_BUILD_ID, app->idf_ver, chip_info.revision,
        chip_info.cores, status->flash_bytes, status->psram_bytes,
        reset_reason_name(esp_reset_reason()),
        status->running_partition_ready ? status->running_partition->label : "UNAVAILABLE",
        ota_state_name(status), bool_json(status->hardware_self_test_passed),
        bool_json(status->nvs_ready), bool_json(status->flash_ready),
        bool_json(status->psram_ready),
        vhos_board_temperature_state_name(temperature->state),
        status->hardware_self_test_passed ? "DEGRADED" : "SENSOR_FAULT"
    );
    fflush(stdout);
}

static void emit_health(const platform_status_t *status,
                        const vhos_temperature_context_t *temperature,
                        uint64_t uptime_ms)
{
    const bool temperature_runtime_healthy =
        temperature->state == VHOS_BOARD_TEMP_READY ||
        temperature->state == VHOS_BOARD_TEMP_OBSERVED;
    const bool runtime_healthy =
        status->hardware_self_test_passed && temperature_runtime_healthy;
    printf(
        "{\"contract\":\"sensor.node.health\",\"contract_version\":\"0.2.0\","
        "\"device_id\":\"%s\",\"uptime_ms\":%" PRIu64 ","
        "\"software_profile\":\"TEMPERATURE_BENCH\","
        "\"hardware_self_test_passed\":%s,\"board_temperature_status\":\"%s\","
        "\"external_temperature_status\":\"HARDWARE_NOT_SELECTED\","
        "\"wifi_state\":\"DISABLED\",\"ble_state\":\"DISABLED\","
        "\"state\":\"%s\"}\n",
        status->device_id, uptime_ms, bool_json(status->hardware_self_test_passed),
        vhos_board_temperature_state_name(temperature->state),
        runtime_healthy ? "DEGRADED" : "SENSOR_FAULT"
    );
    fflush(stdout);
}

void app_main(void)
{
    platform_status_t status = inspect_platform();
    vhos_temperature_context_t temperature = {0};
    const bool temperature_ready = vhos_temperature_init(&temperature) == ESP_OK;

    status.hardware_self_test_passed =
        status.nvs_ready && status.flash_ready &&
        status.flash_bytes == VHOS_EXPECTED_FLASH_BYTES && status.psram_ready &&
        status.running_partition_ready && temperature_ready;
    confirm_pending_image_if_safe(&status);

    printf(
        "VHOS_RADIOS_DISABLED wifi=not-initialized ble=not-initialized "
        "reason=temperature-bench-profile\n"
    );
    emit_boot_post(&status, &temperature);

    uint64_t sample_counter = 0;
    while (true) {
        const uint64_t observed_at_monotonic_us = (uint64_t)esp_timer_get_time();
        vhos_temperature_emit(&temperature, status.device_id, sample_counter,
                              observed_at_monotonic_us);
        if ((sample_counter % 5U) == 0U) {
            emit_health(&status, &temperature, observed_at_monotonic_us / 1000U);
        }
        sample_counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
