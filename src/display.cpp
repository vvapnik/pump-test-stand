#include "display.h"
#include "pins.h"
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdio.h>

static LiquidCrystal_I2C s_lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// The standard HD44780 font has no Cyrillic -> short Latin labels.
static const char *stateLabel(PumpState state) {
    switch (state) {
        case PumpState::Stopped: return "STOP";
        case PumpState::Air: return "AIR ";
        case PumpState::Water: return "H2O ";
        case PumpState::OverCurrent: return "OVER";
    }
    return "????";
}

static void printPaddedRow(uint8_t row, const char *text) {
    char buf[LCD_COLS + 1];
    uint8_t len = strlen(text);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        buf[i] = (i < len) ? text[i] : ' ';
    }
    buf[LCD_COLS] = '\0';
    s_lcd.setCursor(0, row);
    s_lcd.print(buf);
}

void displayInit() {
    s_lcd.init();
    s_lcd.backlight();
    printPaddedRow(0, "Pump test stand");
    printPaddedRow(1, "init...");
}

void displayUpdate(PumpState state, int16_t measuredCurrentMa, uint8_t dutyPercent,
                    uint8_t pressureUnits, EncoderTarget activeTarget, int16_t maxCurrentMa) {
    char line1[LCD_COLS + 1];
    if (state == PumpState::Water) {
        snprintf(line1, sizeof(line1), "%sI:%4dmA%4d", stateLabel(state), measuredCurrentMa, pressureUnits);
    } else {
        snprintf(line1, sizeof(line1), "%sI:%4dmA", stateLabel(state), measuredCurrentMa);
    }

    char dutyMarker = (activeTarget == EncoderTarget::Duty) ? '*' : ' ';
    char curMarker = (activeTarget == EncoderTarget::MaxCurrent) ? '*' : ' ';
    char line2[LCD_COLS + 1];
    snprintf(line2, sizeof(line2), "D%c%3d%%M%c%4dmA", dutyMarker, dutyPercent, curMarker, maxCurrentMa);

    printPaddedRow(0, line1);
    printPaddedRow(1, line2);
}
