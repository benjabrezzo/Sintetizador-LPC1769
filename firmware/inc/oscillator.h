/* Oscilador DDS de una voz (spec 3.1). Lógica pura: sin dependencias del hardware. */
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <stdint.h>

typedef struct {
    uint32_t phase;   /* posición en la tabla, Q16.16 con vuelta en 2^24: índice = bits 23:16 */
    uint32_t step;    /* incremento de fase por muestra (ΔP); se puede cambiar entre bloques */
} Oscillator;

/* Deja la fase en 0 con el incremento dado */
void oscInit(Oscillator *o, uint32_t step);

/* Devuelve la muestra Q15 de la tabla (WAVE_SIZE muestras) en la fase actual y avanza la fase un
 * incremento. La fase se conserva entre llamadas, así que los bloques siguientes continúan la onda. */
int16_t oscNextSample(Oscillator *o, const int16_t *table);

#endif /* OSCILLATOR_H */
