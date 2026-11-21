/* Tests de los patrones de prueba de la matriz (spec Bloque 12: encender un píxel, recorrer filas y columnas, mostrar
 * patrones y colores). Lógica pura: cada patrón dibuja en una LedSurface que el caller apagó de antemano. */
#include <stdio.h>
#include <stdint.h>
#include "led_patterns.h"
#include "wave_select.h"
#include "test_check.h"

/* Recorrido de un píxel: el paso 'step' enciende solo un píxel, en rojo, en orden de filas (x crece primero, después
 * y); a partir del 64 da la vuelta. Eso permite ver a ojo el píxel (0, 0) y la orientación de la matriz. Los píxeles
 * esperados están escritos a mano */
static void testPatronUnPixelEnciendeSoloElPixelN(void) {
    static const struct {
        uint32_t step;
        uint8_t x;
        uint8_t y;
    } cases[] = {
        {0, 0, 0}, {1, 1, 0}, {7, 7, 0}, {8, 0, 1}, {63, 7, 7}, {64, 0, 0}, {70, 6, 0},
    };

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        LedSurface s;
        ledSurfaceInit(&s);
        ledPatternPixel(&s, cases[i].step);

        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
                const LedColor c = ledSurfaceGet(&s, x, y);
                if (x == cases[i].x && y == cases[i].y) {
                    CHECK(c.r == 255 && c.g == 0 && c.b == 0);
                } else {
                    CHECK(c.r == 0 && c.g == 0 && c.b == 0);
                }
            }
        }
    }
}

/* Recorrido de filas y de columnas: el paso 'step' enciende en rojo la fila (o la columna) step % 8 entera, 8 píxeles, y
 * deja apagado el resto. A partir del 8 da la vuelta. Las filas y columnas esperadas están escritas a mano */
static const struct {
    uint32_t step;
    uint8_t line;      /* fila o columna que se enciende */
} lineCases[] = {
    {0, 0}, {1, 1}, {7, 7}, {8, 0}, {13, 5},
};

static void testPatronFilaYColumnaEncienden8Pixeles(void) {
    for (uint32_t i = 0; i < sizeof(lineCases) / sizeof(lineCases[0]); i++) {
        LedSurface rowSurface;
        LedSurface columnSurface;
        ledSurfaceInit(&rowSurface);
        ledSurfaceInit(&columnSurface);
        ledPatternRow(&rowSurface, lineCases[i].step);
        ledPatternColumn(&columnSurface, lineCases[i].step);

        uint32_t rowLit = 0;
        uint32_t columnLit = 0;
        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
                const LedColor r = ledSurfaceGet(&rowSurface, x, y);
                const LedColor c = ledSurfaceGet(&columnSurface, x, y);
                const int rowOn = (r.r == 255 && r.g == 0 && r.b == 0);
                const int columnOn = (c.r == 255 && c.g == 0 && c.b == 0);
                CHECK(rowOn == (y == lineCases[i].line));
                CHECK(columnOn == (x == lineCases[i].line));
                /* Un píxel apagado es (0, 0, 0): ningún otro color vale */
                CHECK(rowOn || (r.r == 0 && r.g == 0 && r.b == 0));
                CHECK(columnOn || (c.r == 0 && c.g == 0 && c.b == 0));
                rowLit += rowOn;
                columnLit += columnOn;
            }
        }
        CHECK(rowLit == 8);
        CHECK(columnLit == 8);
    }
}

/* Prueba de colores: el paso 'step' llena los 64 píxeles con el color step % 5, en el orden de la spec 3.6: rojo,
 * azul, verde, amarillo y morado (el de las ondas senoide, cuadrada, triangular, sierra y Python). Son los mismos colores
 * de las barras (ledWaveColor): el amarillo y el morado con los valores de partida de la story 18, no al valor completo.
 * El tope de brillo se aplica al codificar. A partir del 5 da la vuelta. Los colores esperados están escritos a mano */
static void testPatronCincoColores(void) {
    static const struct {
        uint32_t step;
        LedColor color;
    } cases[] = {
        {0, {255, 0, 0}},        /* rojo */
        {1, {0, 0, 255}},        /* azul */
        {2, {0, 255, 0}},        /* verde */
        {3, {255, 160, 0}},      /* amarillo */
        {4, {170, 0, 255}},      /* morado */
        {5, {255, 0, 0}},        /* vuelta: rojo */
        {9, {170, 0, 255}},      /* vuelta: morado */
    };

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        LedSurface s;
        ledSurfaceInit(&s);
        ledPatternColor(&s, cases[i].step);

        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
                const LedColor c = ledSurfaceGet(&s, x, y);
                CHECK(c.r == cases[i].color.r);
                CHECK(c.g == cases[i].color.g);
                CHECK(c.b == cases[i].color.b);
            }
        }
    }
}

/* Color de las barras según la onda seleccionada (spec 3.6, story 18): cada identificador de onda (WAVE_*) tiene su
 * color, y los identificadores siguen la rotación de las ondas, no el orden de colores de la spec. Los colores esperados
 * son los valores de partida de las Notas de la story, escritos a mano: el amarillo y el morado se ajustan en el
 * laboratorio */
static void testOndaDevuelveSuColor(void) {
    static const struct {
        uint8_t wave;
        LedColor color;
    } cases[] = {
        {WAVE_SINE,     {255, 0, 0}},      /* rojo */
        {WAVE_SQUARE,   {0, 0, 255}},      /* azul */
        {WAVE_TRIANGLE, {0, 255, 0}},      /* verde */
        {WAVE_SAW,      {255, 160, 0}},    /* amarillo */
        {WAVE_PYTHON,   {170, 0, 255}},    /* morado */
    };

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const LedColor c = ledWaveColor(cases[i].wave);
        CHECK(c.r == cases[i].color.r);
        CHECK(c.g == cases[i].color.g);
        CHECK(c.b == cases[i].color.b);
    }
}

/* Un identificador fuera de rango devuelve rojo, el color de la onda con que arranca la placa: waveSelect no lo genera,
 * pero activeWave se puede escribir desde Expressions. El límite es NUM_WAVES: el primero que no existe y el máximo de
 * un byte */
static void testOndaFueraDeRangoEsRojo(void) {
    static const uint8_t invalid[] = {NUM_WAVES, NUM_WAVES + 1, 255};

    for (uint32_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        const LedColor c = ledWaveColor(invalid[i]);
        CHECK(c.r == 255);
        CHECK(c.g == 0);
        CHECK(c.b == 0);
    }
}

/* Prueba de consumo (spec 3.6): los 64 píxeles en blanco (255, 255, 255), al valor completo; el tope de brillo se
 * aplica al codificar. La superficie arranca con un píxel de otro color, que el patrón tiene que pisar */
static void testPatronBlancoLlenaLos64(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor other = {10, 20, 30};
    ledSurfaceSet(&s, 5, 2, other);
    ledPatternWhite(&s);

    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor c = ledSurfaceGet(&s, x, y);
            CHECK(c.r == 255);
            CHECK(c.g == 255);
            CHECK(c.b == 255);
        }
    }
}

int main(void) {
    testPatronUnPixelEnciendeSoloElPixelN();
    testPatronFilaYColumnaEncienden8Pixeles();
    testPatronCincoColores();
    testOndaDevuelveSuColor();
    testOndaFueraDeRangoEsRojo();
    testPatronBlancoLlenaLos64();
    if (failures > 0) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
