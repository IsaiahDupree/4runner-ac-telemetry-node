#pragma once
#include <stdbool.h>
#include <stdint.h>

bool ac_ads_configuration(unsigned channel, uint16_t *configuration);
int32_t ac_adc_microvolts(int16_t counts);
uint16_t ac_word(const uint8_t bytes[2], bool little_endian);
