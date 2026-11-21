/* Patrones de prueba de la matriz (spec Bloque 12). Lógica pura: dibujan en una LedSurface y no la apagan antes, así
 * que el caller la apaga con ledSurfaceInit. */
#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include <stdint.h>
#include "led_encoder.h"

/* Enciende en rojo (255, 0, 0) el píxel número step, en orden de filas: el 0 es (0, 0), el 1 es (1, 0), el 8 es (0, 1)
 * y el 63 es (7, 7). A partir del 64 da la vuelta */
void ledPatternPixel(LedSurface *surface, uint32_t step);

/* Enciende en rojo la fila step % 8 entera (8 píxeles) */
void ledPatternRow(LedSurface *surface, uint32_t step);

/* Enciende en rojo la columna step % 8 entera (8 píxeles) */
void ledPatternColumn(LedSurface *surface, uint32_t step);

#define LED_PATTERN_COLORS  5     /* rojo, azul, verde, amarillo y morado (spec 3.6) */

/* Llena los 64 píxeles con el color step % 5, en el orden de la spec: rojo, azul, verde, amarillo y morado. Son los
 * colores de ledWaveColor para senoide, cuadrada, triangular, sierra y Python (el amarillo y el morado no van a valor
 * completo) */
void ledPatternColor(LedSurface *surface, uint32_t step);

/* Color de las barras para la onda wave (WAVE_* de wave_select.h): senoide rojo, cuadrada azul, triangular verde, sierra
 * amarillo y Python morado (spec 3.6) */
LedColor ledWaveColor(uint8_t wave);

/* Llena los 64 píxeles en blanco (255, 255, 255): el peor caso de consumo, ya con el tope de brillo al codificar */
void ledPatternWhite(LedSurface *surface);

#endif /* LED_PATTERNS_H */
