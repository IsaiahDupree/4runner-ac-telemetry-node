#include "board.h"
#include "display.h"

#if AC_HAS_DISPLAY
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hx8357d_init.h"

static spi_device_handle_t lcd;
static uint8_t pixels[320 * 2];
// Five-column commissioning glyphs, bit zero is the top row.
static const uint8_t alphabet[26][5] = {
 {0x7e,0x09,0x09,0x09,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x41,0x3e},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x3a},{0x7f,0x08,0x08,0x08,0x7f},{0,0x41,0x7f,0x41,0},
 {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
 {0x26,0x49,0x49,0x49,0x32},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
 {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43}
};
static const uint8_t digits[10][5] = {
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x62,0x51,0x49,0x49,0x46},
 {0x22,0x41,0x49,0x49,0x36},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
 {0x06,0x49,0x49,0x29,0x1e}
};

static esp_err_t transfer(const void *data, size_t bytes, bool is_data)
{
    if (!lcd) return ESP_ERR_INVALID_STATE;
    esp_err_t err = gpio_set_level(AC_LCD_DC, is_data);
    if (err != ESP_OK) return err;
    spi_transaction_t t = {.length = bytes * 8, .tx_buffer = data};
    return spi_device_transmit(lcd, &t);
}

static esp_err_t command(uint8_t cmd, const uint8_t *args, size_t count)
{
    esp_err_t err = transfer(&cmd, 1, false);
    return err == ESP_OK && count ? transfer(args, count, true) : err;
}

static esp_err_t window(unsigned y)
{
    const uint8_t x[] = {0,0,1,0x3f}, row[] = {(uint8_t)(y >> 8),(uint8_t)y,(uint8_t)(y >> 8),(uint8_t)y};
    esp_err_t err = command(0x2a, x, 4);
    if (err == ESP_OK) err = command(0x2b, row, 4);
    if (err == ESP_OK) err = command(0x2c, NULL, 0);
    return err;
}

static unsigned column(char c, unsigned col)
{
    if (col >= 5) return 0;
    if (c >= 'A' && c <= 'Z') return alphabet[c-'A'][col];
    if (c >= '0' && c <= '9') return digits[c-'0'][col];
    if (c == ':') return col == 2 ? 0x24 : 0;
    if (c == '-') return 0x08;
    if (c == '/') return (unsigned[]){0x60,0x10,0x08,0x04,0x03}[col];
    return 0;
}

static esp_err_t text(unsigned y, const char *text, uint16_t foreground)
{
    size_t length = strlen(text);
    for (unsigned row = 0; row < 18; ++row) {
        for (unsigned x = 0; x < 320; ++x) {
            unsigned i = x/12, col = (x%12)/2;
            bool lit = row < 14 && i < length && (column(text[i],col) & (1u << (row/2)));
            uint16_t color = lit ? foreground : 0x0841;
            pixels[2*x] = color >> 8; pixels[2*x+1] = color;
        }
        esp_err_t err = window(y+row);
        if (err == ESP_OK) err = transfer(pixels, sizeof(pixels), true);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t ac_display_start(void)
{
    gpio_config_t gpio = {.pin_bit_mask = (1ULL << AC_LCD_DC) | (1ULL << AC_LCD_RESET) | (1ULL << AC_LCD_BL),
        .mode = GPIO_MODE_OUTPUT};
    esp_err_t err = gpio_config(&gpio);
    if (err != ESP_OK) return err;
    (void)gpio_set_level(AC_LCD_BL, 0);
    (void)gpio_set_level(AC_LCD_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(20) + 1);
    (void)gpio_set_level(AC_LCD_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(150) + 1);
    spi_bus_config_t bus = {.mosi_io_num = AC_MOSI, .miso_io_num = AC_MISO, .sclk_io_num = AC_SCK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = sizeof(pixels)};
    err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) return err;
    spi_device_interface_config_t device = {.clock_speed_hz = 10000000, .mode = 0,
        .spics_io_num = AC_LCD_CS, .queue_size = 1};
    err = spi_bus_add_device(SPI2_HOST, &device, &lcd);
    if (err != ESP_OK) return err;
    size_t i = 0;
    while (i < sizeof(hx8357d_init)) {
        uint8_t cmd = hx8357d_init[i++];
        if (!cmd) break;
        if (i >= sizeof(hx8357d_init)) return ESP_ERR_INVALID_SIZE;
        uint8_t count = hx8357d_init[i++];
        if (count & 0x80) {
            if (cmd != 0xff && (err = command(cmd,NULL,0)) != ESP_OK) return err;
            vTaskDelay(pdMS_TO_TICKS((count & 0x7f)*5) + 1);
        } else {
            if (i + count > sizeof(hx8357d_init)) return ESP_ERR_INVALID_SIZE;
            err = command(cmd,hx8357d_init+i,count); i += count;
            if (err != ESP_OK) return err;
        }
    }
    memset(pixels,0,sizeof(pixels));
    for (unsigned y = 0; y < 480; ++y) {
        err = window(y);
        if (err == ESP_OK) err = transfer(pixels,sizeof(pixels),true);
        if (err != ESP_OK) return err;
    }
    const ledc_timer_config_t timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT, .timer_num = LEDC_TIMER_0, .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK};
    err = ledc_timer_config(&timer);
    if (err != ESP_OK) return err;
    const ledc_channel_config_t channel = {.gpio_num = AC_LCD_BL, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0, .duty = 128, .hpoint = 0};
    return ledc_channel_config(&channel);
}

esp_err_t ac_display_status(unsigned page, int gauge_error, int adc_error, unsigned buttons,
                            int touch_x, int touch_y)
{
    char lines[11][27] = {"AC H2 COMMISSIONING", "USB-C CHARGE INPUT", "PRESSURE UNCONFIGURED",
        "TEMP UNCONFIGURED", "PACK MODEL UNQUALIFIED", "", "", "", "", "", "MENU CHANGES PAGE"};
    snprintf(lines[5],sizeof(lines[5]),"GAUGE I/O %s",gauge_error == ESP_OK ? "OK" : "ERROR");
    snprintf(lines[6],sizeof(lines[6]),"ADC I/O %s",adc_error == ESP_OK ? "OK" : "ERROR");
    snprintf(lines[7],sizeof(lines[7]),"BUTTON MASK %u",buttons);
    if (page) {
        if (touch_x >= 0 && touch_y >= 0)
            snprintf(lines[8],sizeof(lines[8]),"TOUCH X %d Y %d",touch_x,touch_y);
        else snprintf(lines[8],sizeof(lines[8]),"TOUCH NOT AVAILABLE");
        snprintf(lines[9],sizeof(lines[9]),"RAW TOUCH UNCALIBRATED");
    } else {
        snprintf(lines[8],sizeof(lines[8]),"SENSOR POWER OFF");
        snprintf(lines[9],sizeof(lines[9]),"DETAILS VIA USB SERIAL");
    }
    for (unsigned n = 0; n < 11; ++n) {
        esp_err_t err = text(16 + n*36,lines[n],n < 2 ? 0x5fff : 0xffff);
        if (err != ESP_OK) return err;
    }
    // A successful SPI transfer is not proof that the display rendered correctly.
    return ESP_OK;
}
#else
esp_err_t ac_display_start(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t ac_display_status(unsigned page, int gauge_error, int adc_error, unsigned buttons,
                            int touch_x, int touch_y) { return ESP_ERR_NOT_SUPPORTED; }
#endif
