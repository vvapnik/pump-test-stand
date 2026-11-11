#pragma once
#include <stdbool.h>
#include <stdint.h>

// Freenove I2C LCD1602 (PCF8574, usually address 0x27; if it does not respond,
// try 0x3F, the other common PCF8574 address).
#define LCD_I2C_ADDR 0x27

// Returns false if the module did not answer on I2C (not connected / other
// address) - the caller can then skip further lcd_* calls.
bool lcd_init(void);

void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *s);
