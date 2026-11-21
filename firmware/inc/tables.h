/* GENERADO por firmware/tools/gen_tables.py. No editar a mano. */
#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

#define WAVE_SIZE 256   /* muestras por período */
#define NUM_NOTES 8     /* teclas de Do4 a Do5 (spec 3.11) */
#define FFT_SIZE 512    /* puntos de la FFT (spec 3.6) */

/* Senoide de un período en Q15 (Flash) */
extern const int16_t sineTable[WAVE_SIZE];

/* Cuadrada, sierra y triangular de un período en Q15, limitadas en banda a 30 armónicos (Flash) */
extern const int16_t squareTable[WAVE_SIZE];
extern const int16_t sawTable[WAVE_SIZE];
extern const int16_t triangleTable[WAVE_SIZE];

/* Incremento de fase por muestra de cada nota (Q16.16, vuelta en 2^24),
 * con Fs = 25 MHz / 781 (Flash) */
extern const uint32_t noteSteps[NUM_NOTES];

/* Ventana Hann periódica de FFT_SIZE puntos en Q15 (Flash) */
extern const int16_t hannWindow[FFT_SIZE];

/* Factores de giro de la FFT, W_N^k = cos(2πk/N) − j·sin(2πk/N) para k = 0 ... FFT_SIZE/2 − 1, en Q15:
 * el coseno y el seno (positivo) de 2πk/N (Flash) */
extern const int16_t fftTwiddleCos[FFT_SIZE / 2];
extern const int16_t fftTwiddleSin[FFT_SIZE / 2];

#endif /* TABLES_H */
