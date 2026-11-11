// CH32V003F4P6 (TSSOP-20) pinout for the stand. Physical pin numbers and
// rationale - see ../../ch32v003-pin-check/README.md.
//
// NEVER configure here: PD1 (pin 18, SWIO), PD7 (pin 4, NRST).
#pragma once

#include <ch32v00x_gpio.h>

// I2C1 (LCD) - fixed peripheral pins, no remap.
#define I2C_GPIO_PORT   GPIOC
#define I2C_SCL_PIN     GPIO_Pin_2 // pin 12
#define I2C_SDA_PIN     GPIO_Pin_1 // pin 11

// PWM to the MOSFET gate - TIM1 CH4.
#define PWM_GPIO_PORT   GPIOC
#define PWM_PIN         GPIO_Pin_4 // pin 14

// ADC from ACS712 OUT (through an RC filter).
#define CURRENT_ADC_GPIO_PORT  GPIOA
#define CURRENT_ADC_PIN        GPIO_Pin_1 // pin 5
#define CURRENT_ADC_CHANNEL    ADC_Channel_1

// Encoder. All three pins are now on PD4/PD5/PD6 (pins 1-3) - physically
// in one corner of the package next to NRST (pin 4), easy to route. They used
// to be spread over PC0/PC3/PD0 - nothing besides pins.h had to change,
// CLK/DT/SW are read the same regardless of the physical pin.
//
// PD5/PD6 are also the default UART1 TX/RX pins; if a serial log is needed
// later, the USART can be remapped to other pins via AFIO - they are not
// taken for good, but by default only these are available.
#define ENC_GPIO_PORT   GPIOD
#define ENC_CLK_PIN     GPIO_Pin_6 // pin 3

#define ENC_DT_GPIO_PORT GPIOD
#define ENC_DT_PIN       GPIO_Pin_5 // pin 2

#define ENC_SW_GPIO_PORT GPIOD
#define ENC_SW_PIN       GPIO_Pin_4 // pin 1

// Start/Stop button.
#define START_STOP_GPIO_PORT GPIOD
#define START_STOP_PIN       GPIO_Pin_2 // pin 19
