// CH32V003F4P6 pin check before building the stand on it.
//
// Configures as push-pull outputs EXACTLY the pins the new CH32V003 version
// of the stand needs (see this folder's README.md for the pin table) and blinks
// them all together every ~500 ms. If the firmware flashed and they blink,
// the VDD/VSS/NRST/SWIO wiring and the link to the chip are fine; then check
// each pin one by one with a multimeter/LED right at the package leads.
//
// Deliberately does NOT touch:
//   PD1 (pin 18, SWIO) - single-wire debug/programming; breaking this pin's
//                         mode at the very least loses contact with the
//                         programmer until a forced recovery.
//   PD7 (pin 4,  NRST) - hardware reset.
#include <ch32v00x_gpio.h>
#include <ch32v00x_rcc.h>

static void busy_delay(void) {
    for (volatile uint32_t i = 0; i < 800000; i++) {
    }
}

int main(void) {
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
        ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_10MHz;

    // PA1 (pin 5) - future ADC input from the ACS712; checked here as a plain GPIO.
    gpio.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &gpio);

    // PC0(10) encoder CLK, PC1(11) future I2C SDA, PC2(12) future I2C SCL,
    // PC3(13) encoder SW, PC4(14) future PWM to the MOSFET gate.
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_Init(GPIOC, &gpio);

    // PD0(8) encoder DT, PD2(19) start/stop button.
    // PD1 (18, SWIO) and PD7 (4, NRST) are deliberately left out.
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2;
    GPIO_Init(GPIOD, &gpio);

    while (1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_1);
        GPIO_SetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4);
        GPIO_SetBits(GPIOD, GPIO_Pin_0 | GPIO_Pin_2);
        busy_delay();

        GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        GPIO_ResetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4);
        GPIO_ResetBits(GPIOD, GPIO_Pin_0 | GPIO_Pin_2);
        busy_delay();
    }
}
