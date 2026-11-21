/* Estado de una voz del sintetizador (spec 3.2). Lógica pura: sin dependencias del hardware. */
#ifndef VOICE_H
#define VOICE_H

#include <stdint.h>
#include "oscillator.h"
#include "envelope.h"
#include "config.h"      /* NUM_VOICES */

typedef struct {
    Oscillator osc;      /* fase y incremento: la nota de la voz */
    Envelope env;        /* etapa y nivel (spec 3.2 y 3.4): la voz está libre si env.stage == ENV_IDLE, y la mezcla usa env.level */
    uint8_t wave;        /* identificador de la onda con que arrancó la nota (wave_select.h): índice del arreglo de tablas de la mezcla */
} Voice;

/* Arranca una nota en la voz con el incremento de fase y la onda dados, y pasa su envolvente a Attack desde el nivel
 * actual (spec 3.2 y 3.4). Si la voz estaba libre, la nota arranca en fase 0 y nivel 0. Si no estaba libre (la
 * reutilizada, la que se apagaba o la más antigua), sigue desde la fase y el nivel que tenía, para evitar clicks. La
 * onda se fija acá, al arrancar la nota (spec 3.8). Quien la llama cuida de que la ISR del audio no vea la voz a
 * medio escribir. */
void voiceStart(Voice *v, uint32_t step, uint8_t wave);

/* ---- Asignación de teclas a voces (spec 3.2) ----
 * Lógica pura: decide qué voz toca cada tecla y devuelve el resultado. Quien arranca y apaga las voces
 * reales es el módulo de audio. Las teclas se numeran de 0 a 7 (la tecla k + 1 de la spec 3.11) y la
 * nota de la tecla k es el índice k de noteSteps: Do4 = 0 ... Do5 = 7.
 * El asignador no lleva su propio "ocupada": mira la envolvente de cada voz. Libre = ENV_IDLE; en Release =
 * ENV_RELEASE (la tecla ya se soltó y la voz se apaga); apretada = Attack, Decay o Sustain. Las funciones
 * reciben las NUM_VOICES voces y no las modifican: quien las llama arranca la envolvente de la voz elegida
 * (envNoteOn) y pasa a Release la de la tecla soltada (envNoteOff). */

typedef struct {
    uint8_t key[NUM_VOICES];    /* la tecla de cada voz que no está libre */
    uint32_t order[NUM_VOICES]; /* número de llegada de esa tecla: el menor es el más antiguo */
    uint32_t nextOrder;         /* número de llegada de la próxima tecla (con 32 bits no da la vuelta tocando) */
} VoiceAllocator;

/* Qué voz toca una tecla recién apretada y con qué nota */
typedef struct {
    uint8_t voice;    /* 0 ... NUM_VOICES - 1 */
    uint8_t note;     /* índice de noteSteps */
} VoiceAssignment;

/* Arranca el asignador: la antigüedad de las teclas empieza de cero. Qué voces están libres lo dicen sus envolventes */
void voiceAllocInit(VoiceAllocator *a);

/* Asigna una voz a la tecla recién apretada (key: 0 ... 7) y devuelve la voz y la nota. Si la tecla todavía tiene una
 * voz activa (apretada o en Release), la reutiliza. Si no, elige una voz libre; si no hay, la que está en Release con el menor
 * nivel; si tampoco, la apretada más antigua. */
VoiceAssignment voiceAllocPress(VoiceAllocator *a, uint8_t key, const Voice *voices);

/* Valor de voiceAllocRelease cuando la tecla no tenía voz: no hay nada que apagar */
#define VOICE_NONE  0xFF

/* Devuelve la voz apretada de la tecla recién soltada, para que el audio pase su envolvente a Release. Devuelve
 * VOICE_NONE si la tecla no tenía una voz apretada (la perdió, o ya estaba en Release). */
uint8_t voiceAllocRelease(const VoiceAllocator *a, uint8_t key, const Voice *voices);

#endif /* VOICE_H */
