// Integer arithmetic everywhere: the CH32V003 (RV32EC) has no hardware FPU,
// and software float emulation is costly on 16 KB of flash (~3-4 KB).
#include "current_sense.h"
#include "pins.h"

#include <ch32v00x_adc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_rcc.h>
#include <debug.h>

#define ADC_MAX_COUNTS 1023 // 10 bits
#define ADC_VREF_MV 5000    // stand VDD = 5V (see README)
#define ACS712_MV_PER_AMP 100
#define SAMPLE_COUNT 32
#define ADC_WAIT_LIMIT 20000u

static int32_t s_zeroOffsetMv = ADC_VREF_MV / 2;

static void adcInit(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div2); // 48 MHz/2 = 24 MHz, within the ADC spec

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin = CURRENT_ADC_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(CURRENT_ADC_GPIO_PORT, &gpio);

    ADC_InitTypeDef adc = {0};
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    uint32_t guard = ADC_WAIT_LIMIT;
    while (ADC_GetResetCalibrationStatus(ADC1) == SET && --guard) {
    }
    ADC_StartCalibration(ADC1);
    guard = ADC_WAIT_LIMIT;
    while (ADC_GetCalibrationStatus(ADC1) == SET && --guard) {
    }
}

static uint16_t adcReadRaw(void) {
    ADC_RegularChannelConfig(ADC1, CURRENT_ADC_CHANNEL, 1, ADC_SampleTime_241Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    uint32_t guard = ADC_WAIT_LIMIT;
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET && --guard) {
    }
    return ADC_GetConversionValue(ADC1);
}

// Mean ADC count over `samples` readings, converted to mV (integer).
static int32_t readAveragedMv(int samples) {
    uint32_t sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += adcReadRaw();
    }
    // round to nearest instead of truncating
    uint32_t avgCounts = (sum + samples / 2) / (uint32_t)samples;
    return (int32_t)((avgCounts * ADC_VREF_MV + ADC_MAX_COUNTS / 2) / ADC_MAX_COUNTS);
}

void current_sense_init(void) {
    adcInit();

    // The board currently has a 1 kOhm/47 uF RC filter (not 10 uF as in the base
    // schematic) - time constant ~47 ms, so fully charging the capacitor
    // after power-up takes ~200-250 ms. If the zero is calibrated
    // before the capacitor has actually charged to the true ACS712
    // level, the offset comes out low and every later current reading
    // carries a systematic error of several amps. Wait here explicitly
    // rather than relying on however much time the previous init steps
    // (LCD etc.) happened to take before this call.
    Delay_Ms(250);

    // The pump must be off at this point (main.c holds PWM=0 until
    // calibration) - use this window to measure the real ACS712 offset
    // instead of assuming exactly Vref/2.
    s_zeroOffsetMv = readAveragedMv(200);
}

int16_t current_sense_read_ma(void) {
    int32_t mv = readAveragedMv(SAMPLE_COUNT);
    // 100 mV/A => 1 mA per 0.1 mV, so just multiply the difference by 10.
    return (int16_t)((mv - s_zeroOffsetMv) * 1000 / ACS712_MV_PER_AMP);
}
