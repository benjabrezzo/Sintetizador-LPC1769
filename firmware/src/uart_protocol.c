#include "uart_protocol.h"

#define FRAME_START  0xA5
#define CMD_UPLOAD_WAVE  0x01
#define CMD_SET_EQ       0x04

/* CHK = complemento a dos de CMD + LEN_L + LEN_H = 0x06 */
const uint8_t uartAckFrame[UART_ACK_FRAME_BYTES] = {FRAME_START, 0x06, 0x00, 0x00, 0xFA};

void uartParserInit(UartParser *p) {
    p->state = UART_STATE_START;
    p->cmd = 0;
    p->len = 0;
    p->received = 0;
    p->sum = 0;
}

/* Guarda un byte del payload: las muestras viajan con el byte bajo primero. El byte bajo escribe la muestra entera
 * y el alto la completa, así no importa qué había antes en el buffer ni el orden de bytes de la máquina */
static void storePayloadByte(int16_t *waveBuf, uint16_t index, uint8_t byte) {
    const uint16_t sample = index / 2;
    if (index % 2 == 0) {
        waveBuf[sample] = (int16_t)byte;
    } else {
        waveBuf[sample] = (int16_t)((uint16_t)waveBuf[sample] | (uint16_t)((uint16_t)byte << 8));
    }
}

/* Guarda un byte del payload de SET_EQ: cada coeficiente viaja con el byte bajo primero. El byte bajo escribe la palabra
 * entera y los otros tres la completan, así no importa qué había antes en el buffer ni el orden de bytes de la máquina */
static void storeEqByte(int32_t *eqBuf, uint16_t index, uint8_t byte) {
    const uint16_t coef = index / 4;
    const uint32_t shift = 8u * (index % 4);
    const uint32_t previous = (index % 4 == 0) ? 0u : (uint32_t)eqBuf[coef];
    eqBuf[coef] = (int32_t)(previous | ((uint32_t)byte << shift));
}

/* Largo del payload de cada comando que la placa recibe (docs/protocolo.md); 0 si el CMD no existe. Ningún comando
 * tiene un payload vacío, así que 0 sirve para marcar un CMD desconocido (también el ACK, que va de la placa a la PC) */
static uint16_t expectedPayloadBytes(uint8_t cmd) {
    switch (cmd) {
    case CMD_UPLOAD_WAVE:
        return UART_PAYLOAD_WAVE_BYTES;
    case CMD_SET_EQ:
        return UART_PAYLOAD_EQ_BYTES;
    default:
        return 0;
    }
}

uint8_t uartParserFeed(UartParser *p, uint8_t byte, int16_t *waveBuf, int32_t *eqBuf) {
    switch (p->state) {
    case UART_STATE_START:
        if (byte == FRAME_START) {
            p->state = UART_STATE_CMD;
        }
        break;
    case UART_STATE_CMD:
        p->cmd = byte;
        p->sum = byte;
        p->state = UART_STATE_LEN_L;
        break;
    case UART_STATE_LEN_L:
        p->sum = (uint8_t)(p->sum + byte);
        p->len = byte;
        p->state = UART_STATE_LEN_H;
        break;
    case UART_STATE_LEN_H:
        p->sum = (uint8_t)(p->sum + byte);
        p->len |= (uint16_t)((uint16_t)byte << 8);
        p->received = 0;
        /* Un CMD desconocido o un LEN distinto del del comando se descarta ya: el payload se escribiría fuera del
         * buffer de recepción */
        {
            const uint16_t expected = expectedPayloadBytes(p->cmd);
            p->state = (expected != 0 && p->len == expected) ? UART_STATE_PAYLOAD : UART_STATE_START;
        }
        break;
    case UART_STATE_PAYLOAD:
        if (p->cmd == CMD_SET_EQ) {
            storeEqByte(eqBuf, p->received, byte);
        } else {
            storePayloadByte(waveBuf, p->received, byte);
        }
        p->sum = (uint8_t)(p->sum + byte);
        p->received++;
        if (p->received == p->len) {
            p->state = UART_STATE_CHK;
        }
        break;
    case UART_STATE_CHK:
        p->state = UART_STATE_START;
        /* CHK es el complemento a dos de la suma: con el CHK incluido, la suma da 0 */
        if ((uint8_t)(p->sum + byte) == 0) {
            return (p->cmd == CMD_SET_EQ) ? UART_PARSER_EQ_READY : UART_PARSER_WAVE_READY;
        }
        break;
    }
    return UART_PARSER_NONE;
}
