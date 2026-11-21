#include "adc_input.h"
#include "config.h"
#include "lpc17xx_adc.h"

/* Canal del ADC de cada potenciómetro, en el orden de PotIndex */
static const ADC_CHANNEL potChannels[POT_COUNT] = {
    (ADC_CHANNEL)POT_ATTACK_CHANNEL,
    (ADC_CHANNEL)POT_DECAY_CHANNEL,
    (ADC_CHANNEL)POT_SUSTAIN_CHANNEL,
    (ADC_CHANNEL)POT_RELEASE_CHANNEL,
};

void adcInputInit(void) {
    ADC_Init(ADC_RATE_HZ);                  /* reloj, divisor y encendido; START queda en 000, como pide el burst */
    for (uint32_t i = 0; i < POT_COUNT; i++) {
        ADC_PinConfig(potChannels[i]);      /* función AD0.x, sin pull-up ni pull-down */
        ADC_ChannelEnable(potChannels[i]);
    }
    ADC_BurstEnable();                      /* desde acá convierte los cuatro canales sin parar */

    /* Espera la primera conversión de cada canal (unos 21 µs): así la primera lectura, la que arranca los filtros,
     * es la posición real del pote. Leer el estado borra DONE, pero el resultado queda en el registro. */
    for (uint32_t i = 0; i < POT_COUNT; i++) {
        while (ADC_ChannelGetStatus(potChannels[i], ADC_DATA_DONE) == RESET) {
        }
    }
}

void adcInputRead(uint16_t readings[POT_COUNT]) {
    for (uint32_t i = 0; i < POT_COUNT; i++) {
        readings[i] = ADC_ChannelGetData(potChannels[i]);
    }
}
