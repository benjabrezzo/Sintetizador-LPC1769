#include "led_bars.h"
#include "config.h"

void ledBarsDraw(LedSurface *surface, const uint8_t levels[LED_BARS_COUNT], LedColor color) {
    for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
        /* La fila 0 es la de arriba: la barra de altura h ocupa de la fila 8 − h a la 7 */
        for (uint8_t i = 0; i < levels[x]; i++) {
            ledSurfaceSet(surface, x, (uint8_t)(LED_SURFACE_HEIGHT - 1 - i), color);
        }
    }
}

void ledBarsFall(uint8_t shown[LED_BARS_COUNT], const uint8_t levels[LED_BARS_COUNT]) {
    for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
        /* La barra mostrada es el mayor entre el nivel nuevo y la anterior menos BARS_FALL (sin pasar de abajo de 0) */
        const uint8_t lowered = (shown[x] > BARS_FALL) ? (uint8_t)(shown[x] - BARS_FALL) : 0;
        shown[x] = (levels[x] > lowered) ? levels[x] : lowered;
    }
}
