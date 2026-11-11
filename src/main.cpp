#include <Arduino.h>
#include <EEPROM.h>

#include "pins.h"
#include "encoder.h"
#include "current_sense.h"
#include "pump_state.h"
#include "display.h"

// --- Adjustable limits and step sizes -------------------------------------
constexpr uint8_t DUTY_STEP_PERCENT = 5;
constexpr int16_t MAX_CURRENT_STEP_MA = 50;
constexpr int16_t MAX_CURRENT_CEILING_MA = 2000; // sane ceiling for a 12V pump
constexpr int16_t MAX_CURRENT_FLOOR_MA = 100;

constexpr unsigned long DISPLAY_PERIOD_MS = 150;
constexpr unsigned long SERIAL_LOG_PERIOD_MS = 200;

// --- Settings persisted in EEPROM ------------------------------------------
struct PersistedSettings {
    uint8_t magic;
    uint8_t dutyPercent;
    int16_t maxCurrentMa;
};
constexpr uint8_t SETTINGS_MAGIC = 0xA5;
constexpr int EEPROM_ADDR = 0;

static PersistedSettings s_settings;
static EncoderTarget s_activeTarget = EncoderTarget::Duty;
static bool s_running = false;
static bool s_overCurrentLatched = false;

static unsigned long s_lastDisplayMs = 0;
static unsigned long s_lastSerialMs = 0;

static void loadSettings() {
    EEPROM.get(EEPROM_ADDR, s_settings);
    if (s_settings.magic != SETTINGS_MAGIC) {
        s_settings.magic = SETTINGS_MAGIC;
        s_settings.dutyPercent = 50;
        s_settings.maxCurrentMa = 800;
        EEPROM.put(EEPROM_ADDR, s_settings);
    }
}

static void saveSettings() {
    // EEPROM.put does an "update" - it only writes bytes that changed.
    EEPROM.put(EEPROM_ADDR, s_settings);
}

static void applyEncoderDelta() {
    int delta = encoderTakeDelta();
    if (delta == 0) {
        return;
    }

    if (s_activeTarget == EncoderTarget::Duty) {
        int newDuty = (int)s_settings.dutyPercent + delta * DUTY_STEP_PERCENT;
        newDuty = constrain(newDuty, 0, 100);
        s_settings.dutyPercent = (uint8_t)newDuty;
    } else {
        int newLimit = (int)s_settings.maxCurrentMa + delta * MAX_CURRENT_STEP_MA;
        newLimit = constrain(newLimit, MAX_CURRENT_FLOOR_MA, MAX_CURRENT_CEILING_MA);
        s_settings.maxCurrentMa = (int16_t)newLimit;
    }
    saveSettings();
}

static void applyEncoderClick() {
    if (encoderTakeClick()) {
        s_activeTarget = (s_activeTarget == EncoderTarget::Duty) ? EncoderTarget::MaxCurrent
                                                                   : EncoderTarget::Duty;
    }
}

static void applyStartStop() {
    if (!startStopTakePress()) {
        return;
    }
    if (s_overCurrentLatched) {
        // The first press after a trip only clears the latch; the pump does not
        // start by itself - press again.
        s_overCurrentLatched = false;
        s_running = false;
    } else {
        s_running = !s_running;
    }
}

void setup() {
    Serial.begin(9600); // 9600 is reliable even on the internal RC oscillator (see platformio.ini)

    pinMode(PIN_PUMP_PWM, OUTPUT);
    analogWrite(PIN_PUMP_PWM, 0); // pump must be off before zero-current calibration

    encoderInit();
    displayInit();
    loadSettings();

    // The pump is known to be stopped (analogWrite(0) above) - calibrate the
    // ACS712 zero against this module's real offset.
    currentSenseInit();
}

void loop() {
    applyEncoderDelta();
    applyEncoderClick();
    applyStartStop();

    float dutyFraction = s_running ? (s_settings.dutyPercent / 100.0f) : 0.0f;
    analogWrite(PIN_PUMP_PWM, s_running ? map(s_settings.dutyPercent, 0, 100, 0, 255) : 0);

    int16_t measuredMa = readAveragedCurrentMa();
    int16_t effectiveMa = estimateEffectiveCurrentMa(measuredMa, dutyFraction);

    if (s_running && !s_overCurrentLatched && measuredMa > s_settings.maxCurrentMa) {
        s_running = false;
        s_overCurrentLatched = true;
        analogWrite(PIN_PUMP_PWM, 0);
    }

    PumpClassification classification;
    if (s_overCurrentLatched) {
        classification = {PumpState::OverCurrent, 0};
    } else {
        classification = classifyPump(effectiveMa, s_settings.maxCurrentMa);
    }

    unsigned long now = millis();
    if (now - s_lastDisplayMs >= DISPLAY_PERIOD_MS) {
        s_lastDisplayMs = now;
        displayUpdate(classification.state, measuredMa, s_settings.dutyPercent,
                       classification.pressureUnits, s_activeTarget, s_settings.maxCurrentMa);
    }

    if (now - s_lastSerialMs >= SERIAL_LOG_PERIOD_MS) {
        s_lastSerialMs = now;
        Serial.print(F("duty="));
        Serial.print(s_settings.dutyPercent);
        Serial.print(F(" limit="));
        Serial.print(s_settings.maxCurrentMa);
        Serial.print(F(" measuredMa="));
        Serial.print(measuredMa);
        Serial.print(F(" effectiveMa="));
        Serial.print(effectiveMa);
        Serial.print(F(" pressure="));
        Serial.print(classification.pressureUnits);
        Serial.print(F(" state="));
        Serial.println((int)classification.state);
    }
}
