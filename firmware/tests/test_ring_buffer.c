/* Tests del buffer circular de recepción de la UART (spec 3.14, story 13). La ISR de la UART guarda los bytes
 * que llegan y el loop principal los saca en el mismo orden. */
#include <stdio.h>
#include <stdint.h>
#include "ring_buffer.h"
#include "test_check.h"

/* Los bytes salen en el orden en que entraron y, cuando se vacía, pop avisa que no hay más */
static void testDevuelveLosBytesEnElOrdenQueEntraron(void) {
    RingBuffer r;
    ringBufferInit(&r);
    uint8_t byte = 0;

    CHECK(ringBufferPop(&r, &byte) == 0);    /* recién creado: vacío */

    CHECK(ringBufferPush(&r, 0xA5) == 1);
    CHECK(ringBufferPush(&r, 0x01) == 1);
    CHECK(ringBufferPush(&r, 0xFF) == 1);

    CHECK(ringBufferPop(&r, &byte) == 1);
    CHECK(byte == 0xA5);
    CHECK(ringBufferPop(&r, &byte) == 1);
    CHECK(byte == 0x01);
    CHECK(ringBufferPop(&r, &byte) == 1);
    CHECK(byte == 0xFF);
    CHECK(ringBufferPop(&r, &byte) == 0);    /* ya no quedan */
}

/* El productor y el consumidor avanzan juntos y pasan varias veces por el final del arreglo, con 100 bytes en vuelo.
 * Se mandan 1000 bytes en total, más de tres vueltas del buffer de 256: cada byte sale tal como entró */
static void testDaLaVueltaSinPerderBytes(void) {
    RingBuffer r;
    ringBufferInit(&r);
    uint8_t byte = 0;

    uint32_t pushed = 0;
    uint32_t popped = 0;
    for (; pushed < 100; pushed++) {
        CHECK(ringBufferPush(&r, (uint8_t)(pushed * 7)) == 1);
    }
    while (popped < 1000) {
        CHECK(ringBufferPop(&r, &byte) == 1);
        CHECK(byte == (uint8_t)(popped * 7));
        popped++;
        if (pushed < 1000) {
            CHECK(ringBufferPush(&r, (uint8_t)(pushed * 7)) == 1);
            pushed++;
        }
    }
    CHECK(ringBufferPop(&r, &byte) == 0);
}

/* Con el buffer lleno (256 bytes sin sacar) el byte nuevo se descarta, sin pisar a los viejos, y se cuenta en
 * dropped. Al sacar uno vuelve a haber lugar */
static void testLlenoDescartaElByteNuevoYLoCuenta(void) {
    RingBuffer r;
    ringBufferInit(&r);
    uint8_t byte = 0;

    for (uint32_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
        CHECK(ringBufferPush(&r, (uint8_t)i) == 1);
    }
    CHECK(r.dropped == 0);

    CHECK(ringBufferPush(&r, 0xEE) == 0);
    CHECK(ringBufferPush(&r, 0xEE) == 0);
    CHECK(r.dropped == 2);

    for (uint32_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
        CHECK(ringBufferPop(&r, &byte) == 1);
        CHECK(byte == (uint8_t)i);              /* salen los 256 originales, ningún 0xEE */
    }
    CHECK(ringBufferPop(&r, &byte) == 0);

    CHECK(ringBufferPush(&r, 0x42) == 1);       /* vacío otra vez: hay lugar */
    CHECK(ringBufferPop(&r, &byte) == 1);
    CHECK(byte == 0x42);
}

int main(void) {
    testDevuelveLosBytesEnElOrdenQueEntraron();
    testDaLaVueltaSinPerderBytes();
    testLlenoDescartaElByteNuevoYLoCuenta();

    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
