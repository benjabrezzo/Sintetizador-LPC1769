# Story 5 · Bloque 4 · Notas afinadas por DDS

**Estado:** COMPLETA. Respalda los dos criterios de la story 5 y los cinco del Bloque 4 (ver "Conclusión").
La frecuencia de las notas y la Fs fija se aprueban con evidencia indirecta, porque no hay instrumento para
medirlas.

## Procedimiento
- Firmware de la story 5: una voz por DDS sobre la senoide, con la fase en Q16.16, vuelta en 2^24, índice en
  los bits 23:16 y muestras Q15. Fs = 25 MHz / 781, sin cambios desde el Bloque 1.
- Secuencia de prueba desde SysTick: Do4→Do5, 5 s por nota (`SEQUENCE_NOTE_MS`), en bucle.
- Compilado en MCUXpresso 25.6, sin warnings.
- Etapa de audio del Bloque 3 con el cargador de 5 V y masa común, y auriculares.
- Sin osciloscopio, multímetro ni afinador. Como referencia de oído, una guitarra (no se registró cómo estaba
  afinada).

## Resultado 1: escucha (criterio 1)
- Con depurador: unas 5 vueltas de la escala (≈ 40 s cada una). Cada nota se mantiene estable durante su
  tramo. Las transiciones son suaves, sin clicks, cortes ni silencios.
- Sin depurador (USB desenchufado y vuelto a enchufar): otras 5 vueltas, con el mismo resultado.
- Comparación de oído con la guitarra: las notas suenan muy parecidas. Es un apoyo cualitativo, no una
  medición.

## Resultado 2: cálculo indirecto de la frecuencia
f = ΔP · Fs / 2^24, con los ΔP de `firmware/src/tables.c` (generados por `gen_tables.py`).
Fs nominal = 25 MHz / 781 = 32 010,24 Hz. Fs medida en la story 2: de 31 936 a 32 012 Hz (−0,232 % a
+0,005 %).

| Nota | §3.11 (Hz) | ΔP | f con Fs nominal (Hz) | Error (cents) | f con la Fs medida (Hz) |
|---|---|---|---|---|---|
| Do4 | 261,63 | 137 126 | 261,6308 | +0,005 | 261,02 a 261,65 |
| Re4 | 293,66 | 153 913 | 293,6597 | −0,002 | 292,98 a 293,68 |
| Mi4 | 329,63 | 172 766 | 329,6305 | +0,003 | 328,87 a 329,65 |
| Fa4 | 349,23 | 183 039 | 349,2309 | +0,005 | 348,42 a 349,25 |
| Sol4 | 392,00 | 205 455 | 391,9998 | −0,001 | 391,09 a 392,02 |
| La4 | 440,00 | 230 613 | 440,0002 | +0,001 | 438,98 a 440,02 |
| Si4 | 493,88 | 258 852 | 493,8790 | −0,003 | 492,73 a 493,91 |
| Do5 | 523,25 | 274 246 | 523,2502 | +0,001 | 522,04 a 523,28 |

- El mayor error de los incrementos es +0,005 cents (Do4). El límite es 1 cent.
- Con la Fs medida, el peor caso es −0,232 % (−4,0 cents) en todas las notas, porque f es proporcional a Fs.
  El límite es ±1 % (≈ 17 cents).

## Resultado 3: tests (criterio 2)
```
$ make -C firmware/tests test
== build/test_block_cursor
OK
== build/test_dac_level
OK
== build/test_ejemplo
OK
== build/test_oscillator
OK
(salida 0)
```
- `test_oscillator`: afinación (8 notas, menos de 1 cent) y vuelta del acumulador (Do5, 4 bloques de 64,
  saltos de índice de 4 o 5 al pasar por 2^24 y entre bloques).
- `test_block_cursor`: alternancia de mitades y flujo del DMA sin saltos, ahora llenado con el oscilador.
  Se borró `testBloquesConsecutivosContinuan`, porque el cursor ya no da índice. La continuidad entre bloques
  la cubren el test de vuelta y el del flujo.
- Test roto a propósito: con `INDEX_SHIFT` = 15 en `oscillator.c`, `make test` salió con 2. Falló
  `test_block_cursor` con 637 fallas, y el bucle se corta en el primer test que falla. `test_oscillator`,
  corrido directamente, dio 253 fallas en `jump == 4 || jump == 5` y salida 1. Restaurado a 16: salida 0.

## Observaciones sin resolver
- **Ruido después de Terminate.** Al terminar la sesión en MCUXpresso, el audio pasa a sonar con ruido. Al
  desenchufar y volver a enchufar el USB queda bien. Hipótesis sin probar: la CPU queda frenada y el GPDMA
  sigue recorriendo las dos LLI, así que repite 4 ms de la nota con un salto de fase cada 4 ms. No afecta el
  funcionamiento sin depurador.
- **Duración de la ISR** sin medir: cada muestra ahora pasa por el DDS. Se mide en los Bloques 5 y 10
  (§3.12).

## Decisiones (09-11-2026)
- La frecuencia de las notas y la Fs fija se aprueban con evidencia indirecta (spec, Bloque 4): incrementos
  verificados en la PC y Fs medida en el Bloque 1. Si una medición posterior lo contradice, se corrige antes
  de seguir.
- Agregado durante la story: SysTick quedaba con prioridad 0, igual que el DMA. Se fijó en 1 desde
  `config.h`, junto con la del DMA (§3.12, principio 9).

## Conclusión
- **Criterio 1:** cumple (10 vueltas, con y sin depurador).
- **Criterio 2:** cumple (tests en verde y salida distinta de 0 al romper uno).
- **Bloque 4:**
  - Una voz reproduce una tabla mediante DDS: cumple (Resultado 1).
  - Se pueden seleccionar distintas frecuencias: cumple (8 notas con `audioOutSetNote`, Resultado 1).
  - La Fs permanece fija: cumple, con evidencia indirecta (el contador del DAC no cambia y la nota se elige
    con ΔP).
  - La frecuencia coincide dentro de la tolerancia: cumple, con evidencia indirecta (−4,0 cents en el peor
    caso, Resultado 2).
  - No aparecen errores numéricos ni interrupciones: cumple (test de vuelta y escucha sin cortes).
