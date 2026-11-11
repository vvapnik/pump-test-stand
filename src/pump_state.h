#pragma once
#include <Arduino.h>

enum class PumpState : uint8_t {
    Stopped,
    Air,
    Water,
    OverCurrent,
};

// --- Classification thresholds by effective current (mA) ------------------
// These are EMPIRICAL values - calibrate them on the real stand with your
// specific pump (see README.md, "Calibration"). The values below are only a
// starting point for the first power-up.
//
//   0 .. CURRENT_IDLE_MAX_MA        -> no current / motor not running
//   CURRENT_IDLE_MAX_MA .. AIR_MAX  -> pumping air (light load)
//   CURRENT_AIR_MAX_MA .. MaxCurrent-> pumping water; pressure (0..100 units)
//                                      linear from WATER_MIN to MaxCurrent
constexpr int16_t CURRENT_IDLE_MAX_MA = 30;
constexpr int16_t CURRENT_AIR_MAX_MA = 150;
constexpr int16_t CURRENT_WATER_MIN_MA = 150;

struct PumpClassification {
    PumpState state;
    uint8_t pressureUnits; // 0..100, meaningful only when state == Water
};

// maxCurrentMa - the current user overload threshold (the same value that is
// set with the encoder and used for auto shutoff).
PumpClassification classifyPump(int16_t effectiveCurrentMa, int16_t maxCurrentMa);
