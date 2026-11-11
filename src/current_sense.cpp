#include "current_sense.h"
#include "pins.h"

static float s_zeroOffsetV = 2.5f;
static constexpr float ADC_VREF_V = 5.0f;
static constexpr int ADC_MAX_COUNTS = 1023;
static constexpr int SAMPLE_COUNT = 32;

static float readAdcVoltageAveraged(int samples) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(PIN_CURRENT_SENSE);
    }
    float avgCounts = (float)sum / samples;
    return (avgCounts / ADC_MAX_COUNTS) * ADC_VREF_V;
}

void currentSenseInit() {
    // The pump is guaranteed off in setup() before this call - use that window
    // to measure the real ACS712 zero offset (real modules are not exactly Vcc/2).
    s_zeroOffsetV = readAdcVoltageAveraged(200);
}

int16_t readAveragedCurrentMa() {
    float voltage = readAdcVoltageAveraged(SAMPLE_COUNT);
    float amps = (voltage - s_zeroOffsetV) * 1000.0f / ACS712_MV_PER_AMP;
    return (int16_t)(amps * 1000.0f);
}

int16_t estimateEffectiveCurrentMa(int16_t measuredMa, float duty01) {
    if (duty01 <= 0.01f) {
        return 0;
    }
    return (int16_t)((float)measuredMa / duty01);
}
