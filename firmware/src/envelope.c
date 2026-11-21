#include "envelope.h"
#include "config.h"                 /* margen de los potenciómetros */

void envInit(Envelope *e) {
    e->stage = ENV_IDLE;
    e->level = 0;
}

uint8_t envIsIdle(const Envelope *e) {
    return e->stage == ENV_IDLE;
}

void envNoteOn(Envelope *e) {
    e->stage = ENV_ATTACK;
    e->startLevel = e->level;       /* la rampa sale del nivel actual: sin salto si la voz ya sonaba */
    e->progress = 0;
}

/* Progreso de una etapa en Q24: 2^24 es la etapa completa */
#define PROGRESS_FULL  (1ul << 24)

/* Lo que avanza el progreso en un paso de 1 ms con una etapa de durationMs: 2^24 / durationMs, redondeado para
 * arriba. Así la etapa termina justo en durationMs pasos: en durationMs pasos suma al menos 2^24, y en
 * durationMs - 1 no llega mientras 2^24 > durationMs · (durationMs - 1), que vale hasta 4096 ms.
 * durationMs >= 2: lo garantiza envMapTimeMs. */
static uint32_t progressPerStep(uint16_t durationMs) {
    return (PROGRESS_FULL + durationMs - 1u) / durationMs;
}

/* La parte recorrida de una distancia de nivel (0 ... 32767) con el progreso dado. El producto usa 64 bits:
 * 32767 · 2^24 no entra en 32. */
static int16_t traveled(int32_t distance, uint32_t progress) {
    return (int16_t)(((uint64_t)distance * progress) >> 24);
}

void envNoteOff(Envelope *e) {
    e->stage = ENV_RELEASE;
    e->startLevel = e->level;       /* el Release baja desde el nivel actual, sea cual sea la etapa */
    e->progress = 0;
}

void envStep(Envelope *e, const EnvParams *p) {
    if (e->stage == ENV_ATTACK) {
        /* El paso sale del attackMs de este momento: si el pote cambia, lo ya recorrido se conserva y lo que falta
         * se hace a la velocidad nueva (spec 3.4) */
        e->progress += progressPerStep(p->attackMs);
        if (e->progress >= PROGRESS_FULL) {
            e->level = ENV_LEVEL_MAX;           /* llega exacto: la rampa no acumula error */
            e->stage = ENV_DECAY;
            e->startLevel = ENV_LEVEL_MAX;      /* el Decay baja desde el pico */
            e->progress = 0;
        } else {
            /* Lineal desde startLevel hasta el máximo */
            e->level = (int16_t)(e->startLevel + traveled(ENV_LEVEL_MAX - e->startLevel, e->progress));
        }
    } else if (e->stage == ENV_DECAY) {
        /* El paso sale del decayMs de este momento, igual que en el Attack */
        e->progress += progressPerStep(p->decayMs);
        if (e->progress >= PROGRESS_FULL) {
            e->level = p->sustainLevel;         /* llega exacto al nivel de Sustain */
            e->stage = ENV_SUSTAIN;
        } else {
            /* Lineal desde el máximo hasta el Sustain. startLevel es el máximo, así que la distancia no es negativa */
            e->level = (int16_t)(e->startLevel - traveled(e->startLevel - p->sustainLevel, e->progress));
        }
    } else if (e->stage == ENV_SUSTAIN) {
        /* El Sustain sigue al pote (spec 3.4): el nivel es el de este momento. El filtro de las lecturas evita que
         * salte de golpe */
        e->level = p->sustainLevel;
    } else if (e->stage == ENV_RELEASE) {
        /* El paso sale del releaseMs de este momento, igual que en el Attack */
        e->progress += progressPerStep(p->releaseMs);
        if (e->progress >= PROGRESS_FULL) {
            e->level = 0;                       /* llega exacto a 0 y la voz queda libre */
            e->stage = ENV_IDLE;
        } else {
            /* Lineal desde el nivel en que se soltó hasta 0: dura releaseMs sin importar desde dónde arranque */
            e->level = (int16_t)(e->startLevel - traveled(e->startLevel, e->progress));
        }
    }
}

/* Mapeo lineal de la lectura de 12 bits del pote a min ... max (spec 3.4). Las primeras y las últimas
 * POT_END_MARGIN cuentas dan el extremo, para que el giro real del pote llegue a los dos. Entre las puntas,
 * raw - 50 recorre 0 ... 3995 y da 0 ... (max - min) por encima de min. Redondea para abajo, así que empalma con
 * los dos extremos. max - min es a lo sumo 32767, y 3995 · 32767 < 2^31: no desborda. */
static uint32_t mapPot(uint16_t raw, uint32_t min, uint32_t max) {
    if (raw <= POT_END_MARGIN) {
        return min;
    }
    if (raw >= ADC_MAX_READING - POT_END_MARGIN) {
        return max;
    }
    const uint32_t span = ADC_MAX_READING - 2u * POT_END_MARGIN;
    const uint32_t offset = (uint32_t)(raw - POT_END_MARGIN);
    return min + offset * (max - min) / span;
}

uint16_t envMapTimeMs(uint16_t raw) {
    return (uint16_t)mapPot(raw, ENV_TIME_MIN_MS, ENV_TIME_MAX_MS);
}

int16_t envMapSustainLevel(uint16_t raw) {
    return (int16_t)mapPot(raw, 0, ENV_LEVEL_MAX);    /* cada cuenta sube unas 8 unidades de Q15 */
}
