#include "mixer.h"

#define Q15_SHIFT       15   /* muestra Q15 · amplitud Q15 = Q30: se vuelve a Q15 */
#define MIX_GAIN_SHIFT  1    /* ganancia fija de 1/2 (spec 3.2) */

int32_t mixNextSample(Voice *voices, uint32_t numVoices, const int16_t *const *tables) {
    int32_t acc = 0;
    for (uint32_t v = 0; v < numVoices; v++) {
        /* Se compara la etapa directo (y no con envIsIdle): es el lazo caliente de la ISR, una vez por muestra y voz */
        if (voices[v].env.stage == ENV_IDLE) {
            continue;                /* una voz libre no suma ni avanza su fase */
        }
        const int32_t sample = oscNextSample(&voices[v].osc, tables[voices[v].wave]);
        /* |sample · nivel| < 2^30: no desborda. Cada producto se desplaza antes de sumar */
        acc += (sample * voices[v].env.level) >> Q15_SHIFT;
    }
    return acc >> MIX_GAIN_SHIFT;
}
