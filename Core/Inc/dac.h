#pragma once

#include <stdint.h>

typedef enum WaveType { SQUARE, SAWTOOTH, TRIANGLE, SINE } WaveType;

void dac_init();
void dac_start(WaveType type, uint16_t frequency_hz, uint8_t percent_duty);
