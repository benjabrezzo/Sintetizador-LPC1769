/* Barras del espectro en la matriz (spec 3.6 y Bloque 13). Lógica pura: sin dependencias del hardware. */
#ifndef LED_BARS_H
#define LED_BARS_H

#include <stdint.h>
#include "led_encoder.h"

#define LED_BARS_COUNT  LED_SURFACE_WIDTH     /* una barra (columna) por banda del espectro */

/* Dibuja las barras en la superficie, sin apagarla antes (el caller la apaga con ledSurfaceInit). La columna b es la
 * banda b + 1 (la 0, de 62,5 a 125 Hz) y levels[b] es su altura de 0 a 8: enciende ese número de píxeles de abajo hacia
 * arriba, con el color recibido */
void ledBarsDraw(LedSurface *surface, const uint8_t levels[LED_BARS_COUNT], LedColor color);

/* Etapa de caída en pantalla (spec 3.6, Bloque 13): actualiza las alturas mostradas con los niveles del análisis. La
 * barra sube de inmediato al nivel nuevo */
void ledBarsFall(uint8_t shown[LED_BARS_COUNT], const uint8_t levels[LED_BARS_COUNT]);

#endif /* LED_BARS_H */
