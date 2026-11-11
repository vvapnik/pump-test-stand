// 12V pump test stand - CH32V003F4P6 firmware.
//
// Differences from the ATmega328 version (simplifications for a first run on a new MCU):
//  - settings are not persisted across power cycles (flash storage is not
//    implemented) - Duty/MaxCurrent/RunTime reset to defaults on every start;
//  - the encoder and buttons are polled on a timer (10 ms), not interrupt-driven;
//  - no serial log (on the CH32V003 that needs separate UART setup, not
//    wired up yet).
// Pins - see pins.h and ../../ch32v003-pin-check/README.md.
#include "current_sense.h"
#include "encoder.h"
#include "i2c_master.h"
#include "lcd1602.h"
#include "pump_state.h"
#include "pwm.h"

#include <debug.h>
#include <stdio.h>

#define TICK_MS 10
#define DISPLAY_PERIOD_TICKS 15 // ~150 ms

#define DUTY_STEP_PERCENT 5
#define MAX_CURRENT_STEP_MA 50
#define MAX_CURRENT_FLOOR_MA 100
#define MAX_CURRENT_CEILING_MA 2000

#define RUN_TIME_STEP_S 5
#define RUN_TIME_MAX_S 600 // 10 minutes; 0 = timer off, runs until stopped manually

// While the motor spins up, the inrush current is well above the running current -
// overcurrent protection is disabled for that time, otherwise the motor would
// never start. Caveat: a real short circuit during these ~0.5 s is not caught
// either - a deliberate trade-off, the stand is tested under supervision.
#define STARTUP_GRACE_MS 500
#define STARTUP_GRACE_TICKS (STARTUP_GRACE_MS / TICK_MS)

typedef enum {
    MODE_SELECT, // turning moves the cursor Duty->MaxCurrent->RunTime, value unchanged
    MODE_EDIT,   // turning changes the selected parameter's value
} EncoderMode;

static uint8_t s_dutyPercent = 50;
static int16_t s_maxCurrentMa = 800;
static uint16_t s_runTimeSeconds = 0; // 0 = timed auto-stop disabled
static EncoderTarget s_activeTarget = TARGET_DUTY;
static EncoderTarget s_secondaryTarget = TARGET_MAX_CURRENT; // what to show in the 2nd slot of line 2 while Duty is selected
static EncoderMode s_encoderMode = MODE_SELECT;
static bool s_running = false;
static bool s_overCurrentLatched = false;
static bool s_lcdPresent = false;
static uint32_t s_startupGraceTicksLeft = 0;
static uint32_t s_runElapsedTicks = 0;

static int16_t clampI16(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static const char *stateLabel(PumpState state) {
    switch (state) {
        case PUMP_STOPPED: return "STOP";
        case PUMP_AIR: return "AIR ";
        case PUMP_WATER: return "H2O ";
        case PUMP_OVERCURRENT: return "OVER";
    }
    return "????";
}

static void applyEncoder(void) {
    int delta = encoder_take_delta();
    if (delta != 0) {
        if (s_encoderMode == MODE_SELECT) {
            switch (s_activeTarget) {
                case TARGET_DUTY: s_activeTarget = TARGET_MAX_CURRENT; break;
                case TARGET_MAX_CURRENT: s_activeTarget = TARGET_RUN_TIME; break;
                default: s_activeTarget = TARGET_DUTY; break;
            }
            if (s_activeTarget != TARGET_DUTY) {
                s_secondaryTarget = s_activeTarget;
            }
        } else if (s_activeTarget == TARGET_DUTY) {
            int v = (int)s_dutyPercent + delta * DUTY_STEP_PERCENT;
            if (v < 0) v = 0;
            if (v > 100) v = 100;
            s_dutyPercent = (uint8_t)v;
        } else if (s_activeTarget == TARGET_MAX_CURRENT) {
            int v = (int)s_maxCurrentMa + delta * MAX_CURRENT_STEP_MA;
            s_maxCurrentMa = clampI16((int16_t)v, MAX_CURRENT_FLOOR_MA, MAX_CURRENT_CEILING_MA);
        } else { // TARGET_RUN_TIME
            int v = (int)s_runTimeSeconds + delta * RUN_TIME_STEP_S;
            if (v < 0) v = 0;
            if (v > RUN_TIME_MAX_S) v = RUN_TIME_MAX_S;
            s_runTimeSeconds = (uint16_t)v;
        }
    }

    if (encoder_take_click()) {
        // 1st click on a parameter -> edit, 2nd -> commit the value
        s_encoderMode = (s_encoderMode == MODE_SELECT) ? MODE_EDIT : MODE_SELECT;
    }

    if (start_stop_take_press()) {
        if (s_overCurrentLatched) {
            s_overCurrentLatched = false;
            s_running = false;
        } else {
            s_running = !s_running;
            if (s_running) {
                s_startupGraceTicksLeft = STARTUP_GRACE_TICKS;
                s_runElapsedTicks = 0;
            }
        }
    }
}

static void updateDisplay(PumpState state, int16_t measuredMa, uint8_t pressureUnits) {
    if (!s_lcdPresent) {
        return;
    }

    char line1[20];
    if (state == PUMP_WATER) {
        snprintf(line1, sizeof(line1), "%sI:%4dmA%3u", stateLabel(state), measuredMa, pressureUnits);
    } else {
        snprintf(line1, sizeof(line1), "%sI:%4dmA", stateLabel(state), measuredMa);
    }

    // '>' - cursor on a parameter (selection; turning does not change the value);
    // '*' - being edited (turning changes the value).
    char dutyMarker = ' ';
    char secondaryMarker = ' ';
    char mark = (s_encoderMode == MODE_EDIT) ? '*' : '>';
    if (s_activeTarget == TARGET_DUTY) {
        dutyMarker = mark;
    } else {
        secondaryMarker = mark; // either MaxCurrent or RunTime is active - both share one slot
    }

    char secondary[12];
    if (s_secondaryTarget == TARGET_RUN_TIME) {
        if (s_runTimeSeconds == 0) {
            snprintf(secondary, sizeof(secondary), "T%cOFF", secondaryMarker);
        } else {
            snprintf(secondary, sizeof(secondary), "T%c%3us", secondaryMarker, s_runTimeSeconds);
        }
    } else {
        snprintf(secondary, sizeof(secondary), "M%c%4dmA", secondaryMarker, s_maxCurrentMa);
    }

    char line2[20];
    snprintf(line2, sizeof(line2), "D%c%3u%%%s", dutyMarker, s_dutyPercent, secondary);

    lcd_set_cursor(0, 0);
    lcd_print("                "); // clear the line without extra flicker
    lcd_set_cursor(0, 0);
    lcd_print(line1);
    lcd_set_cursor(0, 1);
    lcd_print("                ");
    lcd_set_cursor(0, 1);
    lcd_print(line2);
}

int main(void) {
    Delay_Init();

    pwm_init();
    pwm_set_duty_percent(0); // pump off until zero-current calibration

    encoder_init();

    i2c_master_init();
    s_lcdPresent = lcd_init();
    if (s_lcdPresent) {
        lcd_set_cursor(0, 0);
        lcd_print("Pump test stand");
        lcd_set_cursor(0, 1);
        lcd_print("CH32V003 init...");
    }

    current_sense_init(); // pump is guaranteed off (see above)

    uint32_t tick = 0;
    while (1) {
        encoder_poll_tick();
        applyEncoder();

        uint8_t activeDuty = s_running ? s_dutyPercent : 0;
        pwm_set_duty_percent(activeDuty);

        int16_t measuredMa = current_sense_read_ma();
        int16_t effectiveMa = 0;
        if (activeDuty > 0) {
            effectiveMa = (int16_t)(((int32_t)measuredMa * 100) / activeDuty);
        }

        if (s_running) {
            if (s_startupGraceTicksLeft > 0) {
                s_startupGraceTicksLeft--; // soft start - don't let overcurrent protection interfere with spin-up yet
            } else if (!s_overCurrentLatched && measuredMa > s_maxCurrentMa) {
                s_running = false;
                s_overCurrentLatched = true;
                pwm_set_duty_percent(0);
            }
        }

        if (s_running) { // still running (overcurrent protection above did not trip)
            s_runElapsedTicks++;
            uint32_t elapsedS = (s_runElapsedTicks * TICK_MS) / 1000;
            if (s_runTimeSeconds > 0 && elapsedS >= s_runTimeSeconds) {
                s_running = false; // timed auto-stop - a normal stop, no latch
                pwm_set_duty_percent(0);
            }
        }

        PumpClassification classification;
        if (s_overCurrentLatched) {
            classification.state = PUMP_OVERCURRENT;
            classification.pressureUnits = 0;
        } else {
            classification = classify_pump(effectiveMa, s_maxCurrentMa);
        }

        if (tick % DISPLAY_PERIOD_TICKS == 0) {
            updateDisplay(classification.state, measuredMa, classification.pressureUnits);
        }

        tick++;
        Delay_Ms(TICK_MS);
    }
}
