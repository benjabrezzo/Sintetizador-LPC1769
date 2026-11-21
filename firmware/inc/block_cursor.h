/* Regla de bloques del doble buffer de audio (spec 7.4). Lógica pura: sin dependencias del hardware. */
#ifndef BLOCK_CURSOR_H
#define BLOCK_CURSOR_H

#include <stdint.h>

/* Desde el Bloque 4 la posición de lectura es la fase del oscilador (oscillator.h), que se conserva
 * entre bloques: el cursor solo sigue la mitad. */
typedef struct {
    uint8_t nextHalf;     /* mitad del doble buffer (0 o 1) donde va el próximo bloque */
} BlockCursor;

/* Deja el cursor en el primer bloque: mitad 0 */
void blockCursorInit(BlockCursor *c);

/* Devuelve la mitad del doble buffer a llenar. Después el cursor queda en el bloque siguiente. */
uint8_t blockCursorNext(BlockCursor *c);

#endif /* BLOCK_CURSOR_H */
