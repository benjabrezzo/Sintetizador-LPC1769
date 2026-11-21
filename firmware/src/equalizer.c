#include "equalizer.h"

#define Q2_30_FRACTION_BITS  30
#define Q2_30_ONE            (1 << Q2_30_FRACTION_BITS)    /* 1,0 en Q2.30 */

void equalizerInit(Equalizer *eq) {
    for (uint32_t band = 0; band < EQ_NUM_BANDS; band++) {
        const EqBandCoefs identity = {Q2_30_ONE, 0, 0, 0, 0};
        const EqBandState silence = {0, 0, 0, 0, 0};
        eq->coefs[band] = identity;
        eq->state[band] = silence;
    }
}

void equalizerSetCoefs(Equalizer *eq, const EqBandCoefs coefs[EQ_NUM_BANDS]) {
    for (uint32_t band = 0; band < EQ_NUM_BANDS; band++) {
        eq->coefs[band] = coefs[band];       /* el estado de la banda no se toca */
    }
}

/* Una muestra por una banda, forma directa I: y = b0·x + b1·x1 + b2·x2 − a1·y1 − a2·y2. Cada producto es un Q2.30 por
 * un entero y se acumula en 64 bits: con ±65534 a la entrada el producto pasa de 2^46. El resultado vuelve a la escala
 * de la entrada con un desplazamiento de 30 bits.
 * El desplazamiento no tira la parte fraccionaria: queda en s->residue (0 ... 2^30 − 1) y se suma a la muestra
 * siguiente (realimentación del error). Truncar o redondear sin guardarla deja un offset de continua cuando la entrada
 * vuelve a cero, porque el denominador casi se anula en continua (1 + a1 + a2 ≈ 0,001 con el low-shelf a +6 dB) y el
 * error de cuantización se amplifica cientos de veces; con el residuo, la salida vuelve a 0 exacto */
static int32_t biquadStep(const EqBandCoefs *c, EqBandState *s, int32_t x) {
    const int64_t acc = (int64_t)c->b0 * x + (int64_t)c->b1 * s->x1 + (int64_t)c->b2 * s->x2
                      - (int64_t)c->a1 * s->y1 - (int64_t)c->a2 * s->y2 + s->residue;
    const int32_t y = (int32_t)(acc >> Q2_30_FRACTION_BITS);
    s->residue = (int32_t)(acc - ((int64_t)y << Q2_30_FRACTION_BITS));
    s->x2 = s->x1;
    s->x1 = x;
    s->y2 = s->y1;
    s->y1 = y;
    return y;
}

int32_t equalizerProcess(Equalizer *eq, int32_t x) {
    for (uint32_t band = 0; band < EQ_NUM_BANDS; band++) {
        x = biquadStep(&eq->coefs[band], &eq->state[band], x);
    }
    return x;
}
