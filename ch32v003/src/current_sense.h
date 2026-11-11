#pragma once
#include <stdint.h>

// ACS712-20A: 100 mV/A. The offset is calibrated in current_sense_init() -
// call it while the pump is guaranteed off (PWM=0).
void current_sense_init(void);

int16_t current_sense_read_ma(void);
