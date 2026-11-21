/* Tests de la superficie lógica de 8×8 y de la codificación para los WS2812B (spec 3.6, 3.15 y Bloque 12).
 * Lógica pura: la superficie guarda el color completo de cada píxel y el tope de brillo se aplica al codificar. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "led_encoder.h"
#include "test_check.h"

/* Después de ledSurfaceInit los 64 píxeles están apagados (0, 0, 0), aunque la memoria tuviera basura */
static void testSuperficieArrancaApagada(void) {
    LedSurface s;
    memset(&s, 0xFF, sizeof(s));
    ledSurfaceInit(&s);
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor c = ledSurfaceGet(&s, x, y);
            CHECK(c.r == 0);
            CHECK(c.g == 0);
            CHECK(c.b == 0);
        }
    }
}

/* ledSurfaceSet escribe solo el píxel pedido. Se usa (3, 5) y no uno simétrico: el vecino traspuesto (5, 3) delata
 * un x e y cambiados, y los de al lado, un corrimiento */
static void testSetPixelGuardaElColorEnEsaPosicion(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor color = {10, 20, 30};
    ledSurfaceSet(&s, 3, 5, color);

    const LedColor got = ledSurfaceGet(&s, 3, 5);
    CHECK(got.r == 10);
    CHECK(got.g == 20);
    CHECK(got.b == 30);

    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            if (x == 3 && y == 5) {
                continue;
            }
            const LedColor c = ledSurfaceGet(&s, x, y);
            CHECK(c.r == 0 && c.g == 0 && c.b == 0);
        }
    }
}

/* Fuera de 0…7, ledSurfaceSet no hace nada y ledSurfaceGet devuelve apagado. (8, 0) caería sobre el (0, 1) de la
 * fila siguiente, por eso ese píxel se enciende antes: si Set o Get no validan, el test lo ve. (0, 8) caería
 * fuera de la superficie, en 'guard'; la estructura que los junta existe solo para detectarlo */
static void testPixelFueraDeRangoSeIgnora(void) {
    struct {
        LedSurface surface;
        LedColor guard;
    } t;
    memset(&t, 0, sizeof(t));
    const LedColor white = {255, 255, 255};
    ledSurfaceSet(&t.surface, 0, 1, white);

    ledSurfaceSet(&t.surface, 8, 0, white);
    ledSurfaceSet(&t.surface, 0, 8, white);
    ledSurfaceSet(&t.surface, 255, 255, white);

    CHECK(t.guard.r == 0 && t.guard.g == 0 && t.guard.b == 0);
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor c = ledSurfaceGet(&t.surface, x, y);
            const uint8_t expected = (x == 0 && y == 1) ? 255 : 0;
            CHECK(c.r == expected && c.g == expected && c.b == expected);
        }
    }

    const LedColor outside[] = {
        ledSurfaceGet(&t.surface, 8, 0),
        ledSurfaceGet(&t.surface, 0, 8),
        ledSurfaceGet(&t.surface, 255, 255),
    };
    for (uint32_t i = 0; i < sizeof(outside) / sizeof(outside[0]); i++) {
        CHECK(outside[i].r == 0 && outside[i].g == 0 && outside[i].b == 0);
    }
}

/* Hipótesis de mapeo físico de la CJMCU-8*8 (se confirma a ojo en la placa, story 16): sin serpentina, todas las
 * columnas en el mismo sentido. El LED 0 está en (0, 0) y el LED crece con y en cada columna. Tabla escrita a mano,
 * indexada [x][y]: si la placa la desmiente, cambian esta tabla y ledSurfaceLedIndex */
static void testMapeoSinSerpentinaPorColumnas(void) {
    static const uint8_t expected[LED_SURFACE_WIDTH][LED_SURFACE_HEIGHT] = {
        { 0,  1,  2,  3,  4,  5,  6,  7},
        { 8,  9, 10, 11, 12, 13, 14, 15},
        {16, 17, 18, 19, 20, 21, 22, 23},
        {24, 25, 26, 27, 28, 29, 30, 31},
        {32, 33, 34, 35, 36, 37, 38, 39},
        {40, 41, 42, 43, 44, 45, 46, 47},
        {48, 49, 50, 51, 52, 53, 54, 55},
        {56, 57, 58, 59, 60, 61, 62, 63},
    };
    for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
        for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
            CHECK(ledSurfaceLedIndex(x, y) == expected[x][y]);
        }
    }
}

/* 64 LED × 24 bits × 3 transferencias de 1 byte por bit (alto, dato y bajo) = 4608 bytes (spec 3.15 y Bloque 12). Es
 * el tamaño que el DMA reparte en dos tramos de LLI, porque supera el máximo de 4095 por tramo */
static void testElBufferTiene4608Bytes(void) {
    CHECK(LED_ENCODED_BYTES == 4608);
}

/* Un bit en uno se codifica como alto, alto, bajo (T1H = 800 ns y T1L = 400 ns, spec 3.15), y cada byte vale 0 o 1:
 * el nivel de P0.0. Con el verde de LED 0 en 255 se mira el último de sus 8 bits (los bytes 21 a 23 del buffer): con
 * el tope de brillo, 255 pasa a 63, y ese bit sigue en uno. El buffer se rellena con 0xAA para ver los bytes
 * que ledEncode no escribe */
static void testUnBitEnUnoSeCodificaAltoAltoBajo(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor green = {0, 255, 0};
    ledSurfaceSet(&s, 0, 0, green);

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);

    CHECK(buffer[21] == 1);
    CHECK(buffer[22] == 1);
    CHECK(buffer[23] == 0);
}

/* Un bit en cero se codifica como alto, bajo, bajo (T0H = 400 ns y T0L = 800 ns, spec 3.15). Con el píxel apagado
 * los 24 bits de LED 0 son ceros, con o sin tope de brillo */
static void testUnBitEnCeroSeCodificaAltoBajoBajo(void) {
    LedSurface s;
    ledSurfaceInit(&s);

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);

    for (uint8_t bit = 0; bit < LED_BITS_PER_LED; bit++) {
        CHECK(buffer[bit * 3] == 1);
        CHECK(buffer[bit * 3 + 1] == 0);
        CHECK(buffer[bit * 3 + 2] == 0);
    }
}

/* Nivel de datos (byte del medio de la terna) del bit menos significativo del canal 'slot' del LED 0, donde slot 0 es el
 * primer byte de color que recibe el LED, el 1, el segundo y el 2, el tercero */
static uint8_t lastBitOfSlot(const uint8_t buffer[LED_ENCODED_BYTES], uint8_t slot) {
    const uint8_t bit = (uint8_t)(slot * 8 + 7);
    return buffer[bit * LED_BYTES_PER_BIT + 1];
}

/* El LED recibe verde, rojo y azul, en ese orden (spec 3.15). Con un solo canal en 255 (el tope de brillo lo deja en 63,
 * con el último bit en uno), el único canal con ese bit en uno es el que le toca: verde primero, rojo segundo y
 * azul tercero */
static void testElOrdenDeColorEsGRB(void) {
    const LedColor colors[3] = {{0, 255, 0}, {255, 0, 0}, {0, 0, 255}};   /* verde, rojo, azul */
    for (uint8_t slot = 0; slot < 3; slot++) {
        LedSurface s;
        ledSurfaceInit(&s);
        ledSurfaceSet(&s, 0, 0, colors[slot]);

        uint8_t buffer[LED_ENCODED_BYTES];
        memset(buffer, 0xAA, sizeof(buffer));
        ledEncode(&s, buffer);

        for (uint8_t other = 0; other < 3; other++) {
            CHECK(lastBitOfSlot(buffer, other) == (other == slot ? 1 : 0));
        }
    }
}

/* Dentro de cada byte de color se envía primero el bit más significativo (spec 3.15). Con el verde de LED 0 en 128, el
 * tope de brillo lo deja en 32 (solo el bit 5): el único bit en uno de los 24 es el tercero de la cadena (posición 2,
 * porque el bit 7 va primero y el 6, segundo). Si se enviara el menos significativo primero, el uno caería en la
 * posición 5 */
static void testElBitMasSignificativoVaPrimero(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor green = {0, 128, 0};
    ledSurfaceSet(&s, 0, 0, green);

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);

    for (uint8_t bit = 0; bit < LED_BITS_PER_LED; bit++) {
        CHECK(buffer[bit * LED_BYTES_PER_BIT + 1] == (bit == 2 ? 1 : 0));
    }
}

/* Valor de 8 bits que el buffer le da al canal 'slot' del LED 'led' de la cadena (0 es el primer byte de color, 1 el
 * segundo y 2 el tercero), leído de los bytes de datos de sus 8 bits, el más significativo primero */
static uint8_t ledChannelValue(const uint8_t buffer[LED_ENCODED_BYTES], uint8_t led, uint8_t slot) {
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        const uint32_t bit = (uint32_t)led * LED_BITS_PER_LED + slot * 8u + i;
        value = (uint8_t)((value << 1) | buffer[bit * LED_BYTES_PER_BIT + 1]);
    }
    return value;
}

/* Lo mismo para el LED 0 */
static uint8_t channelValue(const uint8_t buffer[LED_ENCODED_BYTES], uint8_t slot) {
    return ledChannelValue(buffer, 0, slot);
}

/* El firmware limita el brillo al 25 % (spec 3.6): cada canal sale con el 25 % de su valor, truncado a entero.
 * 255 pasa a 63 (el 25 % es 63,75), 100 a 25 y 0 queda en 0. Los valores esperados están escritos a mano. Se
 * prueba cada valor en canales distintos; el orden de los canales es verde, rojo y azul */
static void testElBrilloSeLimitaAl25PorCiento(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor first = {255, 100, 0};    /* rojo 255, verde 100, azul 0 */
    ledSurfaceSet(&s, 0, 0, first);

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);
    CHECK(channelValue(buffer, 0) == 25);    /* verde: 100 → 25 */
    CHECK(channelValue(buffer, 1) == 63);    /* rojo: 255 → 63 */
    CHECK(channelValue(buffer, 2) == 0);     /* azul: 0 → 0 */

    const LedColor second = {0, 255, 100};   /* rojo 0, verde 255, azul 100 */
    ledSurfaceSet(&s, 0, 0, second);
    ledEncode(&s, buffer);
    CHECK(channelValue(buffer, 0) == 63);    /* verde: 255 → 63 */
    CHECK(channelValue(buffer, 1) == 0);     /* rojo: 0 → 0 */
    CHECK(channelValue(buffer, 2) == 25);    /* azul: 100 → 25 */
}

/* Peor caso de consumo (spec 3.6): los 64 píxeles en blanco (255, 255, 255). Cada canal de cada uno de los 64 LED sale
 * en 63, el tope del 25 %, y ninguno en 255 */
static void testElBlancoCompletoNoSuperaElTopePorCanal(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    const LedColor white = {255, 255, 255};
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            ledSurfaceSet(&s, x, y, white);
        }
    }

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);

    for (uint8_t led = 0; led < LED_SURFACE_WIDTH * LED_SURFACE_HEIGHT; led++) {
        for (uint8_t slot = 0; slot < 3; slot++) {
            CHECK(ledChannelValue(buffer, led, slot) == 63);
        }
    }
}

/* El píxel (x, y) sale en el LED que le toca en la cadena, y cada LED ocupa 72 bytes (24 bits × 3): el LED k empieza
 * en el byte 72 · k. Los LED esperados salen de la tabla sin serpentina (test de mapeo), escritos a mano; el (3, 2) es
 * el 26 porque la columna 3 empieza en 24 y el y = 2 es el tercero de la columna. Con el píxel en blanco, el LED
 * correcto tiene el verde en 63 y los otros 63 LED quedan apagados */
static void testElLedKSeCodificaEnElOffsetKPor72(void) {
    static const struct {
        uint8_t x;
        uint8_t y;
        uint8_t led;
    } cases[] = {
        {0, 0, 0}, {1, 0, 8}, {0, 7, 7}, {3, 2, 26}, {7, 0, 56}, {7, 7, 63},
    };

    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        LedSurface s;
        ledSurfaceInit(&s);
        const LedColor white = {255, 255, 255};
        ledSurfaceSet(&s, cases[i].x, cases[i].y, white);

        uint8_t buffer[LED_ENCODED_BYTES];
        memset(buffer, 0xAA, sizeof(buffer));
        ledEncode(&s, buffer);

        /* El verde de 63 (0b00111111) tiene el bit 7 y el 6 en cero y el 5 en uno: posiciones 0 y 1 en cero y 2 en uno */
        const uint32_t base = (uint32_t)cases[i].led * 72;
        CHECK(buffer[base + 0 * 3 + 1] == 0);
        CHECK(buffer[base + 1 * 3 + 1] == 0);
        CHECK(buffer[base + 2 * 3 + 1] == 1);

        for (uint8_t led = 0; led < LED_SURFACE_WIDTH * LED_SURFACE_HEIGHT; led++) {
            if (led == cases[i].led) {
                continue;
            }
            for (uint8_t slot = 0; slot < 3; slot++) {
                CHECK(ledChannelValue(buffer, led, slot) == 0);
            }
        }
    }
}

/* Cada byte del buffer es el nivel de P0.0 y nada más: vale 0 o 1, y los otros 7 bits quedan en cero. Así el DMA no
 * depende de FIOMASK0 para no tocar P0.1 a P0.7 (spec 3.15). Se codifica una superficie con colores distintos en los
 * 64 píxeles; el relleno de 0xAA delata cualquier byte sin escribir */
static void testLosBytesCodificadosSoloUsanElBit0(void) {
    LedSurface s;
    ledSurfaceInit(&s);
    for (uint8_t y = 0; y < LED_SURFACE_HEIGHT; y++) {
        for (uint8_t x = 0; x < LED_SURFACE_WIDTH; x++) {
            const LedColor color = {(uint8_t)(x * 36), (uint8_t)(y * 36), (uint8_t)(255 - x * 20 - y * 20)};
            ledSurfaceSet(&s, x, y, color);
        }
    }

    uint8_t buffer[LED_ENCODED_BYTES];
    memset(buffer, 0xAA, sizeof(buffer));
    ledEncode(&s, buffer);

    uint32_t outsideBit0 = 0;
    for (uint32_t i = 0; i < LED_ENCODED_BYTES; i++) {
        if ((buffer[i] & 0xFE) != 0) {
            outsideBit0++;
        }
    }
    CHECK(outsideBit0 == 0);
}

int main(void) {
    testSuperficieArrancaApagada();
    testSetPixelGuardaElColorEnEsaPosicion();
    testPixelFueraDeRangoSeIgnora();
    testMapeoSinSerpentinaPorColumnas();
    testElBufferTiene4608Bytes();
    testUnBitEnUnoSeCodificaAltoAltoBajo();
    testUnBitEnCeroSeCodificaAltoBajoBajo();
    testElOrdenDeColorEsGRB();
    testElBitMasSignificativoVaPrimero();
    testElBrilloSeLimitaAl25PorCiento();
    testElBlancoCompletoNoSuperaElTopePorCanal();
    testElLedKSeCodificaEnElOffsetKPor72();
    testLosBytesCodificadosSoloUsanElBit0();
    if (failures > 0) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
