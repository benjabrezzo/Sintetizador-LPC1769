#include "oscillator.h"

#define PHASE_MASK   0x00FFFFFFu   /* vuelta en 2^24: tabla de 256 muestras con 16 bits de fracción */
#define INDEX_SHIFT  16            /* índice de la tabla = bits 23:16 de la fase */

void oscInit(Oscillator *o, uint32_t step) {
    o->phase = 0;
    o->step = step;
}

int16_t oscNextSample(Oscillator *o, const int16_t *table) {
    const int16_t sample = table[o->phase >> INDEX_SHIFT];   /* fase < 2^24: índice < 256 */
    o->phase = (o->phase + o->step) & PHASE_MASK;
    return sample;
}
