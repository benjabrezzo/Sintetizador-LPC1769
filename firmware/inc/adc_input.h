/* Lectura de los cuatro potenciómetros de la ADSR por ADC (spec 3.4 y 3.13). Módulo de hardware. */
#ifndef ADC_INPUT_H
#define ADC_INPUT_H

#include <stdint.h>

/* Posición de cada potenciómetro en las lecturas */
typedef enum {
    POT_ATTACK,
    POT_DECAY,
    POT_SUSTAIN,
    POT_RELEASE,
    POT_COUNT
} PotIndex;

/* Configura el ADC en modo burst sobre los cuatro canales de los potenciómetros, sin interrupción, y espera la
 * primera conversión de cada uno: al volver, adcInputRead ya da lecturas reales. Se llama antes de habilitar
 * SysTick. */
void adcInputInit(void);

/* Copia en readings la última conversión de cada potenciómetro (12 bits, 0 ... 4095), sin filtrar, en el orden de
 * PotIndex. No espera: el burst convierte solo. Se llama desde SysTick. */
void adcInputRead(uint16_t readings[POT_COUNT]);

#endif /* ADC_INPUT_H */
