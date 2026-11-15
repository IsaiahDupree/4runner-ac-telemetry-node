#include "battery.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum {
    MAX17260_ADDRESS = 0x36,
    ADS1115_ADDRESS = 0x48,
    IO_TIMEOUT_MS = 40,
    MAX_STATUS = 0x00,
    MAX_DEV_NAME = 0x21,
    MAX_FSTAT = 0x3d,
    MAX_HIB_CFG = 0xba,
    MAX_COMMAND = 0x60,
    MAX_DESIGN_CAP = 0x18,
    MAX_ICHG_TERM = 0x1e,
    MAX_VEMPTY = 0x3a,
    MAX_MODEL_CFG = 0xdb,
    MAX_POR = 1 << 1,
    MAX_REFRESH = 1 << 15,
    DESIGN_CAPACITY_MAH = 9800,
    TERMINATION_MA = 100,
    EMPTY_MV = 3000,
    RECOVERY_MV = 3500,
    MUX_SELECT = GPIO_NUM_17,
};

static i2c_master_dev_handle_t gauge;
static i2c_master_dev_handle_t adc;

static esp_err_t read_word(i2c_master_dev_handle_t device, uint8_t reg, bool little, uint16_t *value)
{
    uint8_t bytes[2];
    const esp_err_t result = i2c_master_transmit_receive(device, &reg, 1, bytes, sizeof(bytes), IO_TIMEOUT_MS);
    if (result == ESP_OK) {
        *value = little ? (uint16_t)(bytes[0] | (bytes[1] << 8))
                        : (uint16_t)((bytes[0] << 8) | bytes[1]);
    }
    return result;
}

static esp_err_t write_word(i2c_master_dev_handle_t device, uint8_t reg, bool little, uint16_t value)
{
    const uint8_t low = (uint8_t)value;
    const uint8_t high = (uint8_t)(value >> 8);
    const uint8_t command[] = {reg, little ? low : high, little ? high : low};
    return i2c_master_transmit(device, command, sizeof(command), IO_TIMEOUT_MS);
}

esp_err_t battery_bus_start(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_15,
        .scl_io_num = GPIO_NUM_16,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus;
    esp_err_t result = i2c_new_master_bus(&bus_config, &bus);
    if (result != ESP_OK) return result;
    const i2c_device_config_t gauge_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = MAX17260_ADDRESS, .scl_speed_hz = 100000,
    };
    const i2c_device_config_t adc_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = ADS1115_ADDRESS, .scl_speed_hz = 100000,
    };
    result = i2c_master_bus_add_device(bus, &gauge_config, &gauge);
    if (result != ESP_OK) return result;
    result = i2c_master_bus_add_device(bus, &adc_config, &adc);
    if (result != ESP_OK) return result;
    return gpio_set_direction(MUX_SELECT, GPIO_MODE_OUTPUT);
}

static esp_err_t wait_for_model(uint8_t reg, uint16_t busy)
{
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        uint16_t word;
        const esp_err_t result = read_word(gauge, reg, true, &word);
        if (result != ESP_OK) return result;
        if (!(word & busy)) return ESP_OK;
        vTaskDelay(pdMS_TO_TICKS(20) + 1);
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t configure_model(void)
{
    // Analog Devices UG6595 EZ startup, for the 10 mΩ 1S2P pack. No live
    // charging or switched sensor loads during initialization in RUN.
    esp_err_t result = wait_for_model(MAX_FSTAT, 1);
    if (result != ESP_OK) return result;
    uint16_t hibernate;
    result = read_word(gauge, MAX_HIB_CFG, true, &hibernate);
    if (result != ESP_OK) return result;
    const struct { uint8_t reg; uint16_t value; } configuration[] = {
        {MAX_COMMAND, 0x0090}, {MAX_HIB_CFG, 0}, {MAX_COMMAND, 0},
        {MAX_DESIGN_CAP, DESIGN_CAPACITY_MAH * 2},
        {MAX_ICHG_TERM, TERMINATION_MA * 32 / 5},
        {MAX_VEMPTY, (EMPTY_MV / 10 << 7) | (RECOVERY_MV / 40)},
        {MAX_MODEL_CFG, MAX_REFRESH},
    };
    for (unsigned i = 0; i < sizeof(configuration) / sizeof(configuration[0]); ++i) {
        result = write_word(gauge, configuration[i].reg, true, configuration[i].value);
        if (result != ESP_OK) break;
    }
    if (result == ESP_OK) result = wait_for_model(MAX_MODEL_CFG, MAX_REFRESH);
    const esp_err_t restore = write_word(gauge, MAX_HIB_CFG, true, hibernate);
    if (result != ESP_OK) {
        (void)write_word(gauge, MAX_COMMAND, true, 0);
        return result;
    }
    if (restore != ESP_OK) return restore;
    uint16_t status;
    result = read_word(gauge, MAX_STATUS, true, &status);
    if (result != ESP_OK) return result;
    result = write_word(gauge, MAX_STATUS, true, status & ~MAX_POR);
    if (result != ESP_OK) return result;
    result = read_word(gauge, MAX_STATUS, true, &status);
    return result == ESP_OK && (status & MAX_POR) ? ESP_ERR_INVALID_STATE : result;
}

static esp_err_t read_ntc_bank(bool select, int16_t *counts)
{
    esp_err_t result = gpio_set_level(MUX_SELECT, select);
    if (result != ESP_OK) return result;
    vTaskDelay(pdMS_TO_TICKS(10) + 1);
    // ADS1115 AIN3/GND at ±4.096 V, single conversion, 128 SPS,
    // comparator disabled. ADS register bytes are MSB-first.
    result = write_word(adc, 0x01, false, 0xf383);
    if (result != ESP_OK) return result;
    for (int attempt = 0; attempt < 4; ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(10) + 1);
        uint16_t config, value;
        result = read_word(adc, 0x01, false, &config);
        if (result != ESP_OK) return result;
        if (!(config & 0x8000)) continue;
        result = read_word(adc, 0x00, false, &value);
        if (result == ESP_OK) *counts = (int16_t)value;
        return result;
    }
    return ESP_ERR_TIMEOUT;
}

battery_snapshot_t battery_poll(void)
{
    battery_snapshot_t sample = {.result = BATTERY_GAUGE_IO};
    uint16_t name, status, capacity;
    if (!gauge || read_word(gauge, MAX_DEV_NAME, true, &name) != ESP_OK || name != 0x4031 ||
        read_word(gauge, MAX_STATUS, true, &status) != ESP_OK) {
        return sample;
    }
    if ((status & MAX_POR) && configure_model() != ESP_OK) {
        sample.result = BATTERY_GAUGE_MODEL;
        return sample;
    }
    if (read_word(gauge, MAX_DESIGN_CAP, true, &capacity) != ESP_OK || capacity != DESIGN_CAPACITY_MAH * 2) {
        sample.result = BATTERY_GAUGE_MODEL;
        return sample;
    }
    const struct { uint8_t reg; uint16_t *dest; } fields[] = {
        {0x09, &sample.voltage}, {0x0a, &sample.current}, {0x0b, &sample.average_current},
        {0x06, &sample.state_of_charge}, {0x05, &sample.remaining_capacity}, {0x10, &sample.full_capacity},
        {0x07, &sample.age}, {0x17, &sample.cycles}, {0x11, &sample.time_to_empty}, {0x20, &sample.time_to_full},
        {MAX_STATUS, &sample.status},
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        if (read_word(gauge, fields[i].reg, true, fields[i].dest) != ESP_OK) return sample;
    }
    if ((sample.status & MAX_POR) || sample.voltage < 25600 || sample.voltage > 55680 ||
        sample.state_of_charge > 25600 || !sample.full_capacity) {
        sample.result = BATTERY_GAUGE_RANGE;
        return sample;
    }
    sample.flags = BATTERY_VALID_GAUGE;
    sample.result = BATTERY_NTC_UNAVAILABLE;
    if (adc && read_ntc_bank(false, &sample.adc_voltage) == ESP_OK &&
        read_ntc_bank(true, &sample.adc_temperature) == ESP_OK &&
        battery_temperature(sample.adc_voltage, sample.adc_temperature, &sample.temperature_centi_c) == 0) {
        sample.flags |= BATTERY_VALID_NTC;
        sample.result = BATTERY_OK;
    }
    (void)gpio_set_level(MUX_SELECT, 0);
    return sample;
}
