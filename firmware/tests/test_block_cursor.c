/* Tests de la regla de bloques del audio (spec 7.4, Bloque 2): qué mitad del doble buffer se llena.
 * Lógica pura: se prueba con un llenado simulado, sin hardware. Desde el Bloque 4 la posición de
 * lectura es la fase del oscilador DDS, que se conserva entre bloques: el llenado simulado usa el
 * oscilador sobre una tabla rampa (cada muestra vale su índice), para que cada bloque sea distinguible. */
#include <stdio.h>
#include <stdint.h>
#include "config.h"
#include "tables.h"
#include "block_cursor.h"
#include "oscillator.h"
#include "test_check.h"

/* 10 bloques = 640 muestras = 2,5 períodos: el recorrido cruza el fin de la tabla más de una vez */
#define BLOCKS_TO_CHECK 10

/* ΔP de una posición de la tabla por muestra (Q16.16): con la rampa, el flujo es 0, 1, 2... */
#define ONE_INDEX_STEP  (1u << 16)

static int16_t ramp[WAVE_SIZE];

static void buildRamp(void) {
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        ramp[i] = (int16_t)i;
    }
}

/* La CPU solo escribe en la mitad que el DMA no está leyendo. El DMA simulado sigue al hardware:
 * tras el precargado arranca leyendo la mitad 0 y, cada vez que termina una mitad (fin de
 * transferencia), pasa a la otra. En cada fin de transferencia la ISR pide la mitad a llenar. */
static void testLaCpuSoloEscribeLaMitadQueElDmaNoLee(void) {
    BlockCursor cursor;
    blockCursorInit(&cursor);

    (void)blockCursorNext(&cursor);               /* precargado de las dos mitades: */
    (void)blockCursorNext(&cursor);               /* el DMA todavía no corre */

    uint8_t dmaReads = 0;                         /* el DMA arranca leyendo la mitad 0 */
    for (uint32_t event = 0; event < BLOCKS_TO_CHECK; event++) {
        dmaReads ^= 1;                            /* terminó una mitad y empezó la otra */
        uint8_t half = blockCursorNext(&cursor);
        CHECK(half <= 1);                         /* existe esa mitad */
        CHECK(half != dmaReads);                  /* y no es la que se está reproduciendo */
    }
}

/* Lo que escribe la ISR del DMA en una mitad: un bloque de muestras del oscilador, que sigue desde
 * la fase donde quedó el bloque anterior */
static void fillBlock(int16_t *dst, Oscillator *osc) {
    for (uint32_t i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        dst[i] = oscNextSample(osc, ramp);
    }
}

/* Lo que lee el DMA es la tabla en orden, sin bloques repetidos ni salteados. El DMA simulado
 * reproduce las mitades en orden (0, 1, 0, 1...) y, al terminar cada una, la ISR llena la que
 * acaba de terminar. Con la rampa, un bloque repetido o salteado rompe la cuenta 0, 1, 2...
 * módulo 256. */
static void testElFlujoQueLeeElDmaNoTieneSaltos(void) {
    int16_t halves[2][AUDIO_BLOCK_SIZE] = {{0}};
    BlockCursor cursor;
    blockCursorInit(&cursor);
    Oscillator osc;
    oscInit(&osc, ONE_INDEX_STEP);

    for (uint32_t i = 0; i < 2; i++) {            /* precargado de las dos mitades */
        const uint8_t half = blockCursorNext(&cursor);
        fillBlock(halves[half], &osc);
    }

    uint32_t played = 0;                          /* muestras reproducidas hasta ahora */
    for (uint32_t block = 0; block < BLOCKS_TO_CHECK; block++) {
        const uint8_t dmaReads = block % 2;
        for (uint32_t i = 0; i < AUDIO_BLOCK_SIZE; i++) {
            CHECK(halves[dmaReads][i] == (int)(played % WAVE_SIZE));
            played++;
        }

        /* fin de transferencia: la ISR llena la mitad que acaba de terminar */
        const uint8_t half = blockCursorNext(&cursor);
        fillBlock(halves[half], &osc);
    }
}

int main(void) {
    buildRamp();
    testLaCpuSoloEscribeLaMitadQueElDmaNoLee();
    testElFlujoQueLeeElDmaNoTieneSaltos();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
