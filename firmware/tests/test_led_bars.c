/* Tests de las barras del espectro en la matriz (spec 3.6 y Bloque 13). Lógica pura: ledBarsDraw dibuja los 8 niveles
 * en una LedSurface que el caller apagó de antemano. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "led_bars.h"
#include "spectrum.h"
#include "test_check.h"

/* Con todos los niveles en 0 no hay nada que encender: la superficie sigue apagada, sea cual sea el color */
static void testBarrasNivelCeroDejaTodoApagado(void) {
    const uint8_t levels[LED_BARS_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
    const LedColor red = {255, 0, 0};

    LedSurface s;
    ledSurfaceInit(&s);
    ledBarsDraw(&s, levels, red);

    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor c = ledSurfaceGet(&s, x, y);
            CHECK(c.r == 0 && c.g == 0 && c.b == 0);
        }
    }
}

/* Una barra de altura h enciende h píxeles de abajo hacia arriba: la fila 0 es la de arriba, así que la barra ocupa de la
 * fila 8 − h a la 7. Se prueba en la columna 3 con el resto de los niveles en 0; la primera fila encendida de cada caso
 * está escrita a mano */
static void testBarraDeAlturaHEnciendeHPixelesDesdeAbajo(void) {
    static const struct {
        uint8_t height;
        uint8_t firstRow;    /* fila más alta encendida */
    } cases[] = {
        {1, 7}, {4, 4}, {8, 0},
    };
    const uint8_t column = 3;
    const LedColor red = {255, 0, 0};

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        uint8_t levels[LED_BARS_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
        levels[column] = cases[i].height;

        LedSurface s;
        ledSurfaceInit(&s);
        ledBarsDraw(&s, levels, red);

        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
                const LedColor c = ledSurfaceGet(&s, x, y);
                if (x == column && y >= cases[i].firstRow) {
                    CHECK(c.r == 255 && c.g == 0 && c.b == 0);
                } else {
                    CHECK(c.r == 0 && c.g == 0 && c.b == 0);
                }
            }
        }
    }
}

/* Cada banda tiene su columna, de graves a agudos de izquierda a derecha: la columna 0 es la banda 1 y la 7, la banda 8.
 * Con alturas distintas en cada una (1 a 8) se nota si alguna columna se cruza o se invierte el orden: la columna x
 * tiene que tener x + 1 píxeles encendidos, de abajo hacia arriba */
static void testCadaBandaVaEnSuColumna(void) {
    const uint8_t levels[LED_BARS_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8};
    const LedColor red = {255, 0, 0};

    LedSurface s;
    ledSurfaceInit(&s);
    ledBarsDraw(&s, levels, red);

    for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
        const uint8_t firstRow = (uint8_t)(LED_SURFACE_HEIGHT - 1 - x);    /* la columna x sube x + 1 filas desde la 7 */
        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            const LedColor c = ledSurfaceGet(&s, x, y);
            if (y >= firstRow) {
                CHECK(c.r == 255 && c.g == 0 && c.b == 0);
            } else {
                CHECK(c.r == 0 && c.g == 0 && c.b == 0);
            }
        }
    }
}

/* El color viene por parámetro (la story 17 pasa rojo y la 18 pasará el de la onda seleccionada): los píxeles encendidos
 * tienen exactamente ese color, sea cual sea, y los demás quedan apagados. Se prueban el azul de la cuadrada y el
 * amarillo de la sierra (rojo + verde), con una barra de 3 en la columna 6 */
static void testBarrasUsanElColorRecibido(void) {
    static const LedColor colors[] = {
        {0, 0, 255},      /* azul */
        {255, 255, 0},    /* amarillo */
    };
    const uint8_t column = 6;
    const uint8_t height = 3;

    for (uint32_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
        uint8_t levels[LED_BARS_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
        levels[column] = height;

        LedSurface s;
        ledSurfaceInit(&s);
        ledBarsDraw(&s, levels, colors[i]);

        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
                const LedColor c = ledSurfaceGet(&s, x, y);
                if (x == column && y >= LED_SURFACE_HEIGHT - height) {
                    CHECK(c.r == colors[i].r && c.g == colors[i].g && c.b == colors[i].b);
                } else {
                    CHECK(c.r == 0 && c.g == 0 && c.b == 0);
                }
            }
        }
    }
}

/* Etapa de caída en pantalla (spec 3.6, Bloque 13): la subida es inmediata. Si el nivel nuevo es mayor que la barra
 * mostrada, la barra pasa de una al nivel nuevo, sea cual sea el salto. Todas las columnas del caso suben (ninguna
 * queda igual) y los valores esperados son los niveles nuevos, escritos a mano */
static void testCaidaSubeDeInmediatoAlNivelNuevo(void) {
    uint8_t shown[LED_BARS_COUNT] = {0, 0, 2, 3, 4, 5, 6, 7};
    const uint8_t levels[LED_BARS_COUNT] = {1, 8, 5, 8, 6, 8, 7, 8};
    const uint8_t expected[LED_BARS_COUNT] = {1, 8, 5, 8, 6, 8, 7, 8};

    ledBarsFall(shown, levels);

    for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
        CHECK(shown[x] == expected[x]);
    }
}

/* La caída es de un nivel por cuadro de pantalla (BARS_FALL = 1): con el audio cortado (nivel nuevo 0), una barra de 8
 * tarda 8 cuadros en llegar a 0, o sea 320 ms a 25 FPS. Las alturas esperadas después de cada llamada están escritas
 * a mano */
static void testCaidaBajaUnNivelPorCuadro(void) {
    static const uint8_t expectedAfterFrame[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    uint8_t shown[LED_BARS_COUNT] = {8, 8, 8, 8, 8, 8, 8, 8};
    const uint8_t levels[LED_BARS_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};

    for (uint8_t frame = 0; frame < 8; frame++) {
        ledBarsFall(shown, levels);
        for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
            CHECK(shown[x] == expectedAfterFrame[frame]);
        }
    }
}

/* Bordes de la caída (con BARS_FALL = 1): la barra no baja de 0 (una barra en 0 con nivel 0 sigue en 0, sin dar la vuelta
 * en el entero sin signo), no baja por debajo del nivel nuevo (mostrada 5 con nivel 4 queda en 4) y no baja si el nivel
 * coincide con la barra (4 con 4 sigue en 4). Con la barra más alta que el nivel baja uno, no hasta el nivel (5 con 3
 * queda en 4). Cada caso se prueba con las 8 columnas iguales; los esperados están escritos a mano */
static void testCaidaNoPasaDeAbajoDeCeroNiSaltaPorDebajoDelNivelNuevo(void) {
    static const struct {
        uint8_t shown;
        uint8_t level;
        uint8_t expected;
    } cases[] = {
        {0, 0, 0}, {1, 0, 0}, {4, 4, 4}, {5, 4, 4}, {5, 3, 4}, {8, 7, 7},
    };

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        uint8_t shown[LED_BARS_COUNT];
        uint8_t levels[LED_BARS_COUNT];
        for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
            shown[x] = cases[i].shown;
            levels[x] = cases[i].level;
        }

        ledBarsFall(shown, levels);

        for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
            CHECK(shown[x] == cases[i].expected);
        }
    }
}

/* Cada columna cae por su cuenta: en una sola llamada hay columnas que suben, que bajan un nivel, que ya están en su
 * nivel y que llegan a 0, y ninguna se mezcla con otra. Los niveles del análisis no se tocan. Los esperados están
 * escritos a mano, columna por columna (mostrada → esperada): 0→3 sube, 8→7 baja, 5→5 igual, 3→7 sube, 1→0 baja a 0,
 * 6→5 baja, 4→4 igual, 2→8 sube */
static void testCaidaTrataCadaColumnaPorSeparado(void) {
    uint8_t shown[LED_BARS_COUNT] = {0, 8, 5, 3, 1, 6, 4, 2};
    const uint8_t levels[LED_BARS_COUNT] = {3, 0, 5, 7, 0, 2, 4, 8};
    const uint8_t levelsBefore[LED_BARS_COUNT] = {3, 0, 5, 7, 0, 2, 4, 8};
    const uint8_t expected[LED_BARS_COUNT] = {3, 7, 5, 7, 0, 5, 4, 8};

    ledBarsFall(shown, levels);

    for (uint8_t x = 0; x < LED_BARS_COUNT; x++) {
        CHECK(shown[x] == expected[x]);
        CHECK(levels[x] == levelsBefore[x]);
    }
}

/* Prueba encadenada del Bloque 13: una senoide de 1 kHz pasa por spectrumMagnitudes, spectrumBands, spectrumLevels y
 * ledBarsDraw. A 32 kHz, 1 kHz son 16 períodos en las 512 muestras: cae justo en el bin 16, que es de la banda 5
 * (1 a 2 kHz, bins 16 a 31) y, como la columna 0 es la banda 1, de la columna de índice 4. Solo se mira esa columna: la
 * fuga de la ventana Hann (±2 bins) puede subir también las vecinas y no se asume nada de ellas. Las alturas esperadas
 * salen de la escala de la spec (nivel n desde 128 · 2^(n−1), con 32768 a fondo de escala), no del código: con amplitud
 * 0,75 (24576 ≥ 16384) la barra llega a 8 y con 0,3 (9830, entre 8192 y 16384) a 7. Las dos amplitudes están lejos de
 * los bordes de nivel */
static void testSenoideDe1kHzSoloSeMideEnLaColumna5(void) {
    static const struct {
        double amplitude;     /* Q15 */
        uint8_t height;       /* altura esperada de la barra */
    } cases[] = {
        {0.75 * 32768.0, 8}, {0.3 * 32768.0, 7},
    };
    const uint8_t column = 4;           /* banda 5: la quinta columna, de índice 4 */
    const double twoPi = 6.283185307179586;
    const LedColor red = {255, 0, 0};

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        static int16_t samples[FFT_SIZE];
        static uint16_t magnitudes[SPECTRUM_BINS];
        uint16_t bands[SPECTRUM_NUM_BANDS];
        uint8_t levels[SPECTRUM_NUM_BANDS] = {0, 0, 0, 0, 0, 0, 0, 0};

        for (uint32_t n = 0; n < FFT_SIZE; n++) {
            samples[n] = (int16_t)lround(cases[i].amplitude * sin(twoPi * 16.0 * n / FFT_SIZE));
        }
        spectrumMagnitudes(samples, magnitudes);
        spectrumBands(magnitudes, bands);
        spectrumLevels(bands, levels);

        LedSurface s;
        ledSurfaceInit(&s);
        ledBarsDraw(&s, levels, red);

        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            const LedColor c = ledSurfaceGet(&s, column, y);
            if (y >= LED_SURFACE_HEIGHT - cases[i].height) {
                CHECK(c.r == 255 && c.g == 0 && c.b == 0);
            } else {
                CHECK(c.r == 0 && c.g == 0 && c.b == 0);
            }
        }
    }
}

int main(void) {
    testBarrasNivelCeroDejaTodoApagado();
    testBarraDeAlturaHEnciendeHPixelesDesdeAbajo();
    testCadaBandaVaEnSuColumna();
    testBarrasUsanElColorRecibido();
    testCaidaSubeDeInmediatoAlNivelNuevo();
    testCaidaBajaUnNivelPorCuadro();
    testCaidaNoPasaDeAbajoDeCeroNiSaltaPorDebajoDelNivelNuevo();
    testCaidaTrataCadaColumnaPorSeparado();
    testSenoideDe1kHzSoloSeMideEnLaColumna5();
    if (failures > 0) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
