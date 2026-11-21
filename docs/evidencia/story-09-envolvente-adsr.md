# Story 9 · Bloque 7 (parte 1) · Envolvente ADSR por voz

**Estado:** COMPLETA. Respalda los tres criterios de la story 9 y los criterios 1, 2 y 5 del Bloque 7 (ver
"Conclusión"). Los criterios 3 y 4 del Bloque 7 (potenciómetros, y que mover un parámetro no genere saltos) son de
la story 10. Sin osciloscopio: los tiempos de la envolvente en placa no se midieron con un instrumento, y la
ausencia de clicks es de oído.

## Procedimiento
- Spec registrada antes del código (§3.2, §3.4, Bloque 7): rampas lineales y cada etapa dura su tiempo completo
  desde el nivel en que arranca (soltar a mitad del Attack igual tarda R en apagarse); parámetros fijos de prueba
  hasta la story 10 (A 300 ms, D 300 ms, S 50 %, R 1000 ms); una voz está libre cuando su envolvente terminó el
  Release; la nota que toma una voz que no estaba libre sigue desde su fase y su nivel.
- Firmware de la story 9:
  - `envelope` (lógica pura): estados libre, Attack, Decay, Sustain y Release por voz, nivel Q15 (0 a 32767), rampas
    lineales calculadas desde el nivel de partida en 32 bits, exactas al final de cada etapa.
  - `voice` (lógica pura): el asignador mira la envolvente de cada voz y elige la voz de la tecla si sigue activa, una
    libre, la que está en Release con menor nivel o la apretada más antigua. `voiceStart`: una voz libre arranca en
    fase 0 y nivel 0; si no estaba libre, la nota sigue desde su fase y su nivel.
  - `mixer`: una voz que no está libre suma `muestra · nivel`; una libre no suma ni avanza su fase.
  - `audio_out`: `audioOutVoiceStart`, `audioOutVoiceRelease` y `audioOutEnvelopeStep` escriben las voces con la IRQ
    del DMA deshabilitada (`audioIsrLock`). Variables para el depurador: `envStage[4]` (0 libre, 1 Attack, 2 Decay,
    3 Sustain, 4 Release) y `envLevel[4]` (nivel Q15).
  - `sintetizador.c`: SysTick avanza las envolventes cada 1 ms, antes de atender las teclas. Los parámetros fijos
    viven en `config.h` (`ENV_ATTACK_MS`, `ENV_DECAY_MS`, `ENV_SUSTAIN_PERCENT`, `ENV_RELEASE_MS`).
- Hardware: lo de la story 8 (las ocho teclas en P2.0–P2.7, la etapa de audio del Bloque 3 y los auriculares), sin
  potenciómetros.
- Compilado y cargado desde MCUXpresso 25.6. Además, en la PC, los 12 fuentes de `firmware/src` (sin el startup ni
  `crp.c`) compilan con `arm-none-eabi-gcc -Wall` en `-O0` y `-Os`, sin errores ni warnings (sin enlazar).

## Resultado 1: cuatro teclas mantenidas (criterio 1)
- Con las cuatro teclas apretadas a la vez y la CPU suspendida una sola vez (`story-09-cuatro-teclas-mantenidas.jpg`):
  las cuatro voces están en Sustain (`envStage` = 3) con `envLevel` = 16383, que es el 50 % de 32767 (32767 · 50 / 100),
  y `audioUnderruns` = 0.

## Resultado 2: cada voz con su envolvente (criterio 2)
- Cuatro teclas mantenidas y dos soltadas con ~250 ms de diferencia, con una sola suspensión
  (`story-09-release-en-curso.jpg`): `envStage` = {4, 4, 3, 3} y `envLevel` = {4932, 8978, 16383, 16383}. Solo se
  apagan las dos soltadas, cada una en su nivel, y las otras dos siguen en el Sustain.
- Terminado el Release (`story-09-voz-libre.jpg`): `envStage` = {0, 0, 0, 0} y `envLevel` = {0, 0, 0, 0}. Las cuatro
  voces quedaron libres (criterio 5 del Bloque 7).
- Consistencia con las rampas: los niveles son los de un Release lineal desde 16383 en 1000 ms con
  `nivel = 16383 · (1 − t / 1000)`, que da t = 699 ms para 4932 y t = 452 ms para 8978. Son compatibles con el
  tiempo programado, pero no son una medición de los tiempos: no hay una referencia de tiempo independiente.

## Resultado 3: reemplazo (criterio 3)
- Cuatro teclas mantenidas, dos soltadas (primero la de la voz 0 y después la de la voz 1, como en el resultado 2),
  una quinta apretada y una sola suspensión (`story-09-reemplazo.jpg`): `envStage` = {2, 4, 3, 3} y `envLevel` =
  {25231, 1082, 16383, 16383}.
  - Voz 1: sigue en Release y casi apagada (1082 equivale a 934 ms de Release).
  - Voz 0: está en Decay (2) en 25231, que equivale a 138 ms de Decay. Un Release solo llega a Decay si arrancó una
    nota nueva sobre esa voz, así que la quinta tomó la voz 0. La captura llegó ≈ 438 ms después de apretarla
    (300 ms de Attack y 138 de Decay), así que no se ve el Attack en curso.
  - Voces 2 y 3: no se cortaron (Sustain, 16383).
  - `audioUnderruns` = 0.
- Que la voz tomada sea la de menor nivel no sale de esta captura sola: por las rampas, la voz 1 estaba entonces en
  ≈ 8,3 mil, y la voz 0, soltada antes, más abajo. Es compatible, y lo prueba el test 12 de `test_voice`.

## Resultado 4: de oído, con y sin depurador (criterios 1, 2 y 3)
- Con depurador y sin depurador (USB desenchufado y vuelto a enchufar): suena bien, sin clicks al tocar ni al soltar.
  Se probó soltar antes de que terminen el Attack y el Decay, volver a apretar una tecla mientras se apaga, y la
  quinta tecla con las cuatro apretadas. Esta última reemplaza a la más antigua, ahora sin click: en la story 7
  hacía click.
- Que la tecla que vuelve a sonar use la misma voz no se oye ni se capturó; lo respaldan los tests 15 y 16 de
  `test_voice` (la tecla reutiliza su voz mientras se apaga y no la reutiliza si otra tecla se la llevó).

## Resultado 5: tests de la PC
- `make -C firmware/tests test`: los diez programas en verde (`test_block_cursor`, `test_dac_level`,
  `test_debounce`, `test_ejemplo`, `test_envelope`, `test_mixer`, `test_oscillator`, `test_tables`, `test_voice`,
  `test_wave_select`).
- Nuevos:
  - 12 en `test_envelope`: libre al inicio; tiempos de Attack, Decay y Release en 2 y 2000 ms con 1 ms de error;
    Sustain en 0 % y en 100 %; el Sustain se mantiene 70 000 ms; Release soltando a mitad del Attack y del Decay; volver
    a tocar durante el Release; el nivel siempre entre 0 y 32767 en un barrido de extremos; el ciclo completo con los
    valores de `config.h`.
  - 7 en `test_voice`: voz libre antes que una en Release, menor nivel entre las que se apagan, las apretadas no se
    cortan, reutilización de la voz de la tecla, la tecla a la que otra le quitó la voz, y `voiceStart` en voz libre y
    en voz que suena.
  - 1 en `test_mixer`: una voz apretada con Sustain en 0 % suma 0 pero su fase avanza.
- Adaptados sin cambiar lo que verifican: los 8 de `test_voice` al asignador con envolvente (sus 30 `CHECK`
  quedaron idénticos) y los 9 de `test_mixer` a `Voice` sin `active` ni `amplitude` (19 `CHECK` idénticos).
- Dos casos de la lista de tests (la quinta toma la más antigua; soltar devuelve la voz o `VOICE_NONE`) ya los cubrían
  tests existentes.
- Mutaciones en copias fuera del repo: 44 (16 de `envelope`, 25 de `voice`, 3 de `mixer`). Los 9 tests que dieron
  verde de entrada se validaron así. 43 se detectaron y una no (ver "Observaciones sin resolver").

## Observaciones sin resolver
- **Una mutación sin detectar:** reutilizar la voz de la tecla solo si está en Release (en vez de en cualquier etapa
  activa) no rompe ningún test. El caso de una tecla apretada que se vuelve a apretar no tiene test: el antirrebote
  solo genera una pulsación después de una liberación, así que no se da. Se dejó la regla de la spec (apretada o en
  Release).
- **El test del rango del nivel no ve todo:** una mutación que trunca el producto de la rampa a 16 bits no sale del
  rango 0 a 32767, y la detectaron otros tests (tiempo del Release y saltos).
- **Tiempos en placa:** sin osciloscopio no se midieron A, D y R. Los respaldan los tests de la PC (1 ms de error) y la
  consistencia de los niveles capturados. El Attack en curso no se capturó: se pasó por él (la voz del reemplazo llegó
  a Decay) y se oye subir.
- **`audioUnderruns`:** 0 en las cuatro capturas, cada una con una sola suspensión. No se leyó P2.8 (bit 8 de
  `FIOPIN`): si la CPU quedó frenada dentro de la ISR del audio, el contador no incluye el bloque en curso (a lo sumo
  1). Sin depurador solo vale el oído.
- **`audioIsrMaxCycles`:** no se leyó en esta prueba. Referencia de la story 7, con 1 a 4 voces: 23 099 · 29 435 ·
  36 113 · 42 101 de un bloque de 199 936 ciclos.
- **Ausencia de clicks:** es de oído; sin osciloscopio no se midió el escalón.

## Conclusión
- **Criterio 1 (story):** cumple. Sube, baja al Sustain (16383) y se queda; al soltar se apaga y la voz queda libre.
  Sin clicks, tampoco si se suelta antes de que termine el Attack o el Decay.
- **Criterio 2:** cumple. Solo se apagan las soltadas, cada una en su nivel, las otras siguen, y al terminar el
  Release las voces quedan libres en `envStage`. Underruns en 0.
- **Criterio 3:** cumple. Con las cuatro ocupadas, la quinta toma la voz en Release (el reemplazo del resultado 3);
  volver a apretar una tecla mientras se apaga y la quinta con las cuatro apretadas no hacen click. Que sea la de menor
  nivel y que la tecla use su misma voz lo prueban los tests de la PC.
- **Bloque 7, criterio 1** (cada voz mantiene su propia envolvente), **criterio 2** (las cuatro etapas ADSR funcionan:
  capturas en Decay, Sustain y Release, y el Attack de oído y por tests) y **criterio 5** (las voces quedan
  disponibles al finalizar el Release): cumplen. Los criterios 3 y 4 quedan para la story 10.
