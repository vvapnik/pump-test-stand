#pragma once
#include <Arduino.h>

// ACS712-20A: 100 mV/A, ~Vcc/2 at zero current (the real offset is calibrated
// automatically at startup rather than assumed).
constexpr float ACS712_MV_PER_AMP = 100.0f;

void currentSenseInit();

// Averaged measured current (mA) - what the ACS712 actually sees through the
// RC filter, i.e. the PWM-chopped average current on the +12V rail.
int16_t readAveragedCurrentMa();

// Estimated "effective" motor current (mA): measured average current divided by
// the duty cycle, so readings at different duty values are comparable. duty01 in [0,1].
// Returns 0 when duty01 == 0 (motor is not running, nothing to divide by).
int16_t estimateEffectiveCurrentMa(int16_t measuredMa, float duty01);
