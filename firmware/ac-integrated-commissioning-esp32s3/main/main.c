#include "board.h"
#include "diagnostic_math.h"
#include "display.h"

#include <inttypes.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t gauge, adc;
#if AC_HAS_DISPLAY
static i2c_master_dev_handle_t touch;
static portMUX_TYPE key_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned key_state, key_edges, display_page;

static void key_task(void *unused)
{
    const int pins[] = {AC_MENU, AC_UP, AC_DOWN, AC_SELECT};
    unsigned candidate = 0, stable = 0;
    for (;;) {
        unsigned state = 0;
        for (unsigned n = 0; n < 4; ++n) state |= (!gpio_get_level(pins[n])) << n;
        if (state != candidate) {candidate = state; stable = 0;}
        else if (stable < 3) ++stable;
        if (stable == 3) {
            portENTER_CRITICAL(&key_lock);
            unsigned pressed = state & ~key_state;
            key_edges |= pressed;
            if (pressed & 1) display_page = (display_page + 1) % 2;
            key_state = state;
            portEXIT_CRITICAL(&key_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(20) + 1);
    }
}
#endif
enum { IO_TIMEOUT = 40 };

static esp_err_t read_word(i2c_master_dev_handle_t dev, uint8_t reg, bool little, uint16_t *value)
{
    if (!dev) return ESP_ERR_INVALID_STATE;
    uint8_t data[2];
    esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, data, 2, IO_TIMEOUT);
    if (err == ESP_OK) *value = ac_word(data, little);
    return err;
}

static esp_err_t device_add(unsigned address, i2c_master_dev_handle_t *dev)
{
    if (!bus) return ESP_ERR_INVALID_STATE;
    i2c_device_config_t config = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address, .scl_speed_hz = 100000};
    return i2c_master_bus_add_device(bus, &config, dev);
}

static esp_err_t input(int pin)
{
    gpio_config_t config = {.pin_bit_mask = 1ULL << pin, .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, .intr_type = GPIO_INTR_DISABLE};
    return gpio_config(&config);
}

static esp_err_t output(int pin, unsigned level)
{
    // Preload the output latch before enabling the driver.
    esp_err_t err = gpio_set_level(pin, level);
    return err == ESP_OK ? gpio_set_direction(pin, GPIO_MODE_OUTPUT) : err;
}

static esp_err_t initialize(void)
{
    ESP_ERROR_CHECK(output(AC_PRESS_H_ON, 0));
    ESP_ERROR_CHECK(output(AC_PRESS_L_ON, 0));
    ESP_ERROR_CHECK(output(AC_BOOST_EN, 0));
    ESP_ERROR_CHECK(output(AC_MUX, 0));
    ESP_ERROR_CHECK(output(AC_SD_CS, 1));
    ESP_ERROR_CHECK(input(AC_SD_CD));
    ESP_ERROR_CHECK(input(AC_PRESS_H_FLT));
    ESP_ERROR_CHECK(input(AC_PRESS_L_FLT));
    ESP_ERROR_CHECK(input(AC_ADC_RDY));
#if AC_HAS_DISPLAY
    ESP_ERROR_CHECK(output(AC_RTD_H_CS, 1));
    ESP_ERROR_CHECK(output(AC_RTD_L_CS, 1));
    const int keys[] = {AC_MENU, AC_UP, AC_DOWN, AC_SELECT, AC_TOUCH_IRQ};
    for (unsigned i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i) ESP_ERROR_CHECK(input(keys[i]));
#endif
    const i2c_master_bus_config_t config = {.i2c_port = I2C_NUM_0,
        .sda_io_num = AC_SDA, .scl_io_num = AC_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7};
    esp_err_t err = i2c_new_master_bus(&config, &bus);
    if (err != ESP_OK) return err;
    err = device_add(AC_GAUGE_ADDRESS, &gauge);
    if (err != ESP_OK) return err;
    err = device_add(AC_ADS_ADDRESS, &adc);
#if AC_HAS_DISPLAY
    if (err == ESP_OK) err = device_add(AC_TOUCH_ADDRESS, &touch);
#endif
    return err;
}

static esp_err_t adc_read(unsigned bank, unsigned channel, int16_t *counts)
{
    uint16_t config;
    if (!adc || bank > 1 || !ac_ads_configuration(channel, &config)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = gpio_set_level(AC_MUX, bank);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(15) + 1);
    uint8_t request[] = {1, (uint8_t)(config >> 8), (uint8_t)config};
    err = i2c_master_transmit(adc, request, sizeof(request), IO_TIMEOUT);
    if (err != ESP_OK) return err;
    for (unsigned n = 0; n < 5; ++n) {
        vTaskDelay(pdMS_TO_TICKS(10) + 1);
        uint16_t state, raw;
        err = read_word(adc, 1, false, &state);
        if (err != ESP_OK) return err;
        if ((state & 0x7fff) != (config & 0x7fff)) return ESP_ERR_INVALID_RESPONSE;
        if (!(state & 0x8000)) continue;
        err = read_word(adc, 0, false, &raw);
        if (err == ESP_OK) *counts = (int16_t)raw;
        return err;
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t gauge_report(uint64_t seq)
{
    uint16_t identity;
    esp_err_t err = read_word(gauge, 0x21, true, &identity);
    if (err == ESP_OK && identity != 0x4031) err = ESP_ERR_INVALID_RESPONSE;
    printf("{\"event\":\"gauge_identity\",\"seq\":%" PRIu64 ",\"io\":%d,\"model_qualified\":false}\n", seq, err);
    if (err != ESP_OK) return err;
    // Read-only: never write a guessed pack capacity, thermistor or charging model.
    const uint8_t regs[] = {0x00,0x09,0x0a,0x0b,0x06,0x05,0x10,0x07,0x17,0x11,0x20,0x18,0x1e,0x3a,0x3d,0xdb};
    esp_err_t first = ESP_OK;
    for (unsigned n = 0; n < sizeof(regs); ++n) {
        uint16_t raw;
        err = read_word(gauge, regs[n], true, &raw);
        printf("{\"event\":\"gauge_raw\",\"seq\":%" PRIu64 ",\"register\":%u,\"io\":%d,\"raw\":", seq, regs[n], err);
        if (err == ESP_OK) printf("%u", raw); else { printf("null"); if (first == ESP_OK) first = err; }
        printf(",\"quality\":\"RAW_UNQUALIFIED_PACK\"}\n");
    }
    return first;
}

#if AC_HAS_DISPLAY
static esp_err_t touch_axis(uint8_t command, uint16_t *value)
{
    if (!touch) return ESP_ERR_INVALID_STATE;
    esp_err_t err = i2c_master_transmit(touch, &command, 1, IO_TIMEOUT);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(2) + 1);
    uint8_t data[2];
    err = i2c_master_receive(touch, data, 2, IO_TIMEOUT);
    if (err == ESP_OK) *value = (uint16_t)((data[0] << 4) | (data[1] >> 4));
    return err;
}
#endif

void app_main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    printf("{\"event\":\"identity\",\"profile\":\"INTEGRATED_COMMISSIONING\",\"board\":\"%s\","
        "\"source_sha256\":\"%s\",\"efuse_mac\":\"%02x%02x%02x%02x%02x%02x\","
        "\"ads_address\":%u,\"radio\":\"DISABLED\",\"pressure\":\"UNCONFIGURED\","
        "\"line_temperature\":\"UNCONFIGURED\",\"battery_model\":\"UNQUALIFIED\","
        "\"usb_role\":\"CHARGE_SINK_SERVICE_DATA\"}\n", AC_BOARD_NAME, AC_SOURCE_SHA256,
        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],AC_ADS_ADDRESS);
    esp_err_t init = initialize();
    printf("{\"event\":\"i2c_setup\",\"io\":%d}\n", init);
    esp_err_t screen = ac_display_start();
    printf("{\"event\":\"display_setup\",\"io\":%d,\"physical_verified\":false}\n", screen);
#if AC_HAS_DISPLAY
    if (xTaskCreate(key_task,"keys",2048,NULL,4,NULL) != pdPASS)
        printf("{\"event\":\"key_task\",\"io\":%d}\n",ESP_ERR_NO_MEM);
#endif
    for (uint64_t seq = 1;; ++seq) {
        esp_err_t g = gauge_report(seq), a = ESP_OK;
        for (unsigned bank = 0; bank < 2; ++bank) for (unsigned ch = 0; ch < 4; ++ch) {
            int16_t counts;
            esp_err_t err = adc_read(bank, ch, &counts);
            if (err != ESP_OK) a = err;
            printf("{\"event\":\"adc_raw\",\"seq\":%" PRIu64 ",\"bank\":%u,\"channel\":%u,\"io\":%d,\"counts\":",seq,bank,ch,err);
            if (err == ESP_OK) printf("%d,\"adc_input_uv\":%" PRId32,counts,ac_adc_microvolts(counts));
            else printf("null,\"adc_input_uv\":null");
            printf(",\"quality\":\"RAW_NO_SENSOR_CALIBRATION\",\"pressure_supply\":\"OFF\"}\n");
        }
        (void)gpio_set_level(AC_MUX, 0);
        unsigned buttons = 0, edges = 0, page = 0;
        int tx = -1, ty = -1;
#if AC_HAS_DISPLAY
        portENTER_CRITICAL(&key_lock);
        buttons = key_state; edges = key_edges; key_edges = 0; page = display_page;
        portEXIT_CRITICAL(&key_lock);
        if (!gpio_get_level(AC_TOUCH_IRQ)) {
            uint16_t x, y, unused;
            esp_err_t t = touch_axis(0xc4, &x);
            if (t == ESP_OK) t = touch_axis(0xd4, &y);
            // Always restore power-down/IRQ-on after a touch conversion attempt.
            esp_err_t down = touch_axis(0x00, &unused);
            printf("{\"event\":\"touch_raw\",\"seq\":%" PRIu64 ",\"io\":%d,\"powerdown_io\":%d,\"xy\":",seq,t,down);
            if (t == ESP_OK) {printf("[%u,%u]",x,y);tx=x;ty=y;} else printf("null");
            printf(",\"calibrated\":false}\n");
        }
#endif
        printf("{\"event\":\"digital\",\"seq\":%" PRIu64 ",\"uptime_us\":%" PRId64 ",\"buttons_pressed\":%u,"
            "\"button_press_edges\":%u,\"sd_cd_level\":%d,\"high_fault_n\":%d,\"low_fault_n\":%d}\n",seq,esp_timer_get_time(),buttons,edges,
            gpio_get_level(AC_SD_CD),gpio_get_level(AC_PRESS_H_FLT),gpio_get_level(AC_PRESS_L_FLT));
        if (screen == ESP_OK) {
            esp_err_t update = ac_display_status(page, g, a, buttons, tx, ty);
            if (update != ESP_OK) printf("{\"event\":\"display_io\",\"io\":%d}\n",update);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
