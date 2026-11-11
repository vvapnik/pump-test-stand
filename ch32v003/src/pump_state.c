#include "pump_state.h"

static int16_t clampI16(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

PumpClassification classify_pump(int16_t effectiveCurrentMa, int16_t maxCurrentMa) {
    if (effectiveCurrentMa <= CURRENT_IDLE_MAX_MA) {
        PumpClassification r = {PUMP_STOPPED, 0};
        return r;
    }
    if (effectiveCurrentMa <= CURRENT_AIR_MAX_MA) {
        PumpClassification r = {PUMP_AIR, 0};
        return r;
    }

    int16_t span = (int16_t)(maxCurrentMa - CURRENT_WATER_MIN_MA);
    uint8_t pressure = 0;
    if (span > 0) {
        int16_t clamped = clampI16(effectiveCurrentMa, CURRENT_WATER_MIN_MA, maxCurrentMa);
        pressure = (uint8_t)(((int32_t)(clamped - CURRENT_WATER_MIN_MA) * 100) / span);
    }
    PumpClassification r = {PUMP_WATER, pressure};
    return r;
}
