#include "diagnostic_math.h"
#include <stddef.h>

bool ac_ads_configuration(unsigned channel, uint16_t *configuration)
{
    if (channel > 3 || configuration == NULL) return false;
    // Single-ended AINx/GND, +/-4.096 V, single-shot, 128 SPS, comparator off.
    *configuration = (uint16_t)(0xc383u | (channel << 12));
    return true;
}

int32_t ac_adc_microvolts(int16_t counts)
{
    return (int32_t)counts * 125;
}

uint16_t ac_word(const uint8_t bytes[2], bool little_endian)
{
    return little_endian ? (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8))
                         : (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}
