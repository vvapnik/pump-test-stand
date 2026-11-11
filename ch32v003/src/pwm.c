// PWM to the MOSFET gate via TIM1 CH4 (PC4). ~1 kHz, duty 0..100%.
#include "pwm.h"
#include "pins.h"

#include <ch32v00x_gpio.h>
#include <ch32v00x_rcc.h>
#include <ch32v00x_tim.h>

#define PWM_PERIOD 99 // 100 steps -> duty equals "percent" directly

void pwm_init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin = PWM_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(PWM_GPIO_PORT, &gpio);

    // SystemCoreClock (48 MHz) / 480 / 100 steps ~= 1 kHz PWM.
    TIM_TimeBaseInitTypeDef timeBase = {0};
    timeBase.TIM_Prescaler = 479;
    timeBase.TIM_CounterMode = TIM_CounterMode_Up;
    timeBase.TIM_Period = PWM_PERIOD;
    timeBase.TIM_ClockDivision = TIM_CKD_DIV1;
    timeBase.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &timeBase);

    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OutputNState = TIM_OutputNState_Disable;
    oc.TIM_Pulse = 0;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    oc.TIM_OCNPolarity = TIM_OCNPolarity_High;
    oc.TIM_OCIdleState = TIM_OCIdleState_Reset;
    oc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC4Init(TIM1, &oc);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_CtrlPWMOutputs(TIM1, ENABLE); // required for TIM1 (advanced timer)
    TIM_Cmd(TIM1, ENABLE);
}

void pwm_set_duty_percent(uint8_t duty) {
    if (duty > 100) {
        duty = 100;
    }
    TIM1->CH4CVR = duty;
}
