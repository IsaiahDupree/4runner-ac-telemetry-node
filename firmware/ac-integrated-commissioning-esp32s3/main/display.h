#pragma once
#include "esp_err.h"

esp_err_t ac_display_start(void);
esp_err_t ac_display_status(unsigned page, int gauge_error, int adc_error, unsigned buttons,
                            int touch_x, int touch_y);
