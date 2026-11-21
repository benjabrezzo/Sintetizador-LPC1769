/* Mezcla de las voces (spec 3.2). Lógica pura: sin dependencias del hardware. */
#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>
#include "voice.h"

/* Pasos 1 y 2 de la cadena de niveles: acc = Σ ((muestra · nivel de la envolvente) >> 15) en 32 bits, y después
 * acc >> 1 (ganancia 1/2). Cada voz que no está libre (envolvente distinta de ENV_IDLE) lee su tabla en su fase y
 * la avanza, aunque su nivel sea 0 (un Sustain de 0 %); las libres no suman ni avanzan. tables es el arreglo de tablas (WAVE_SIZE muestras Q15 cada una) indexado por el
 * identificador de onda: cada voz lee tables[voice.wave], así dos voces pueden sonar con ondas distintas
 * en el mismo bloque. Quien arranca la voz es responsable de que su identificador esté dentro del arreglo. */
int32_t mixNextSample(Voice *voices, uint32_t numVoices, const int16_t *const *tables);

#endif /* MIXER_H */
