#pragma once
#include <stdint.h>

void pwm_init(void);

// duty 0..100 (%).
void pwm_set_duty_percent(uint8_t duty);
