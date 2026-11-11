// HD44780 LCD1602 via PCF8574 (4-bit bus), standard "I2C LCD backpack"
// wiring: bit0=RS, bit1=RW (unused, always 0),
// bit2=E, bit3=backlight, bits4-7=D4..D7.
#include "lcd1602.h"
#include "i2c_master.h"
#include <debug.h>

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_RS 0x01

static bool s_present = false;

static bool sendNibble(uint8_t rs, uint8_t nibble) {
    uint8_t base = (uint8_t)((nibble << 4) | LCD_BACKLIGHT | (rs ? LCD_RS : 0));
    uint8_t withEnable = base | LCD_ENABLE;
    if (!i2c_master_write(LCD_I2C_ADDR, &withEnable, 1)) {
        return false;
    }
    return i2c_master_write(LCD_I2C_ADDR, &base, 1);
}

static bool sendByte(uint8_t rs, uint8_t value) {
    bool ok = sendNibble(rs, (uint8_t)(value >> 4));
    ok = sendNibble(rs, (uint8_t)(value & 0x0F)) && ok;
    return ok;
}

static bool command(uint8_t value) {
    return sendByte(0, value);
}

static bool data(uint8_t value) {
    return sendByte(1, value);
}

bool lcd_init(void) {
    Delay_Ms(50); // give the module itself time to power up after 5V is applied

    // Classic HD44780 reset into 4-bit mode.
    s_present = sendNibble(0, 0x03);
    Delay_Ms(5);
    sendNibble(0, 0x03);
    Delay_Ms(1);
    sendNibble(0, 0x03);
    Delay_Ms(1);
    sendNibble(0, 0x02); // switch to the 4-bit bus
    Delay_Ms(1);

    command(0x28); // 4 bits, 2 lines, 5x8 font
    command(0x0C); // display on, cursor/blink off
    command(0x06); // cursor auto-increment
    command(0x01); // clear the screen
    Delay_Ms(2);

    return s_present;
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t addr = (uint8_t)(col + (row == 0 ? 0x00 : 0x40));
    command((uint8_t)(0x80 | addr));
}

void lcd_print(const char *s) {
    while (*s) {
        data((uint8_t)*s++);
    }
}
