#include "debounce.h"
#include "config.h"

/* El contador es de 8 bits y llega hasta DEBOUNCE_MS + 1: con 255 o más se desbordaría y la tecla nunca dispararía */
_Static_assert(DEBOUNCE_MS < 255, "DEBOUNCE_MS no entra en el contador de 8 bits");

void debounceInit(Debounce *d) {
    d->stable = 0;
    for (uint32_t k = 0; k < DEBOUNCE_NUM_KEYS; k++) {
        d->counter[k] = 0;
    }
}

void debounceUpdate(Debounce *d, uint8_t raw, uint8_t *pressedEvents, uint8_t *releasedEvents) {
    *pressedEvents = 0;
    *releasedEvents = 0;
    for (uint32_t k = 0; k < DEBOUNCE_NUM_KEYS; k++) {
        const uint8_t mask = (uint8_t)(1u << k);
        if ((raw & mask) != (d->stable & mask)) {
            /* La primera lectura distinta es el instante 0: a los DEBOUNCE_MS ms (una lectura más) ya es estable */
            if (++d->counter[k] == DEBOUNCE_MS + 1) {
                d->stable ^= mask;
                d->counter[k] = 0;
                if (raw & mask) {
                    *pressedEvents |= mask;
                } else {
                    *releasedEvents |= mask;
                }
            }
        } else {
            d->counter[k] = 0;
        }
    }
}
