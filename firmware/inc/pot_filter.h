/* Filtro de las lecturas de un potenciómetro (Bloque 7: "filtrar o suavizar las lecturas ADC"). Lógica pura: sin
 * dependencias del hardware. */
#ifndef POT_FILTER_H
#define POT_FILTER_H

#include <stdint.h>

typedef struct {
    uint16_t state;       /* lectura filtrada con 4 bits fraccionarios (lectura · 16): 0 ... 65520 */
} PotFilter;

/* Arranca el filtro en la primera lectura del ADC: al encender, el parámetro no barre desde 0 */
void potFilterInit(PotFilter *f, uint16_t firstReading);

/* Procesa una lectura de 12 bits (una por ms, desde SysTick) y devuelve la lectura filtrada */
uint16_t potFilterUpdate(PotFilter *f, uint16_t reading);

#endif /* POT_FILTER_H */
