// The encoder and buttons are polled once per tick (see TICK_MS in main.c), not
// interrupt-driven - simpler and reliable enough for turning the encoder by hand.
#include "encoder.h"
#include "pins.h"

#include <ch32v00x_gpio.h>
#include <ch32v00x_rcc.h>

#define DEBOUNCE_TICKS 3 // ~30 ms at TICK_MS=10

static volatile int s_delta = 0;
static uint8_t s_lastClk = 1;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t stableLevel; // 1 = released (pull-up), 0 = pressed
    uint8_t stableCount;
    bool pendingEdge;
} DebouncedButton;

static DebouncedButton s_encSw = {ENC_SW_GPIO_PORT, ENC_SW_PIN, 1, 0, false};
static DebouncedButton s_startStop = {START_STOP_GPIO_PORT, START_STOP_PIN, 1, 0, false};

static void pollButton(DebouncedButton *b) {
    uint8_t raw = GPIO_ReadInputDataBit(b->port, b->pin);
    if (raw == b->stableLevel) {
        b->stableCount = 0;
        return;
    }
    if (++b->stableCount >= DEBOUNCE_TICKS) {
        b->stableLevel = raw;
        b->stableCount = 0;
        if (raw == 0) { // transition to pressed
            b->pendingEdge = true;
        }
    }
}

void encoder_init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_10MHz;

    gpio.GPIO_Pin = ENC_CLK_PIN | ENC_SW_PIN;
    GPIO_Init(ENC_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = ENC_DT_PIN;
    GPIO_Init(ENC_DT_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = START_STOP_PIN;
    GPIO_Init(START_STOP_GPIO_PORT, &gpio);

    s_lastClk = GPIO_ReadInputDataBit(ENC_GPIO_PORT, ENC_CLK_PIN);
}

void encoder_poll_tick(void) {
    uint8_t clk = GPIO_ReadInputDataBit(ENC_GPIO_PORT, ENC_CLK_PIN);
    if (s_lastClk == 1 && clk == 0) { // falling edge of CLK
        uint8_t dt = GPIO_ReadInputDataBit(ENC_DT_GPIO_PORT, ENC_DT_PIN);
        s_delta += (dt == 1) ? 1 : -1;
    }
    s_lastClk = clk;

    pollButton(&s_encSw);
    pollButton(&s_startStop);
}

int encoder_take_delta(void) {
    int d = s_delta;
    s_delta = 0;
    return d;
}

bool encoder_take_click(void) {
    if (s_encSw.pendingEdge) {
        s_encSw.pendingEdge = false;
        return true;
    }
    return false;
}

bool start_stop_take_press(void) {
    if (s_startStop.pendingEdge) {
        s_startStop.pendingEdge = false;
        return true;
    }
    return false;
}
