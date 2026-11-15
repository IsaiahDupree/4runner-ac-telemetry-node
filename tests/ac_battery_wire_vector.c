/* Emits a deterministic wire test vector using the production frame encoder. */
#include <assert.h>
#include <stdio.h>
#include "battery_wire.h"

int main(void)
{
    int16_t temperature;
    assert(battery_temperature(16000, 8000, &temperature) == 0 && temperature == 2500);
    assert(battery_temperature(16000, 0, &temperature) != 0);
    assert(battery_temperature(16000, 16000, &temperature) != 0);
    assert(battery_temperature(16000, 800, &temperature) != 0);

    const battery_snapshot_t sample = {
        .flags = BATTERY_VALID_GAUGE | BATTERY_VALID_NTC,
        .result = BATTERY_OK,
        .voltage = 49920,
        .current = (uint16_t)-2000,
        .average_current = (uint16_t)-1600,
        .state_of_charge = 18816,
        .remaining_capacity = 14200,
        .full_capacity = 19200,
        .age = 25120,
        .cycles = 2541,
        .time_to_empty = 960,
        .time_to_full = 100,
        .temperature_centi_c = -625,
    };
    uint8_t raw[BATTERY_PAYLOAD_SIZE];
    uint8_t wire[VHOS_HEADER_SIZE + sizeof(raw)];
    battery_payload(&sample, raw);
    assert(!battery_frame(VHOS_AC_BATTERY, 32, 2900000, raw, sizeof(raw), wire, sizeof(wire) - 1));
    const size_t size = battery_frame(VHOS_AC_BATTERY, 32, 2900000, raw, sizeof(raw), wire, sizeof(wire));
    assert(size == sizeof(wire));
    for (size_t i = 0; i < size; ++i) printf("%02x", wire[i]);
    putchar('\n');
    return 0;
}
