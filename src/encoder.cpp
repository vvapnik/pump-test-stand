#include "encoder.h"
#include "pins.h"

// Encoder detent accumulator: written only by the ISR, read/reset only by loop().
static volatile int8_t s_encDelta = 0;

// Classic approach: interrupt on the CLK edge, read the DT level inside the ISR
// to get the direction. No second hardware interrupt on DT is needed - the
// ATmega328 has external interrupts only on D2/D3.
static void encoderIsr() {
    delayMicroseconds(2); // let the signals settle before reading DT
    if (digitalRead(PIN_ENC_DT) != digitalRead(PIN_ENC_CLK)) {
        s_encDelta++;
    } else {
        s_encDelta--;
    }
}

// Debounce for the two buttons (encoder click, start/stop) polled in loop().
struct DebouncedButton {
    uint8_t pin;
    bool lastRaw = true; // INPUT_PULLUP => idle level is HIGH
    bool stableState = true;
    unsigned long lastChangeMs = 0;
    bool pendingPress = false;
};

static DebouncedButton s_encButton{PIN_ENC_SW};
static DebouncedButton s_startStopButton{PIN_START_STOP};

static constexpr unsigned long DEBOUNCE_MS = 25;

static void pollButton(DebouncedButton &b) {
    bool raw = digitalRead(b.pin);
    unsigned long now = millis();

    if (raw != b.lastRaw) {
        b.lastChangeMs = now;
        b.lastRaw = raw;
    }

    if ((now - b.lastChangeMs) > DEBOUNCE_MS && b.stableState != raw) {
        b.stableState = raw;
        if (b.stableState == LOW) { // transition to pressed
            b.pendingPress = true;
        }
    }
}

void encoderInit() {
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    pinMode(PIN_START_STOP, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderIsr, FALLING);
}

int encoderTakeDelta() {
    noInterrupts();
    int delta = s_encDelta;
    s_encDelta = 0;
    interrupts();
    return delta;
}

bool encoderTakeClick() {
    pollButton(s_encButton);
    if (s_encButton.pendingPress) {
        s_encButton.pendingPress = false;
        return true;
    }
    return false;
}

bool startStopTakePress() {
    pollButton(s_startStopButton);
    if (s_startStopButton.pendingPress) {
        s_startStopButton.pendingPress = false;
        return true;
    }
    return false;
}
