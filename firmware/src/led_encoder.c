#include "led_encoder.h"
#include <string.h>

void ledSurfaceInit(LedSurface *surface) {
    memset(surface, 0, sizeof(*surface));
}

void ledSurfaceSet(LedSurface *surface, uint8_t x, uint8_t y, LedColor color) {
    if (x >= LED_SURFACE_WIDTH || y >= LED_SURFACE_HEIGHT) {
        return;
    }
    surface->pixels[y][x] = color;
}

uint8_t ledSurfaceLedIndex(uint8_t x, uint8_t y) {
    /* Mapeo sin serpentina: todas las columnas van en el mismo sentido (y crece con el LED) */
    return (uint8_t)(x * LED_SURFACE_HEIGHT + y);
}

/* Tope de brillo (spec 3.6): el 25 % del valor del canal, truncado a entero. 255 queda en 63 */
static uint8_t limitBrightness(uint8_t value) {
    return (uint8_t)(value * LED_BRIGHTNESS_PERCENT / 100);
}

void ledEncode(const LedSurface *surface, uint8_t buffer[LED_ENCODED_BYTES]) {
    memset(buffer, 0, LED_ENCODED_BYTES);

    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor c = surface->pixels[y][x];
            const uint32_t word = ((uint32_t)limitBrightness(c.g) << 16) | ((uint32_t)limitBrightness(c.r) << 8) |
                                  limitBrightness(c.b);
            uint8_t *ledBytes = &buffer[ledSurfaceLedIndex(x, y) * LED_BITS_PER_LED * LED_BYTES_PER_BIT];
            for (uint8_t bit = 0; bit < LED_BITS_PER_LED; bit++) {
                ledBytes[bit * LED_BYTES_PER_BIT] = 1;
                if (word & (1u << (LED_BITS_PER_LED - 1 - bit))) {
                    ledBytes[bit * LED_BYTES_PER_BIT + 1] = 1;
                }
            }
        }
    }
}

LedColor ledSurfaceGet(const LedSurface *surface, uint8_t x, uint8_t y) {
    if (x >= LED_SURFACE_WIDTH || y >= LED_SURFACE_HEIGHT) {
        const LedColor off = {0, 0, 0};
        return off;
    }
    return surface->pixels[y][x];
}
