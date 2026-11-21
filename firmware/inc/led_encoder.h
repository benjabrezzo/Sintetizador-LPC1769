/* Superficie lógica de 8×8 y codificación para los WS2812B (spec 3.6, 3.15 y Bloque 12).
 * Lógica pura: sin dependencias del hardware. La superficie guarda el color completo de cada píxel. */
#ifndef LED_ENCODER_H
#define LED_ENCODER_H

#include <stdint.h>

#define LED_SURFACE_WIDTH   8     /* columnas: una por banda del espectro */
#define LED_SURFACE_HEIGHT  8     /* filas */

/* Buffer codificado para el DMA (spec 3.15): cada LED lleva 24 bits y cada bit se envía con 3 transferencias de 1 byte
 * (alto, dato y bajo) */
#define LED_BITS_PER_LED      24
#define LED_BYTES_PER_BIT     3
#define LED_ENCODED_BYTES     (LED_SURFACE_WIDTH * LED_SURFACE_HEIGHT * LED_BITS_PER_LED * LED_BYTES_PER_BIT)

/* Tope de brillo (spec 3.6): ledEncode saca cada canal al 25 % de su valor. El peor caso, los 64 LED en blanco, queda en
 * ≈ 1 A. La superficie guarda el color completo: el tope solo se aplica al codificar */
#define LED_BRIGHTNESS_PERCENT  25

/* Color de un píxel, un canal por byte de 0 a 255 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} LedColor;

/* Superficie lógica: pixels[y][x], con (0, 0) arriba a la izquierda, x hacia la derecha e y hacia abajo */
typedef struct {
    LedColor pixels[LED_SURFACE_HEIGHT][LED_SURFACE_WIDTH];
} LedSurface;

/* Apaga los 64 píxeles */
void ledSurfaceInit(LedSurface *surface);

/* Pone el color del píxel (x, y). Fuera de la superficie no hace nada; ledSurfaceGet devuelve apagado */
void ledSurfaceSet(LedSurface *surface, uint8_t x, uint8_t y, LedColor color);

/* Color del píxel (x, y) */
LedColor ledSurfaceGet(const LedSurface *surface, uint8_t x, uint8_t y);

/* Posición en la cadena (0 a 63) del LED que está en el píxel (x, y), con x e y dentro de la superficie. Hipótesis de
 * la CJMCU-8*8: sin serpentina, todas las columnas en el mismo sentido, con el LED 0 en (0, 0). Se confirma a ojo en la
 * placa (story 16) */
uint8_t ledSurfaceLedIndex(uint8_t x, uint8_t y);

/* Codifica la superficie en el buffer que el DMA copia a P0.0: cada byte vale 0 o 1, el nivel del pin. Un bit en uno
 * son 3 bytes alto, alto, bajo (spec 3.15) */
void ledEncode(const LedSurface *surface, uint8_t buffer[LED_ENCODED_BYTES]);

#endif /* LED_ENCODER_H */
