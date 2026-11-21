/* Selección de la forma de onda con el pulsador de EINT1 (spec 3.8 y 3.11). Lógica pura: sin dependencias
 * del hardware. La hora entra por parámetro. */
#ifndef WAVE_SELECT_H
#define WAVE_SELECT_H

#include <stdint.h>

/* Identificadores de onda, en el orden de la rotación de la spec 3.8. Cada voz guarda el de su onda al
 * arrancar la nota, y el audio lo usa como índice de su arreglo de tablas. */
enum {
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_TRIANGLE,
    WAVE_SINE,
    WAVE_PYTHON,      /* la quinta: llega por UART (story 13) */
    NUM_WAVES
};

typedef struct {
    uint8_t current;        /* identificador de la onda activa */
    uint8_t needsRelease;   /* 1 = hubo una pulsación aceptada y el pulsador todavía no se soltó */
    uint8_t highMs;         /* lecturas seguidas con el pin en alto, hasta DEBOUNCE_MS */
    uint8_t hasAccepted;    /* 0 hasta la primera pulsación aceptada: todavía no hay una anterior */
    uint32_t lastAcceptedMs; /* hora de la última pulsación aceptada */
} WaveSelect;

/* La placa arranca en senoide */
void waveSelectInit(WaveSelect *s);

/* Onda activa: la que toman las notas que arrancan ahora */
uint8_t waveSelectCurrent(const WaveSelect *s);

/* Flanco descendente de EINT1, a la hora nowMs (ms). Si la pulsación se acepta, avanza la onda y devuelve 1;
 * si no, devuelve 0. pythonLoaded = 1 si ya llegó una onda de Python. */
uint8_t waveSelectPress(WaveSelect *s, uint32_t nowMs, uint8_t pythonLoaded);

/* Lectura del nivel de P2.11 (1 = alto, pulsador suelto), una por ms desde SysTick. Con DEBOUNCE_MS lecturas
 * seguidas en alto el pulsador se da por soltado y la pulsación siguiente puede aceptarse. */
void waveSelectSample(WaveSelect *s, uint8_t pinHigh);

#endif /* WAVE_SELECT_H */
