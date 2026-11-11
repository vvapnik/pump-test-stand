#pragma once

// PWM to the MOSFET gate (Timer1/OC1A - analogWrite supports D9 directly)
constexpr uint8_t PIN_PUMP_PWM = 9;

// ACS712 analog output (through an RC filter)
constexpr uint8_t PIN_CURRENT_SENSE = A0;

// Encoder
constexpr uint8_t PIN_ENC_CLK = 2; // INT0
constexpr uint8_t PIN_ENC_DT = 4;
constexpr uint8_t PIN_ENC_SW = 5;

// Separate start/stop button
constexpr uint8_t PIN_START_STOP = 6;

// LCD (Freenove I2C IIC LCD1602, PCF8574 controller) - default address.
// If the module does not respond, try 0x3F (the other common PCF8574 address).
constexpr uint8_t LCD_I2C_ADDR = 0x27;
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
