#include "battery_wire.h"

#include <math.h>
#include <string.h>

int battery_temperature(int16_t voltage_counts, int16_t temperature_counts, int16_t *centi_c)
{
    if (voltage_counts < 8000 || temperature_counts <= 0 || temperature_counts >= voltage_counts) {
        return -1;
    }
    // Both signals are scaled by a nominal 2:1 divider and read at the same
    // gain. Semitec 103AT-2 is the engineering pack's specified run NTC.
    const double ratio = (double)temperature_counts / voltage_counts;
    const double resistance = 10000.0 * ratio / (1.0 - ratio);
    const double kelvin = 1.0 / (1.0 / 298.15 + log(resistance / 10000.0) / 3435.0);
    const double celsius = kelvin - 273.15;
    if (!isfinite(celsius) || celsius < -40.0 || celsius > 85.0) {
        return -1;
    }
    *centi_c = (int16_t)lround(celsius * 100.0);
    return 0;
}

static void write_le(uint8_t *dest, uint64_t value, size_t bytes)
{
    for (size_t i = 0; i < bytes; ++i) {
        dest[i] = (uint8_t)(value >> (8 * i));
    }
}

void battery_payload(const battery_snapshot_t *sample, uint8_t out[BATTERY_PAYLOAD_SIZE])
{
    out[0] = BATTERY_WIRE_VERSION;
    out[1] = sample->flags;
    out[2] = sample->result;
    out[3] = 0;
    const uint16_t words[] = {
        sample->status, sample->voltage, sample->current, sample->average_current,
        sample->state_of_charge, sample->remaining_capacity, sample->full_capacity,
        sample->age, sample->cycles, sample->time_to_empty, sample->time_to_full,
        (uint16_t)sample->temperature_centi_c, (uint16_t)sample->adc_voltage,
        (uint16_t)sample->adc_temperature,
    };
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        write_le(out + 4 + 2 * i, words[i], 2);
    }
}

static uint32_t crc32c(const uint8_t *input, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < size; ++i) {
        crc ^= input[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1) ? UINT32_C(0x82f63b78) : 0);
        }
    }
    return ~crc;
}

size_t battery_frame(uint8_t type, uint64_t seq, uint64_t time_us, const uint8_t *payload, size_t size,
                     uint8_t *out, size_t capacity)
{
    if (capacity < VHOS_HEADER_SIZE || size > capacity - VHOS_HEADER_SIZE || size > UINT32_MAX) {
        return 0;
    }
    memcpy(out, "VHOS", 4);
    out[4] = 1;
    out[5] = 0;
    out[6] = type;
    out[7] = 0;
    write_le(out + 8, size, 4);
    write_le(out + 12, seq, 8);
    write_le(out + 20, time_us, 8);
    write_le(out + 28, crc32c(payload, size), 4);
    write_le(out + 32, crc32c(out, 32), 4);
    memcpy(out + VHOS_HEADER_SIZE, payload, size);
    return VHOS_HEADER_SIZE + size;
}
