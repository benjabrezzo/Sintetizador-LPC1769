#include "voice.h"

void voiceStart(Voice *v, uint32_t step, uint8_t wave) {
    if (envIsIdle(&v->env)) {
        oscInit(&v->osc, step);     /* una voz libre arranca en fase 0 */
    } else {
        v->osc.step = step;         /* una voz que suena sigue desde su fase: solo cambia la nota */
    }
    v->wave = wave;
    envNoteOn(&v->env);             /* Attack desde el nivel actual: 0 en una voz libre */
}

void voiceAllocInit(VoiceAllocator *a) {
    a->nextOrder = 0;
}

/* 1 = la tecla de la voz sigue apretada: la envolvente está en Attack, Decay o Sustain */
static uint8_t isHeld(const Voice *voice) {
    const EnvStage stage = voice->env.stage;
    return stage == ENV_ATTACK || stage == ENV_DECAY || stage == ENV_SUSTAIN;
}

VoiceAssignment voiceAllocPress(VoiceAllocator *a, uint8_t key, const Voice *voices) {
    /* Si la tecla vuelve a sonar mientras su voz sigue activa (apretada o en Release), se reutiliza esa voz. Si no,
     * una voz libre; si no hay, la que está en Release con el menor nivel; si tampoco, la apretada con el menor
     * número de llegada: la más antigua. Si empatan los niveles, la de menor número de voz */
    uint8_t reusedVoice = VOICE_NONE;
    uint8_t freeVoice = VOICE_NONE;
    uint8_t releasingVoice = VOICE_NONE;
    uint8_t oldestVoice = VOICE_NONE;
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        if (voices[v].env.stage != ENV_IDLE && a->key[v] == key) {
            reusedVoice = (uint8_t)v;
        }
        if (voices[v].env.stage == ENV_IDLE) {
            if (freeVoice == VOICE_NONE) {
                freeVoice = (uint8_t)v;
            }
        } else if (voices[v].env.stage == ENV_RELEASE) {
            if (releasingVoice == VOICE_NONE || voices[v].env.level < voices[releasingVoice].env.level) {
                releasingVoice = (uint8_t)v;
            }
        } else if (oldestVoice == VOICE_NONE || a->order[v] < a->order[oldestVoice]) {
            oldestVoice = (uint8_t)v;
        }
    }

    uint8_t voice = oldestVoice;
    if (reusedVoice != VOICE_NONE) {
        voice = reusedVoice;
    } else if (freeVoice != VOICE_NONE) {
        voice = freeVoice;
    } else if (releasingVoice != VOICE_NONE) {
        voice = releasingVoice;
    }
    a->key[voice] = key;
    a->order[voice] = a->nextOrder++;       /* la tecla que reemplaza pasa a ser la más nueva */
    const VoiceAssignment assignment = {voice, key};
    return assignment;
}

uint8_t voiceAllocRelease(const VoiceAllocator *a, uint8_t key, const Voice *voices) {
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        if (isHeld(&voices[v]) && a->key[v] == key) {
            return (uint8_t)v;
        }
    }
    return VOICE_NONE;
}
