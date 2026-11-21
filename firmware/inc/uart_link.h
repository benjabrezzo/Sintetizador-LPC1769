/* UART0 hacia la aplicación (spec 3.14). Módulo de hardware: usa el driver de la UART. La ISR solo copia los bytes
 * que llegan a un buffer circular; el parser de uart_protocol los procesa fuera de la ISR, en el loop principal. */
#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>
#include "ring_buffer.h"

/* Buffer circular de recepción. Global a propósito, para leer uartRxRing.dropped en la vista Expressions con la CPU
 * suspendida: bytes que llegaron con el buffer lleno (debe quedar en 0). Lo escribe la ISR y lo lee el loop. */
extern RingBuffer uartRxRing;

/* Configura UART0 (TXD0 P0.2, RXD0 P0.3) a UART_BAUD_RATE 8N1, con la FIFO de recepción y la interrupción de
 * recepción en UART_IRQ_PRIORITY. Desde acá, cada byte que llega termina en uartRxRing */
void uartLinkInit(void);

/* Saca el byte más antiguo recibido en *byte. Devuelve 1 si había uno, 0 si no había ninguno */
uint8_t uartLinkReadByte(uint8_t *byte);

/* Envía length bytes por UART0 y espera a que entren en la FIFO de transmisión. Se llama desde el loop principal */
void uartLinkSend(const uint8_t *data, uint32_t length);

#endif /* UART_LINK_H */
