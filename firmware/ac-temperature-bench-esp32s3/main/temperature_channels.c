#include "temperature_channels.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define VHOS_BOARD_TEMP_RANGE_MIN_C (-10)
#define VHOS_BOARD_TEMP_RANGE_MAX_C 80
#define VHOS_BOARD_TEMP_DRIVER_MAX_ERROR_C 1

static void emit_unconfigured_external_channel(const char *channel_id)
{
    printf(
        "\"%s\":{\"value_c\":null,\"quality\":\"MISSING\","
        "\"state\":\"HARDWARE_NOT_SELECTED\",\"source\":\"UNCONFIGURED\","
        "\"sensor_identity\":null,\"driver_identity\":null,"
        "\"calibration\":{\"configured\":false,\"calibration_id\":null,"
        "\"revision\":null,\"validation_status\":null}}",
        channel_id
    );
}

esp_err_t vhos_temperature_init(vhos_temperature_context_t *context)
{
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(context, 0, sizeof(*context));
    context->state = VHOS_BOARD_TEMP_DRIVER_ERROR;
    const temperature_sensor_config_t config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(VHOS_BOARD_TEMP_RANGE_MIN_C,
                                          VHOS_BOARD_TEMP_RANGE_MAX_C);
    esp_err_t error = temperature_sensor_install(&config, &context->handle);
    if (error != ESP_OK) {
        return error;
    }
    context->installed = true;

    error = temperature_sensor_enable(context->handle);
    if (error != ESP_OK) {
        temperature_sensor_uninstall(context->handle);
        memset(context, 0, sizeof(*context));
        context->state = VHOS_BOARD_TEMP_DRIVER_ERROR;
        return error;
    }

    context->enabled = true;
    context->state = VHOS_BOARD_TEMP_READY;
    return ESP_OK;
}

void vhos_temperature_deinit(vhos_temperature_context_t *context)
{
    if (context == NULL) {
        return;
    }
    if (context->enabled) {
        temperature_sensor_disable(context->handle);
    }
    if (context->installed) {
        temperature_sensor_uninstall(context->handle);
    }
    memset(context, 0, sizeof(*context));
}

bool vhos_temperature_ready(const vhos_temperature_context_t *context)
{
    return context != NULL && context->installed && context->enabled;
}

const char *vhos_board_temperature_state_name(vhos_board_temperature_state_t state)
{
    switch (state) {
    case VHOS_BOARD_TEMP_READY:
        return "READY";
    case VHOS_BOARD_TEMP_OBSERVED:
        return "OBSERVED_UNCALIBRATED";
    case VHOS_BOARD_TEMP_OUT_OF_RANGE:
        return "OUT_OF_DRIVER_RANGE";
    case VHOS_BOARD_TEMP_READ_ERROR:
        return "READ_ERROR";
    case VHOS_BOARD_TEMP_DRIVER_ERROR:
    default:
        return "DRIVER_ERROR";
    }
}

void vhos_temperature_emit(vhos_temperature_context_t *context,
                           const char *device_id,
                           uint64_t sample_counter,
                           uint64_t observed_at_monotonic_us)
{
    float value_c = 0.0f;
    esp_err_t read_error = ESP_ERR_INVALID_STATE;
    if (vhos_temperature_ready(context)) {
        read_error = temperature_sensor_get_celsius(context->handle, &value_c);
    }

    if (read_error == ESP_OK) {
        context->last_value_c = value_c;
        context->state = VHOS_BOARD_TEMP_OBSERVED;
    } else if (read_error == ESP_FAIL) {
        context->state = VHOS_BOARD_TEMP_OUT_OF_RANGE;
    } else if (vhos_temperature_ready(context)) {
        context->state = VHOS_BOARD_TEMP_READ_ERROR;
    } else {
        context->state = VHOS_BOARD_TEMP_DRIVER_ERROR;
    }

    printf(
        "{\"contract\":\"sensor.node.temperature-bench\","
        "\"contract_version\":\"0.1.0\",\"device_id\":\"%s\","
        "\"software_profile\":\"TEMPERATURE_BENCH\","
        "\"sample_counter\":%" PRIu64 ",\"observed_at_monotonic_us\":%" PRIu64 ","
        "\"channels\":{",
        device_id, sample_counter, observed_at_monotonic_us
    );

    if (context->state == VHOS_BOARD_TEMP_OBSERVED) {
        printf(
            "\"board\":{\"value_c\":%.2f,\"quality\":\"SENSOR_UNVERIFIED\","
            "\"state\":\"%s\",\"source\":\"ESP32_S3_INTERNAL_DIE\","
            "\"semantic_role\":\"BOARD_DIE_HEALTH_ONLY\","
            "\"driver_identity\":\"esp-idf.esp32s3.tsens/5.5.3\","
            "\"driver_range_min_c\":%d,\"driver_range_max_c\":%d,"
            "\"driver_claimed_max_error_c\":%d,"
            "\"calibration\":{\"configured\":false,\"calibration_id\":null,"
            "\"revision\":null,\"validation_status\":null}},",
            context->last_value_c, vhos_board_temperature_state_name(context->state),
            VHOS_BOARD_TEMP_RANGE_MIN_C, VHOS_BOARD_TEMP_RANGE_MAX_C,
            VHOS_BOARD_TEMP_DRIVER_MAX_ERROR_C
        );
    } else {
        printf(
            "\"board\":{\"value_c\":null,\"quality\":\"%s\","
            "\"state\":\"%s\",\"source\":\"ESP32_S3_INTERNAL_DIE\","
            "\"semantic_role\":\"BOARD_DIE_HEALTH_ONLY\","
            "\"driver_identity\":\"esp-idf.esp32s3.tsens/5.5.3\","
            "\"driver_range_min_c\":%d,\"driver_range_max_c\":%d,"
            "\"driver_claimed_max_error_c\":%d,"
            "\"calibration\":{\"configured\":false,\"calibration_id\":null,"
            "\"revision\":null,\"validation_status\":null}},",
            context->state == VHOS_BOARD_TEMP_OUT_OF_RANGE ? "OUT_OF_RANGE" : "MISSING",
            vhos_board_temperature_state_name(context->state),
            VHOS_BOARD_TEMP_RANGE_MIN_C, VHOS_BOARD_TEMP_RANGE_MAX_C,
            VHOS_BOARD_TEMP_DRIVER_MAX_ERROR_C
        );
    }

    emit_unconfigured_external_channel("high_line");
    printf(",");
    emit_unconfigured_external_channel("low_line");
    printf(",");
    emit_unconfigured_external_channel("ambient");
    printf(",");
    emit_unconfigured_external_channel("cabin_return");
    printf(",");
    emit_unconfigured_external_channel("center_vent");
    printf("}}\n");
    fflush(stdout);
}
