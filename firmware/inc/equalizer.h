/* Ecualizador de tres bandas (spec 3.5): tres Biquad IIR en cascada (low-shelf, peaking y high-shelf) en punto fijo.
 * Lógica pura: sin dependencias del hardware. Los coeficientes Q2.30 los calcula la aplicación y llegan por UART. */
#ifndef EQUALIZER_H
#define EQUALIZER_H

#include <stdint.h>

#define EQ_NUM_BANDS  3      /* low-shelf, peaking y high-shelf, en ese orden (docs/protocolo.md) */

/* Coeficientes de una banda en Q2.30 (el valor real por 2^30), normalizados con a0 = 1 */
typedef struct {
    int32_t b0, b1, b2, a1, a2;
} EqBandCoefs;

/* Historia de una banda en forma directa I: las dos últimas entradas y las dos últimas salidas, más el residuo
 * fraccionario que el desplazamiento de 30 bits descartó en la muestra anterior (realimentación del error) */
typedef struct {
    int32_t x1, x2, y1, y2;
    int32_t residue;        /* 0 ... 2^30 − 1 */
} EqBandState;

typedef struct {
    EqBandCoefs coefs[EQ_NUM_BANDS];
    EqBandState state[EQ_NUM_BANDS];
} Equalizer;

/* Deja las tres bandas en 0 dB (identidad: b0 = 1,0 y el resto en 0) y el estado en cero */
void equalizerInit(Equalizer *eq);

/* Reemplaza los coeficientes de las tres bandas (low-shelf, peaking y high-shelf). El estado de los filtros no se toca:
 * el cambio solo afecta a cómo se calculan las muestras siguientes */
void equalizerSetCoefs(Equalizer *eq, const EqBandCoefs coefs[EQ_NUM_BANDS]);

/* Pasa una muestra por la cascada y devuelve la salida. Entrada y salida van en la escala de la mezcla (spec 3.2,
 * paso 2): a escala máxima, 4 voces dan ±65534. Cada banda acumula en 64 bits */
int32_t equalizerProcess(Equalizer *eq, int32_t x);

#endif /* EQUALIZER_H */
