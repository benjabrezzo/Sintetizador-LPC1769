/* Análisis espectral de la señal de audio (spec 3.6): ventana Hann, FFT de 512 puntos en Q15 y magnitudes.
 * Lógica pura: sin dependencias del hardware. Las tablas de la ventana y de los factores de giro están en tables.h. */
#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "tables.h"          /* FFT_SIZE */

#define SPECTRUM_BINS       (FFT_SIZE / 2)    /* bins útiles: de 0 (continua) a FFT_SIZE/2 − 1; cada uno mide Fs / FFT_SIZE */
#define SPECTRUM_NUM_BANDS  8                 /* octavas de 62,5 Hz a 16 kHz (spec 3.6) */
#define SPECTRUM_MAX_LEVEL  8                 /* altura máxima de una barra, en píxeles (spec 3.6) */

/* Aplica la ventana Hann a FFT_SIZE muestras Q15, calcula su FFT (radix-2, con escalado de 1/2 por etapa) y deja en
 * magnitudes el módulo de cada bin de 0 a SPECTRUM_BINS − 1, normalizado para que una senoide centrada en un bin dé su
 * amplitud Q15 (resolución de 4 unidades: el módulo se calcula antes de normalizar). Usa dos arreglos de trabajo propios (2 KB de RAM): no es
 * reentrante. */
void spectrumMagnitudes(const int16_t samples[FFT_SIZE], uint16_t magnitudes[SPECTRUM_BINS]);

/* Agrupa los bins en las 8 bandas de una octava (spec 3.6): la banda b (0 a 7) toma el máximo de las magnitudes de
 * los bins 2^b ... 2^(b+1) − 1, o sea 1, 2-3, 4-7, ... 128-255. El bin 0 (continua) no entra en ninguna */
void spectrumBands(const uint16_t magnitudes[SPECTRUM_BINS], uint16_t bands[SPECTRUM_NUM_BANDS]);

/* Convierte la magnitud de cada banda en la altura de su barra, de 0 a SPECTRUM_MAX_LEVEL (spec 3.6). Escala
 * logarítmica, un nivel por cada duplicación de la magnitud: el nivel 1 desde 128 (1/256 del fondo de escala, 32768)
 * y el nivel 8 desde 16384 (1/2, −6 dB); por debajo de 128, 0. levels entra con los niveles del cuadro anterior y sale
 * con los nuevos: el caller los inicializa en 0. */
void spectrumLevels(const uint16_t bands[SPECTRUM_NUM_BANDS], uint8_t levels[SPECTRUM_NUM_BANDS]);

/* Bin de mayor magnitud entre el 1 y el SPECTRUM_BINS − 1 (el bin 0, la continua, no cuenta como pico, igual que en las
 * bandas). Con empate, el más bajo. Devuelve 0 si ninguno pasa de 0 (no hay pico): sirve para leer en el depurador
 * dónde cae un tono. */
uint32_t spectrumPeakBin(const uint16_t magnitudes[SPECTRUM_BINS]);

#endif /* SPECTRUM_H */
