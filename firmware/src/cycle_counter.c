#include "cycle_counter.h"
#include "lpc17xx_timer.h"

#define CYCLES_PER_TICK  4      /* PCLK = CCLK/4: lo deja así TIM_InitTimer */

void cycleCounterInit(void) {
    const TIM_TIMERCFG_T cfg = {TIM_TICK, 1};      /* prescaleValue 1 → PR = 0: un tick por período de PCLK */
    TIM_InitTimer(LPC_TIM0, &cfg);
    TIM_Enable(LPC_TIM0);
}

uint32_t cycleCounterNow(void) {
    return TIM_ReadTimer(LPC_TIM0) * CYCLES_PER_TICK;
}
