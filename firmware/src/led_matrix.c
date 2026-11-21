#include <cr_section_macros.h>
#include "led_matrix.h"
#include "config.h"
#include "cycle_counter.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

/* El buffer son 4608 transferencias y el DMA admite como máximo 4095 por tramo: se reparten en dos mitades iguales. La
 * primera sale de los registros del canal y la segunda, de una LLI */
#define MATRIX_HALF_BYTES  (LED_ENCODED_BYTES / 2)

/* El destino se escribe directo en el canal 1 (ver ledMatrixSend) */
_Static_assert(MATRIX_DMA_CHANNEL == 1, "ledMatrixSend usa LPC_GPDMACH1");

/* Lo que lee el DMA va en AHB (RAM2), como el audio (CLAUDE.md, "Memoria"). La separación en el banco 1 depende de la
 * configuración de memoria del IDE: se comprueba en la placa */
__BSS(RAM2) static uint8_t matrixBuf[LED_ENCODED_BYTES];
__BSS(RAM2) static GPDMA_LLI_T matrixLli;

/* 1 desde que ledMatrixSend arranca una transmisión hasta que ledMatrixBusy ve que terminó. Solo la tocan las dos
 * funciones, desde el loop principal */
static uint8_t transmitting = 0;

/* Lectura de cycleCounterNow cuando ledMatrixBusy vio terminar la última transmisión (o en ledMatrixInit, antes de la
 * primera). Es posterior al fin real: el bajo medido desde acá nunca es mayor que el real */
static uint32_t lastEndCycles = 0;

void ledMatrixInit(void) {
    /* Reposo: P0.0 como salida en bajo (el latch está en 0 desde el reset). Tras un reset el pin es entrada con pull-up y
     * DIN quedaría en alto (spec 3.15) */
    GPIO_SetDir(MATRIX_PORT, 1u << MATRIX_PIN, GPIO_OUTPUT);
    GPIO_ClearPins(MATRIX_PORT, 1u << MATRIX_PIN);
    /* FIOMASK0: los bits en 1 no se modifican. Solo P0.0 queda libre. Afecta a toda escritura en P0.0 a P0.7 */
    GPIO_SetMask(MATRIX_PORT, 0xFFu & ~(1u << MATRIX_PIN), ENABLE);

    /* Timer2 con PR = 0 (TIM_TICK con 1 deja PR en 0): cuenta a PCLK = 25 MHz y, con MR0 = 9, reinicia cada 10 cuentas
     * (400 ns). La petición de DMA no necesita la interrupción por coincidencia (UM10360 21.6.12): es otro flag */
    const TIM_TIMERCFG_T timerCfg = {TIM_TICK, 1};
    TIM_InitTimer(LPC_TIM2, &timerCfg);
    TIM_Disable(LPC_TIM2);

    TIM_MATCHCFG_T matchCfg = {0};
    matchCfg.channel    = TIM_MATCH_0;
    matchCfg.intEn      = DISABLE;
    matchCfg.stopEn     = DISABLE;
    matchCfg.resetEn    = ENABLE;
    matchCfg.extOpt     = TIM_NOTHING;
    matchCfg.matchValue = MATRIX_TIMER_MATCH;
    TIM_ConfigMatch(LPC_TIM2, &matchCfg);

    transmitting = 0;
    lastEndCycles = cycleCounterNow();
}

uint8_t ledMatrixBusy(void) {
    if (!transmitting) {
        return 0;
    }
    if (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, (GPDMA_CH)MATRIX_DMA_CHANNEL) == SET) {
        return 1;
    }
    /* Terminó: el último byte de la tanda es un bajo, así que P0.0 queda en 0. Con el timer apagado no se generan más
     * peticiones hasta el próximo envío */
    TIM_Disable(LPC_TIM2);
    transmitting = 0;
    lastEndCycles = cycleCounterNow();
    return 0;
}

uint8_t ledMatrixSend(const LedSurface *surface) {
    if (ledMatrixBusy()) {
        return 0;
    }
    if ((uint32_t)(cycleCounterNow() - lastEndCycles) < MATRIX_RESET_CYCLES) {
        return 0;
    }

    ledEncode(surface, matrixBuf);

    /* Segundo tramo: mismos anchos y ráfagas que el primero, sin interrupción y sin otra LLI a continuación */
    matrixLli.srcAddr = (uint32_t)&matrixBuf[MATRIX_HALF_BYTES];
    matrixLli.dstAddr = (uint32_t)&LPC_GPIO0->FIOPIN0;
    matrixLli.nextLLI = 0;
    matrixLli.control = GPDMA_DMACCxControl_TransferSize(MATRIX_HALF_BYTES)
                      | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                      | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                      | GPDMA_DMACCxControl_SWidth(GPDMA_BYTE)
                      | GPDMA_DMACCxControl_DWidth(GPDMA_BYTE)
                      | GPDMA_DMACCxControl_SI;

    GPDMA_Channel_CFG_T dmaCfg = {0};
    dmaCfg.channelNum    = (GPDMA_CH)MATRIX_DMA_CHANNEL;
    dmaCfg.transferSize  = MATRIX_HALF_BYTES;
    dmaCfg.type          = GPDMA_M2P;
    dmaCfg.srcMemAddr    = (uint32_t)matrixBuf;
    dmaCfg.dstConn       = GPDMA_MAT2_0;          /* pide el Timer2; el destino real se corrige abajo */
    dmaCfg.src.width     = GPDMA_BYTE;
    dmaCfg.src.burst     = GPDMA_BSIZE_1;
    dmaCfg.src.increment = ENABLE;
    dmaCfg.dst.width     = GPDMA_BYTE;
    dmaCfg.dst.burst     = GPDMA_BSIZE_1;
    dmaCfg.dst.increment = DISABLE;
    dmaCfg.intTC         = DISABLE;               /* sin interrupción: DMA_IRQHandler es el del audio */
    dmaCfg.intErr        = DISABLE;
    dmaCfg.linkedList    = (uint32_t)&matrixLli;
    if (GPDMA_SetupChannel(&dmaCfg) == ERROR) {
        return 0;
    }
    /* El driver apunta M2P con MAT2.0 a MR0: el destino real es el GPIO */
    LPC_GPDMACH1->DMACCDestAddr = (uint32_t)&LPC_GPIO0->FIOPIN0;

    /* Con el timer apagado y en cero no hay peticiones: la primera llega 400 ns después de encenderlo, con P0.0 en
     * bajo, y el primer pulso sale completo. La petición pendiente se limpia escribiendo en T2IR (UM10360 21.6.1) */
    TIM_ResetCounter(LPC_TIM2);
    TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
    GPDMA_ChannelStart((GPDMA_CH)MATRIX_DMA_CHANNEL);
    TIM_Enable(LPC_TIM2);

    transmitting = 1;
    return 1;
}
