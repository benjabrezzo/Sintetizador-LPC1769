/* Transmisión a la matriz WS2812B por Timer2 + GPDMA hacia P0.0 (spec 3.15 y Bloque 12). Módulo de hardware: sin
 * esperas y sin interrupciones propias, se maneja desde el loop principal. */
#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>
#include "led_encoder.h"

/* Deja P0.0 como salida en bajo, con FIOMASK0 = 0xFE (solo cambia P0.0), y configura el Timer2 (una petición de DMA cada
 * 400 ns, apagado hasta que haya un envío). No inicia ninguna transmisión. Se llama una vez, después de audioOutInit: no
 * llama a GPDMA_Init, que resetearía el canal 0 del audio, y también usa cycleCounterInit. */
void ledMatrixInit(void);

/* 1 si hay una transmisión en curso y 0 si terminó (o no hubo ninguna). Mira si el canal 1 del DMA sigue habilitado, sin
 * interrupción: la ISR del DMA es la del audio y no se toca. Al ver que terminó apaga el Timer2 y anota el instante, para
 * respetar el bajo de reinicio entre cuadros. Solo la llama el loop principal. */
uint8_t ledMatrixBusy(void);

/* Codifica la superficie (con el tope de brillo) e inicia su transmisión. Devuelve 1 si la inició y 0 si no: hay un envío
 * en curso o todavía no pasó el tiempo de reinicio (MATRIX_RESET_CYCLES) desde que terminó el anterior. Nunca espera: el
 * caller reintenta en la próxima vuelta del loop. La superficie se puede modificar apenas vuelve. Solo la llama el loop
 * principal. */
uint8_t ledMatrixSend(const LedSurface *surface);

#endif /* LED_MATRIX_H */
