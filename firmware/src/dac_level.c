#include "dac_level.h"

#define DAC_MAX_VALUE 1023

uint32_t dacValueFromSampleCounted(int32_t y, uint32_t *saturations) {
    int32_t v = (y >> 6) + 512;
    if (v > DAC_MAX_VALUE) {
        v = DAC_MAX_VALUE;
        (*saturations)++;
    } else if (v < 0) {
        v = 0;
        (*saturations)++;
    }
    return (uint32_t)v;
}

uint32_t dacValueFromSample(int32_t y) {
    uint32_t discarded = 0;
    return dacValueFromSampleCounted(y, &discarded);
}

int16_t dacValueToQ15(uint32_t dacValue) {
    return (int16_t)(((int32_t)dacValue - 512) * 64);
}
