#pragma once
#include <stdint.h>

typedef enum {
    PUMP_STOPPED,
    PUMP_AIR,
    PUMP_WATER,
    PUMP_OVERCURRENT,
} PumpState;

typedef struct {
    PumpState state;
    uint8_t pressureUnits; // 0..100, meaningful only when state == PUMP_WATER
} PumpClassification;

// Starting thresholds - calibrate on the real stand, see the project README.
#define CURRENT_IDLE_MAX_MA 30
#define CURRENT_AIR_MAX_MA 150
#define CURRENT_WATER_MIN_MA 150

PumpClassification classify_pump(int16_t effectiveCurrentMa, int16_t maxCurrentMa);
