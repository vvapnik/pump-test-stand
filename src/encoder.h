#pragma once
#include <Arduino.h>

// What the encoder currently adjusts.
enum class EncoderTarget : uint8_t {
    Duty = 0,
    MaxCurrent = 1,
};

void encoderInit();

// Takes the encoder detents accumulated since the last call (+1/-1 per detent).
// Call from loop(), not from an ISR.
int encoderTakeDelta();

// true once per encoder button click (already debounced).
bool encoderTakeClick();

// Start/stop button: true once per press (debounced).
bool startStopTakePress();
