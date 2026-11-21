/* Contador de ciclos de CPU para medir duraciones en la placa (spec 3.12; Bloques 5, 10 y 11). Módulo de hardware.
 * Usa el Timer0 libre, sin interrupción: no da la vuelta durante una ISR, a diferencia de SysTick (1 ms). */
#ifndef CYCLE_COUNTER_H
#define CYCLE_COUNTER_H

#include <stdint.h>

/* Arranca el Timer0 con PR = 0. Con el PCLK en CCLK/4 (lo deja el driver) cuenta a 25 MHz: un tick cada 4 ciclos de la
 * CPU. Se llama una vez, antes de audioOutInit: la ISR del DMA ya lo lee desde su primer bloque. */
void cycleCounterInit(void);

/* Ciclos de CPU transcurridos, módulo 2^32. Resolución de 4 ciclos. Una duración es la resta sin signo de dos lecturas,
 * y vale hasta unos 43 s (el timer da la vuelta a los 171 s, y la cuenta en ciclos pierde sus dos bits altos). */
uint32_t cycleCounterNow(void);

#endif /* CYCLE_COUNTER_H */
