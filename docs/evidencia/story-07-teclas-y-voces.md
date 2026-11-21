# Story 7 · Bloque 6 (parte 1) · Teclas y asignación de voces

**Estado:** COMPLETA. Respalda los tres criterios de la story 7 y los criterios 1 a 5 del Bloque 6 (ver "Conclusión").
El criterio 6 (EINT1) es de la story 8. Se aprueba con evidencia indirecta para el rebote, porque no hay
osciloscopio: el rebote real del contacto no se observó.

## Procedimiento
- Firmware de la story 7: `button_input` lee P2.0–P2.7 (pull-up interno, activas en bajo), `debounce` pasa las
  lecturas de SysTick (cada 1 ms) a eventos tras 10 ms estables, `voice` asigna voz (libre, o reemplazo de la
  más antigua) y `applyKeyEvents` (en `sintetizador.c`) aplica los eventos. `audioOutVoiceStart` deshabilita
  `DMA_IRQn` mientras escribe la voz. `keyPressCount[8]` cuenta las pulsaciones que detecta el antirrebote.
- Se retira la secuencia de acordes del Bloque 5; el firmware arranca en silencio.
- Hardware: ocho pulsadores entre P2.0–P2.7 y GND, sin resistencias; continuidad verificada (solo al apretar, sin
  continuidad entre vecinos). Etapa de audio del Bloque 3 y auriculares.
- Compilado en MCUXpresso 25.6, configuración Debug, sin errores ni warnings.

## Resultado 1: una tecla por vez (criterio 1)
- Las ocho teclas suenan con su nota (Do4 a Do5, teclas 1 a 8) mientras se mantienen y se callan al soltar.
- 20 pulsaciones por tecla: `keyPressCount` = {20, 20, 20, 20, 20, 20, 20, 20}
  (`story-07-20-pulsaciones-por-tecla.jpg`). Cada pulsación cuenta una vez, aunque el contacto rebote.

## Resultado 2: varias teclas y reemplazo (criterios 2 y 3)
- De oído, con y sin depurador (USB desenchufado y vuelto a enchufar): 2, 3 y 4 teclas suenan juntas sin cortes,
  al soltar una se calla solo esa. Con la quinta tecla se calla la que empezó primero, las otras tres siguen y al
  soltar la que perdió su voz la nota nueva no se corta. Sin depurador se repitió la prueba de las 20 pulsaciones
  por tecla y la del límite de cuatro, de oído (los contadores no se leen sin depurador): sin cortes.
- Con depurador, sesión con 4 teclas y una quinta (`story-07-cuatro-voces-y-reemplazo.jpg`):

| Variable | Valor |
|---|---|
| `audioUnderruns` | 0 |
| `audioSaturations` | 40 304 |
| `audioIsrMaxCycles[1]` a `[4]` | 23 099 · 29 435 · 36 113 · 42 101 |
| `keyPressCount[0]` a `[4]` · `[5]` | 1 · 0 (las teclas 7 y 8 no se ven en la captura) |

- `story-07-audioUnderruns.jpg` es de otra sesión, con una sola voz: `audioUnderruns` = 0 y solo
  `audioIsrMaxCycles[1]` = 23 103.
- Sin ADSR se oyen clicks al apretar y soltar, como se esperaba. LED de P0.22 con período normal.

## Resultado 3: tests de la PC
- `make -C firmware/tests test`: los siete programas en verde (`test_block_cursor`, `test_dac_level`,
  `test_debounce`, `test_ejemplo`, `test_mixer`, `test_oscillator`, `test_voice`).
- Nuevos: 8 en `test_debounce` y 8 en `test_voice`, escritos primero y vistos en rojo.
- Mutaciones en copias fuera del repo: una mostró que el reinicio del contador de rebote no tenía cobertura, y
  se agregó el test de rebote alternado.

## Observaciones sin resolver
- **Rebote real:** sin osciloscopio no se midió su duración. El antirrebote de 10 ms se respalda con los tests y
  con las 20 pulsaciones contadas.
- **Click al reemplazar:** la voz reemplazada reinicia la fase en 0 (click aceptado hasta el ADSR del Bloque 7).
- **Compilación ARM:** en la PC solo se revisó la sintaxis; la compilación fue la del IDE.

## Conclusión
- **Criterio 1 (story):** cumple (ocho notas, una por pulsación, 20 de 20).
- **Criterio 2:** cumple (hasta cuatro teclas, sin cortes, underruns 0 con depurador).
- **Criterio 3:** cumple (reemplazo de la más antigua, las otras tres siguen, la nota nueva no se corta).
- **Bloque 6, criterios 1 a 5:** cumplen. El 5 (rebote) con evidencia indirecta.
