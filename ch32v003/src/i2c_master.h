#pragma once
#include <stdbool.h>
#include <stdint.h>

void i2c_master_init(void);

// Blocking write of len bytes to addr7 (7-bit, without the R/W bit).
// Returns false on timeout/NACK - the caller must survive this
// (e.g. just skip the LCD update) rather than hang.
bool i2c_master_write(uint8_t addr7, const uint8_t *data, uint16_t len);
