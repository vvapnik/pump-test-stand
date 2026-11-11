#pragma once
#include <Arduino.h>
#include "pump_state.h"
#include "encoder.h"

void displayInit();

void displayUpdate(PumpState state, int16_t measuredCurrentMa, uint8_t dutyPercent,
                    uint8_t pressureUnits, EncoderTarget activeTarget, int16_t maxCurrentMa);
