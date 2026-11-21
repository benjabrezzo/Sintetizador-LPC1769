/* Lectura de las ocho teclas por GPIO (spec 3.11 y 3.13). Módulo de hardware. */
#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdint.h>

/* Configura P2.0 a P2.7 como entradas GPIO con el pull-up interno. No usa interrupciones de GPIO. */
void buttonInputInit(void);

/* Lee las ocho teclas ahora, sin filtrar. Bit k = 1 si la tecla k + 1 está apretada (el pin P2.k a 0 V): la
 * lógica activa en bajo se invierte acá, para que debounce.h reciba "apretada = 1". Se llama desde SysTick. */
uint8_t buttonInputRead(void);

/* Configura el pulsador de selección de onda: P2.11 como EINT1 con el pull-up interno, flanco descendente,
 * y habilita su interrupción con la prioridad EINT1_IRQ_PRIORITY. Se llama antes de habilitar SysTick; desde
 * ese momento cada flanco descendente entra en EINT1_IRQHandler. */
void buttonInputWaveInit(void);

/* Nivel de P2.11 ahora, sin filtrar: 1 = alto (pulsador suelto), 0 = bajo (apretado). Se llama desde SysTick.
 * Lee el nivel con el GPIO aunque el pin tenga la función EINT1: falta comprobarlo en la placa (story 8). */
uint8_t buttonInputWaveRead(void);

#endif /* BUTTON_INPUT_H */
