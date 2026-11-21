#include "ring_buffer.h"

void ringBufferInit(RingBuffer *r) {
    r->head = 0;
    r->tail = 0;
    r->dropped = 0;
}

uint8_t ringBufferPush(RingBuffer *r, uint8_t byte) {
    if ((uint16_t)(r->head - r->tail) >= UART_RX_BUFFER_SIZE) {
        r->dropped++;
        return 0;
    }
    r->data[r->head % UART_RX_BUFFER_SIZE] = byte;
    r->head++;
    return 1;
}

uint8_t ringBufferPop(RingBuffer *r, uint8_t *byte) {
    if (r->tail == r->head) {
        return 0;
    }
    *byte = r->data[r->tail % UART_RX_BUFFER_SIZE];
    r->tail++;
    return 1;
}
