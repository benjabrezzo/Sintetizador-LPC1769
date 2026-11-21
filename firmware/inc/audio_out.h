/* Salida de audio por DAC + GPDMA (spec 3.1). Módulo de hardware. */
#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "lpc_types.h"
#include "config.h"                 /* NUM_VOICES, para el tamaño de audioIsrMaxCycles */
#include "voice.h"                  /* Voice y EnvParams */
#include "equalizer.h"              /* EQ_NUM_BANDS */

/* Configura el DAC (P0.26) con su contador en DAC_COUNTER_VALUE y el canal DAC_DMA_CHANNEL del
 * GPDMA en M2P con dos LLI circulares (doble buffer). Cada 2 ms la ISR del DMA (prioridad 0) llena
 * la mitad que terminó con la mezcla de las voces que no están libres (cada una lee la tabla de su onda,
 * generada por DDS, con el nivel de su envolvente), y deja P2.8 en alto mientras la genera. Arranca en
 * silencio, con todas las voces libres. Devuelve ERROR si el canal no se pudo configurar. Antes hay que llamar a
 * cycleCounterInit: la ISR lo lee desde su primer bloque. */
Status audioOutInit(void);

/* Las cuatro voces, solo lectura: el asignador mira la envolvente de cada una (spec 3.2). Solo audio_out las
 * escribe. Se leen desde SysTick, el mismo contexto que las escribe, así que no hace falta bloquear la ISR del DMA. */
const Voice *audioOutVoices(void);

/* Arranca una nota en una voz: la nota dada (0 = Do4 ... NUM_NOTES - 1 = Do5, spec 3.11) y la onda dada (un
 * identificador de wave_select.h: la onda se fija al arrancar la nota, spec 3.8). Su envolvente pasa a Attack desde el
 * nivel actual. Si la voz estaba libre, la nota arranca en fase 0 y nivel 0; si no (la reutilizada, la que se apagaba
 * o la más antigua), sigue desde la fase y el nivel que tenía, sin click (spec 3.2). Se llama desde SysTick, después
 * de audioOutInit: deshabilita la IRQ del DMA mientras escribe la voz, así la ISR la ve completa. Un valor fuera de
 * rango se ignora. */
void audioOutVoiceStart(uint8_t voice, uint8_t note, uint8_t wave);

/* Suelta la tecla de una voz: su envolvente pasa a Release desde el nivel actual (spec 3.4). La voz sigue sonando
 * mientras se apaga y queda libre cuando termina el Release. Se llama desde SysTick, con el mismo bloqueo de la IRQ
 * del DMA. Un valor fuera de rango se ignora. */
void audioOutVoiceRelease(uint8_t voice);

/* Avanza 1 ms las envolventes de las cuatro voces con los parámetros dados (spec 3.4). Se llama desde SysTick, una
 * vez por ms, con el mismo bloqueo de la IRQ del DMA. */
void audioOutEnvelopeStep(const EnvParams *params);

/* Buffer donde se recibe la onda de Python (WAVE_SIZE muestras int16_t). No es la tabla que suena: recibir ahí no
 * afecta al audio, así que una onda que se descarta lo deja con basura sin consecuencias (spec 3.8). Cambia después de
 * cada audioOutPythonWaveActivate: se pide de nuevo antes de empezar a recibir una onda. Solo lo toca el loop principal. */
int16_t *audioOutPythonWaveRxBuffer(void);

/* Activa la onda recibida: la quinta posición pasa a leer el buffer de recepción y el buffer que sonaba pasa a ser el de
 * recepción. El cambio es una escritura atómica del puntero, entre dos bloques de la ISR del DMA. Las voces
 * que usan la quinta onda pasan a la tabla nueva sin cortarse (se acepta un click, spec 3.8); las demás no cambian. Se
 * llama desde el loop principal, solo con una onda ya validada (checksum correcto). */
void audioOutPythonWaveActivate(void);

/* Encola los coeficientes del ecualizador (spec 3.5): 15 int32 en Q2.30, por banda b0, b1, b2, a1, a2 (docs/protocolo.md),
 * para aplicarlos al empezar el próximo bloque de audio, sin tocar el estado de los filtros. Se llama desde el loop
 * principal, solo con una trama validada (checksum correcto). Si llegan dos antes de que empiece un bloque, gana la
 * última. El ecualizador arranca plano (las tres bandas en 0 dB) y la ISR del DMA lo aplica a cada muestra, entre la
 * mezcla y la saturación. */
void audioOutEqApply(const int32_t coefs[EQ_NUM_BANDS * 5]);

/* Captura de lo que se oye para el análisis espectral (spec 3.6): FFT_SIZE muestras Q15 seguidas, las mismas que van al DAC
 * (ya saturadas; dacValueToQ15). La ISR del DMA las copia bloque a bloque y, cuando se llenan, deja de escribir hasta que
 * el loop principal las libera. Devuelve el buffer (solo lectura) si ya está lleno y 0 si todavía se está llenando; no
 * espera. Solo la llama el loop principal. */
const int16_t *audioOutCapture(void);

/* Termina de usar la captura y reabre su llenado: la ISR empieza a copiar desde el próximo bloque. Se llama una vez por
 * cuadro, después de audioOutCapture devolvió el buffer y de haber leído sus muestras; el puntero deja de valer. */
void audioOutCaptureRelease(void);

/* Etapa de la envolvente de cada voz: 0 libre, 1 Attack, 2 Decay, 3 Sustain, 4 Release (EnvStage de envelope.h). Es
 * evidencia para la story 9: se lee en la vista Expressions con la CPU suspendida, igual que audioUnderruns. Solo la
 * escribe SysTick, al final de audioOutVoiceStart, audioOutVoiceRelease y audioOutEnvelopeStep. */
extern volatile uint8_t envStage[NUM_VOICES];

/* Nivel Q15 (0 ... 32767) de la envolvente de cada voz; se lee y se escribe igual que envStage. */
extern volatile int16_t envLevel[NUM_VOICES];

/* Bloques que la ISR terminó tarde (spec 7.1). Debe quedar en 0; se lee en la vista Expressions con la
 * CPU suspendida. */
extern volatile uint32_t audioUnderruns;

/* Muestras que la mezcla recortó al rango del DAC (spec 3.2, paso 4), acumuladas desde el arranque. Con
 * 3 o 4 voces al máximo tiene que crecer; se lee igual que audioUnderruns. */
extern volatile uint32_t audioSaturations;

/* Máximo de ciclos de CPU que tardó la ISR en generar un bloque, por cantidad de voces que no estaban libres al
 * empezar el bloque: el índice es esa cantidad (0...NUM_VOICES). Mide lo mismo que P2.8 (desde que sube hasta que
 * baja) con cycleCounterNow (Timer0, resolución de 4 ciclos), así que vale con la duración que tenga la ISR; un bloque
 * dura 199 936 ciclos. Se lee en la vista Expressions con la CPU suspendida. */
extern volatile uint32_t audioIsrMaxCycles[NUM_VOICES + 1];

#endif /* AUDIO_OUT_H */
