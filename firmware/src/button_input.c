#include "button_input.h"
#include "config.h"
#include "debounce.h"            /* DEBOUNCE_NUM_KEYS: cantidad de teclas */
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define KEYS_MASK  (((1u << DEBOUNCE_NUM_KEYS) - 1u) << KEYS_FIRST_PIN)   /* los bits de P2.0 a P2.7 */

_Static_assert(DEBOUNCE_NUM_KEYS == 8, "button_input lee un byte: una tecla por bit");

void buttonInputInit(void) {
    /* Tras el reset los pines ya son GPIO con pull-up; se configura igual para que no dependa del reset */
    for (uint32_t k = 0; k < DEBOUNCE_NUM_KEYS; k++) {
        const PINSEL_CFG_T pinCfg = {KEYS_PORT, KEYS_FIRST_PIN + k, PINSEL_FUNC_00, PINSEL_PULLUP, DISABLE};
        PINSEL_ConfigPin(&pinCfg);
    }
    GPIO_SetDir(KEYS_PORT, KEYS_MASK, GPIO_INPUT);
}

uint8_t buttonInputRead(void) {
    const uint32_t pins = GPIO_ReadValue(KEYS_PORT);
    /* Activas en bajo: pin en 0 = tecla apretada. P2.8 (medición) queda fuera con el & 0xFF */
    return (uint8_t)(~(pins >> KEYS_FIRST_PIN) & 0xFFu);
}

void buttonInputWaveInit(void) {
    const EXTI_CFG_T extiCfg = {EXTI_EINT1, EXTI_EDGE_SENSITIVE, EXTI_FALLING_EDGE};

    EXTI_Init();                                      /* las cuatro líneas deshabilitadas, por flanco bajo */
    EXTI_PinConfig(EXTI_EINT1, EXTI_PULLUP);          /* P2.11 con la función EINT1 y el pull-up interno */
    /* El driver no fija la prioridad y quedaría en 0, igual que el DMA (spec 3.12 pide 4) */
    NVIC_SetPriority(EINT1_IRQn, EINT1_IRQ_PRIORITY);
    EXTI_ConfigEnable(&extiCfg);                      /* limpia el flag y la interrupción pendiente, y habilita */
}

uint8_t buttonInputWaveRead(void) {
    return (uint8_t)((GPIO_ReadValue(WAVE_BUTTON_PORT) >> WAVE_BUTTON_PIN) & 1u);
}
