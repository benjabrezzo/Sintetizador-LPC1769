/* Parser de las tramas UART (spec 3.14, docs/protocolo.md). Lógica pura: sin dependencias del hardware.
 * Recibe los bytes de a uno, porque una trama UPLOAD_WAVE (517 B) no entra en el buffer circular de 256 B.
 * Entiende UPLOAD_WAVE y SET_EQ (story 14). */
#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include "tables.h"      /* WAVE_SIZE */

#define UART_PAYLOAD_WAVE_BYTES  (2 * WAVE_SIZE)     /* UPLOAD_WAVE: 256 muestras int16_t, 2 bytes cada una */
#define UART_EQ_COEF_COUNT       15                  /* SET_EQ: 3 bandas × (b0, b1, b2, a1, a2), en Q2.30 */
#define UART_PAYLOAD_EQ_BYTES    (4 * UART_EQ_COEF_COUNT)    /* SET_EQ: 15 int32_t, 4 bytes cada uno */

/* Trama ACK de docs/protocolo.md: 0xA5 | 0x06 | LEN = 0 | CHK. La placa la envía cuando aceptó una onda */
#define UART_ACK_FRAME_BYTES  5
extern const uint8_t uartAckFrame[UART_ACK_FRAME_BYTES];

/* Qué devuelve uartParserFeed */
enum {
    UART_PARSER_NONE,         /* nada para avisar: la trama sigue o se descartó */
    UART_PARSER_WAVE_READY,   /* llegó una trama UPLOAD_WAVE completa y con el checksum correcto */
    UART_PARSER_EQ_READY      /* llegó una trama SET_EQ completa y con el checksum correcto */
};

/* Estado interno del parser, en el orden en que llegan los campos de la trama */
enum {
    UART_STATE_START,         /* esperando el 0xA5 */
    UART_STATE_CMD,
    UART_STATE_LEN_L,
    UART_STATE_LEN_H,
    UART_STATE_PAYLOAD,
    UART_STATE_CHK
};

typedef struct {
    uint8_t state;            /* UART_STATE_* */
    uint8_t cmd;              /* CMD de la trama en curso */
    uint16_t len;             /* LEN de la trama en curso */
    uint16_t received;        /* bytes del payload que ya llegaron */
    uint8_t sum;              /* suma de 8 bits de CMD, LEN_L, LEN_H y el payload que ya llegaron */
} UartParser;

/* Deja el parser esperando una trama nueva */
void uartParserInit(UartParser *p);

/* Procesa un byte recibido. El payload de UPLOAD_WAVE se escribe en waveBuf (WAVE_SIZE muestras int16_t, little-endian
 * en el cable) a medida que llega; el de SET_EQ, en eqBuf (UART_EQ_COEF_COUNT int32_t, little-endian en el cable, en el
 * orden de docs/protocolo.md). Ninguno de los dos es lo que usa el audio, así que una trama que se descarta los deja
 * con basura sin afectar el sonido. Devuelve UART_PARSER_WAVE_READY o UART_PARSER_EQ_READY solo con el último byte de
 * una trama válida de ese comando. */
uint8_t uartParserFeed(UartParser *p, uint8_t byte, int16_t *waveBuf, int32_t *eqBuf);

#endif /* UART_PROTOCOL_H */
