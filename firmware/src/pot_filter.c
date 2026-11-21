#include "pot_filter.h"
#include "config.h"                 /* POT_FILTER_SHIFT */

/* Bits fraccionarios del estado. Sin ellos, con α = 1/16 el filtro se detendría hasta 15 cuentas antes del destino:
 * cuando falta menos de 16, 1/16 de lo que falta es 0 */
#define FRACTION_BITS  4

void potFilterInit(PotFilter *f, uint16_t firstReading) {
    f->state = (uint16_t)(firstReading << FRACTION_BITS);
}

uint16_t potFilterUpdate(PotFilter *f, uint16_t reading) {
    /* EMA de primer orden: en cada paso el estado avanza 1/16 de lo que le falta. La división trunca hacia 0, así
     * que nunca se pasa del destino y se detiene a menos de 16 (una cuenta) de él. */
    const int32_t remaining = ((int32_t)reading << FRACTION_BITS) - f->state;
    f->state = (uint16_t)(f->state + remaining / (1 << POT_FILTER_SHIFT));
    /* Redondea a la cuenta más cercana: queda a lo sumo a 1 cuenta del destino y tampoco se pasa (65520 + 8 da
     * 4095, no 4096) */
    return (uint16_t)((f->state + (1u << (FRACTION_BITS - 1))) >> FRACTION_BITS);
}
