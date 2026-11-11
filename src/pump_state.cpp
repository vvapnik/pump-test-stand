#include "pump_state.h"

PumpClassification classifyPump(int16_t effectiveCurrentMa, int16_t maxCurrentMa) {
    if (effectiveCurrentMa <= CURRENT_IDLE_MAX_MA) {
        return {PumpState::Stopped, 0};
    }
    if (effectiveCurrentMa <= CURRENT_AIR_MAX_MA) {
        return {PumpState::Air, 0};
    }

    int16_t span = maxCurrentMa - CURRENT_WATER_MIN_MA;
    uint8_t pressure = 0;
    if (span > 0) {
        long clamped = constrain(effectiveCurrentMa, CURRENT_WATER_MIN_MA, maxCurrentMa);
        pressure = (uint8_t)map(clamped, CURRENT_WATER_MIN_MA, maxCurrentMa, 0, 100);
    }
    return {PumpState::Water, pressure};
}
