#include "led_patterns.h"
#include "wave_select.h"

/* Color de los patrones de un solo color (píxel, fila y columna): rojo puro. Con VDD bajo el verde y el azul pueden salir
 * más tenues (spec 3.15) */
static const LedColor PATTERN_COLOR = {255, 0, 0};

/* Color de cada onda, indexado por su identificador (WAVE_*) con inicializadores designados, para no depender del orden
 * del enum. El amarillo y el morado no son los valores completos: se ajustan a ojo para distinguirlos del verde y del rojo
 * (valores de partida, story 18) */
static const LedColor waveColors[NUM_WAVES] = {
    [WAVE_SINE]     = {255, 0, 0},      /* rojo */
    [WAVE_SQUARE]   = {0, 0, 255},      /* azul */
    [WAVE_TRIANGLE] = {0, 255, 0},      /* verde */
    [WAVE_SAW]      = {255, 160, 0},    /* amarillo */
    [WAVE_PYTHON]   = {170, 0, 255},    /* morado */
};

LedColor ledWaveColor(uint8_t wave) {
    if (wave >= NUM_WAVES) {
        return waveColors[WAVE_SINE];       /* fuera de rango: rojo, la onda con que arranca la placa */
    }
    return waveColors[wave];
}

void ledPatternRow(LedSurface *surface, uint32_t step) {
    const uint8_t y = (uint8_t)(step % LED_SURFACE_HEIGHT);
    for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
        ledSurfaceSet(surface, x, y, PATTERN_COLOR);
    }
}

void ledPatternColumn(LedSurface *surface, uint32_t step) {
    const uint8_t x = (uint8_t)(step % LED_SURFACE_WIDTH);
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        ledSurfaceSet(surface, x, y, PATTERN_COLOR);
    }
}

void ledPatternColor(LedSurface *surface, uint32_t step) {
    /* Orden de la spec 3.6 (senoide, cuadrada, triangular, sierra y Python): el mismo color que ledWaveColor, así los
     * colores se ajustan en un solo lugar */
    static const uint8_t order[LED_PATTERN_COLORS] = {WAVE_SINE, WAVE_SQUARE, WAVE_TRIANGLE, WAVE_SAW, WAVE_PYTHON};
    const LedColor color = ledWaveColor(order[step % LED_PATTERN_COLORS]);
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            ledSurfaceSet(surface, x, y, color);
        }
    }
}

void ledPatternWhite(LedSurface *surface) {
    const LedColor white = {255, 255, 255};
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            ledSurfaceSet(surface, x, y, white);
        }
    }
}

void ledPatternPixel(LedSurface *surface, uint32_t step) {
    const uint32_t pixel = step % (LED_SURFACE_WIDTH * LED_SURFACE_HEIGHT);
    ledSurfaceSet(surface, (uint8_t)(pixel % LED_SURFACE_WIDTH), (uint8_t)(pixel / LED_SURFACE_WIDTH), PATTERN_COLOR);
}
