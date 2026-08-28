#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/temperature_sensor.h"
#include "esp_err.h"

typedef enum {
    VHOS_BOARD_TEMP_DRIVER_ERROR = 0,
    VHOS_BOARD_TEMP_READY,
    VHOS_BOARD_TEMP_OBSERVED,
    VHOS_BOARD_TEMP_OUT_OF_RANGE,
    VHOS_BOARD_TEMP_READ_ERROR,
} vhos_board_temperature_state_t;

typedef struct {
    temperature_sensor_handle_t handle;
    bool installed;
    bool enabled;
    vhos_board_temperature_state_t state;
    float last_value_c;
} vhos_temperature_context_t;

esp_err_t vhos_temperature_init(vhos_temperature_context_t *context);
void vhos_temperature_deinit(vhos_temperature_context_t *context);
bool vhos_temperature_ready(const vhos_temperature_context_t *context);
const char *vhos_board_temperature_state_name(vhos_board_temperature_state_t state);
void vhos_temperature_emit(vhos_temperature_context_t *context,
                           const char *device_id,
                           uint64_t sample_counter,
                           uint64_t observed_at_monotonic_us);
