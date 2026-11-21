/* Envolvente ADSR de una voz (spec 3.4). Lógica pura: sin dependencias del hardware. */
#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <stdint.h>

/* Etapas de la envolvente. ENV_IDLE: la voz está libre (no suena ni entra en la mezcla) */
typedef enum {
    ENV_IDLE,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvStage;

/* Nivel máximo (Q15): al que llega el Attack */
#define ENV_LEVEL_MAX  32767

/* Parámetros globales de la envolvente (spec 3.4): los comparten las cuatro voces. Se pasan a cada envStep
 * porque cambian con los potenciómetros (story 10) mientras las notas suenan. */
typedef struct {
    uint16_t attackMs;         /* 2 ... 2000 */
    uint16_t decayMs;          /* 2 ... 2000 */
    int16_t sustainLevel;      /* Q15: 0 ... 32767 = 0 ... 100 % */
    uint16_t releaseMs;        /* 2 ... 2000 */
} EnvParams;

typedef struct {
    EnvStage stage;       /* etapa actual */
    int16_t level;        /* nivel de amplitud Q15, 0 ... 32767: el que usa la mezcla */
    int16_t startLevel;   /* nivel con el que arrancó la etapa actual: la rampa sale de ahí */
    uint32_t progress;    /* fracción recorrida de la etapa actual en Q24 (2^24 = completa) */
} Envelope;

/* Deja la envolvente libre y en nivel 0 */
void envInit(Envelope *e);

/* 1 = la envolvente está libre (ENV_IDLE) */
uint8_t envIsIdle(const Envelope *e);

/* Toca la nota: pasa a Attack desde el nivel actual (0 si estaba libre). La llamada no cuenta como un paso. */
void envNoteOn(Envelope *e);

/* Suelta la tecla: en cualquier etapa pasa a Release desde el nivel actual. Cuando el Release termina, el nivel
 * vale 0 y la envolvente queda libre (ENV_IDLE). La llamada no cuenta como un paso. */
void envNoteOff(Envelope *e);

/* Avanza un paso (1 ms), desde SysTick. Las rampas son lineales y cada etapa dura su tiempo completo desde el
 * nivel en que arranca (spec 3.4). Si un tiempo cambia a mitad de una etapa, lo ya recorrido se conserva y lo que
 * falta se hace a la velocidad nueva. */
void envStep(Envelope *e, const EnvParams *p);

/* Rango de los tiempos de Attack, Decay y Release (spec 3.4) */
#define ENV_TIME_MIN_MS  2
#define ENV_TIME_MAX_MS  2000

/* Tiempo de Attack, Decay o Release (ms) a partir de la lectura de 12 bits del potenciómetro, ya filtrada */
uint16_t envMapTimeMs(uint16_t raw);

/* Nivel de Sustain en Q15 (0 ... 32767 = 0 ... 100 %) a partir de la lectura de 12 bits del potenciómetro, ya
 * filtrada */
int16_t envMapSustainLevel(uint16_t raw);

#endif /* ENVELOPE_H */
