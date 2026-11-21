/* Antirrebote de las ocho teclas (spec 3.11). Lógica pura: sin dependencias del hardware. */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

#define DEBOUNCE_NUM_KEYS  8

typedef struct {
    uint8_t stable;                          /* estado filtrado: bit k = 1 si la tecla k + 1 está apretada */
    uint8_t counter[DEBOUNCE_NUM_KEYS];      /* lecturas seguidas que la tecla lleva distinta de su estado estable */
} Debounce;

/* Deja todas las teclas sueltas */
void debounceInit(Debounce *d);

/* Procesa una lectura (una por ms). raw: bit k = 1 si la tecla k + 1 está apretada ahora. Devuelve en
 * pressedEvents y releasedEvents los bits de las teclas que acaban de cambiar de estado (pulsación y
 * liberación) tras estar estables el tiempo del antirrebote. */
void debounceUpdate(Debounce *d, uint8_t raw, uint8_t *pressedEvents, uint8_t *releasedEvents);

#endif /* DEBOUNCE_H */
