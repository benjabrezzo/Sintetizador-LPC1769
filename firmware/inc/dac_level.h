/* Conversión de muestra a valor del DAC (spec 3.1). Lógica pura: sin dependencias del hardware. */
#ifndef DAC_LEVEL_H
#define DAC_LEVEL_H

#include <stdint.h>

/* Paso 4 de la cadena de niveles: (y >> 6) + 512, saturado a 0...1023.
 * Devuelve el valor de 10 bits; el desplazamiento a DACR lo hace DAC_VALUE() en el módulo de hardware. */
uint32_t dacValueFromSample(int32_t y);

/* Igual que dacValueFromSample, y suma 1 a *saturations si la muestra se recortó (spec 3.2, paso 4).
 * Un valor justo en 0 o 1023 no se recortó, así que no cuenta. */
uint32_t dacValueFromSampleCounted(int32_t y, uint32_t *saturations);

/* Lleva un valor de 10 bits del DAC (0...1023) a una muestra Q15 con signo, sin el desplazamiento de 512: (v − 512) · 64,
 * de −32768 a 32704. Es lo que se captura para el análisis espectral (spec 3.6): lo que se oye, recorte incluido. */
int16_t dacValueToQ15(uint32_t dacValue);

#endif /* DAC_LEVEL_H */
