#pragma once

#include <stddef.h>
#include <stdint.h>

#define BATTERY_WIRE_VERSION 1
#define BATTERY_PAYLOAD_SIZE 32
#define VHOS_HEADER_SIZE 36
#define VHOS_HANDSHAKE 1
#define VHOS_AC_BATTERY 14

enum battery_flags {
    BATTERY_VALID_GAUGE = 1,
    BATTERY_VALID_NTC = 2,
};

enum battery_result {
    BATTERY_OK,
    BATTERY_GAUGE_IO,
    BATTERY_GAUGE_MODEL,
    BATTERY_GAUGE_RANGE,
    BATTERY_NTC_UNAVAILABLE,
};

typedef struct {
    uint8_t flags;
    uint8_t result;
    uint16_t status;
    uint16_t voltage;
    uint16_t current;
    uint16_t average_current;
    uint16_t state_of_charge;
    uint16_t remaining_capacity;
    uint16_t full_capacity;
    uint16_t age;
    uint16_t cycles;
    uint16_t time_to_empty;
    uint16_t time_to_full;
    int16_t temperature_centi_c;
    int16_t adc_voltage;
    int16_t adc_temperature;
} battery_snapshot_t;

int battery_temperature(int16_t voltage_counts, int16_t temperature_counts, int16_t *centi_c);
void battery_payload(const battery_snapshot_t *sample, uint8_t out[BATTERY_PAYLOAD_SIZE]);
size_t battery_frame(uint8_t type, uint64_t seq, uint64_t time_us, const uint8_t *payload, size_t size,
                     uint8_t *out, size_t capacity);
