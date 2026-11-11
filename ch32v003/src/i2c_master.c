#include "i2c_master.h"
#include "pins.h"

#include <ch32v00x_gpio.h>
#include <ch32v00x_i2c.h>
#include <ch32v00x_rcc.h>

#define I2C_WAIT_LIMIT 20000u

static bool waitFlag(uint32_t event) {
    uint32_t guard = I2C_WAIT_LIMIT;
    while (I2C_CheckEvent(I2C1, event) != READY) {
        if (--guard == 0) {
            return false;
        }
    }
    return true;
}

static bool waitTxe(void) {
    uint32_t guard = I2C_WAIT_LIMIT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET) {
        if (--guard == 0) {
            return false;
        }
    }
    return true;
}

static void recoverBus(void) {
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, DISABLE);
}

void i2c_master_init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD; // I2C is open-drain; pull-ups are on the LCD module
    gpio.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(I2C_GPIO_PORT, &gpio);

    I2C_InitTypeDef i2c = {0};
    i2c.I2C_ClockSpeed = 100000; // 100 kHz - reliable for PCF8574 modules
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i2c);

    I2C_Cmd(I2C1, ENABLE);
}

bool i2c_master_write(uint8_t addr7, const uint8_t *data, uint16_t len) {
    uint32_t guard = I2C_WAIT_LIMIT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET) {
        if (--guard == 0) {
            recoverBus();
            return false;
        }
    }

    I2C_GenerateSTART(I2C1, ENABLE);
    if (!waitFlag(I2C_EVENT_MASTER_MODE_SELECT)) {
        recoverBus();
        return false;
    }

    I2C_Send7bitAddress(I2C1, (uint8_t)(addr7 << 1), I2C_Direction_Transmitter);
    if (!waitFlag(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        recoverBus();
        return false;
    }

    for (uint16_t i = 0; i < len; i++) {
        I2C_SendData(I2C1, data[i]);
        if (!waitTxe()) {
            recoverBus();
            return false;
        }
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
    return true;
}
