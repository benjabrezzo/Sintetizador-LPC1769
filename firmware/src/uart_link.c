#include "uart_link.h"
#include "config.h"
#include "lpc17xx_uart.h"

RingBuffer uartRxRing;

void uartLinkInit(void) {
    ringBufferInit(&uartRxRing);

    UART_PinConfig(UART_TX0_P0_2);
    UART_PinConfig(UART_RX0_P0_3);

    const UART_CFG_T uartCfg = {
        .baudRate = UART_BAUD_RATE,
        .parity   = UART_PARITY_NONE,
        .dataBits = UART_DBITS_8,
        .stopBits = UART_STOPBIT_1,
    };
    UART_Init(UART0, &uartCfg);              /* deja la FIFO apagada y el transmisor deshabilitado */

    /* La FIFO de recepción guarda 16 bytes y avisa al llegar a 4: quedan 12 bytes de margen (unos 1,04 ms a
     * 115200 baudios, 87 µs por byte) mientras la ISR del audio o SysTick, más prioritarias, ocupan la CPU. El
     * timeout de caracteres avisa los bytes que queden por debajo del nivel */
    const UART_FIFO_CFG_T fifoCfg = {
        .resetRxBuf = ENABLE,
        .resetTxBuf = ENABLE,
        .dmaMode    = DISABLE,
        .level      = UART_FIFO_TRGLEV1,
    };
    UART_FIFOConfig(UART0, &fifoCfg);
    UART_TxEnable(UART0);

    UART_IntConfig(UART0, UART_INT_RBR, ENABLE);    /* dato recibido y timeout de caracteres */
    NVIC_SetPriority(UART0_IRQn, UART_IRQ_PRIORITY);
    NVIC_EnableIRQ(UART0_IRQn);
}

/* Vacía la FIFO de recepción hacia el buffer circular. Se lee hasta que no queden datos y no solo hasta el nivel de
 * disparo: así se atienden juntos el aviso por nivel y el de timeout, y la interrupción se limpia sola. El parser
 * no corre acá (spec 5): la ISR solo copia bytes */
void UART0_IRQHandler(void) {
    while (UART_GetLineStatus(UART0) & UART_LSR_RDR) {
        ringBufferPush(&uartRxRing, UART_ReceiveByte(UART0));
    }
}

uint8_t uartLinkReadByte(uint8_t *byte) {
    return ringBufferPop(&uartRxRing, byte);
}

void uartLinkSend(const uint8_t *data, uint32_t length) {
    UART_Send(UART0, data, length, BLOCKING);
}
