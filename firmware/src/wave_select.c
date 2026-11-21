#include "wave_select.h"
#include "config.h"      /* DEBOUNCE_MS: los 10 ms estables de las teclas valen también para soltar el pulsador */

void waveSelectInit(WaveSelect *s) {
    s->current = WAVE_SINE;
    s->needsRelease = 0;
    s->highMs = 0;
    s->hasAccepted = 0;
    s->lastAcceptedMs = 0;
}

uint8_t waveSelectCurrent(const WaveSelect *s) {
    return s->current;
}

/* La onda que sigue a la actual en el orden de la spec 3.8. Sin onda de Python se saltea la quinta posición.
 * Después de la última se vuelve a la cuadrada. */
static uint8_t nextWave(uint8_t current, uint8_t pythonLoaded) {
    uint8_t next = (uint8_t)(current + 1);
    if (next == WAVE_PYTHON && !pythonLoaded) {
        next++;
    }
    return (next >= NUM_WAVES) ? WAVE_SQUARE : next;
}

uint8_t waveSelectPress(WaveSelect *s, uint32_t nowMs, uint8_t pythonLoaded) {
    if (s->needsRelease || (s->hasAccepted && nowMs - s->lastAcceptedMs < WAVE_LOCKOUT_MS)) {
        return 0;
    }
    s->hasAccepted = 1;
    s->lastAcceptedMs = nowMs;
    /* El pin acaba de bajar: los 10 ms en alto se cuentan desde acá. Va antes de needsRelease porque SysTick
     * (prioridad 1) puede interrumpir a EINT1 (prioridad 4) entre las dos escrituras: con needsRelease en 1 y
     * el highMs de la ronda anterior (10), una lectura en alto lo soltaría sin esperar los 10 ms */
    s->highMs = 0;
    s->needsRelease = 1;
    s->current = nextWave(s->current, pythonLoaded);
    return 1;
}

void waveSelectSample(WaveSelect *s, uint8_t pinHigh) {
    if (pinHigh) {
        if (s->highMs < DEBOUNCE_MS) {
            s->highMs++;
        }
        if (s->highMs >= DEBOUNCE_MS) {
            s->needsRelease = 0;
        }
    } else {
        s->highMs = 0;      /* el pin bajó (rebote o pulsador apretado): los 10 ms tienen que ser seguidos */
    }
}
