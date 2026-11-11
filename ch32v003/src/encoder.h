#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TARGET_DUTY,
    TARGET_MAX_CURRENT,
    TARGET_RUN_TIME,
} EncoderTarget;

void encoder_init(void);

// Call once per poll tick (see TICK_MS in main.c), not from an interrupt -
// there are no interrupts here: the encoder and buttons are polled, not edge-triggered.
void encoder_poll_tick(void);

int encoder_take_delta(void);
bool encoder_take_click(void);
bool start_stop_take_press(void);
