/* Buffer circular de bytes para la recepción de la UART (spec 3.14). Lógica pura: sin dependencias del hardware.
 * Un productor (la ISR de la UART) y un consumidor (el loop principal): cada uno escribe solo su contador. */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include "config.h"      /* UART_RX_BUFFER_SIZE */

typedef struct {
    uint8_t data[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;     /* bytes que entraron desde el arranque (solo lo escribe el productor) */
    volatile uint16_t tail;     /* bytes que salieron desde el arranque (solo lo escribe el consumidor) */
    volatile uint32_t dropped;  /* bytes descartados por estar lleno el buffer (solo lo escribe el productor) */
} RingBuffer;

/* Deja el buffer vacío */
void ringBufferInit(RingBuffer *r);

/* Guarda un byte. Devuelve 1 si lo guardó. Con el buffer lleno descarta el byte nuevo (los que ya están no se pisan),
 * lo suma a dropped y devuelve 0 */
uint8_t ringBufferPush(RingBuffer *r, uint8_t byte);

/* Saca el byte más antiguo en *byte. Devuelve 1 si había uno, 0 si el buffer estaba vacío (*byte no cambia) */
uint8_t ringBufferPop(RingBuffer *r, uint8_t *byte);

#endif /* RING_BUFFER_H */
