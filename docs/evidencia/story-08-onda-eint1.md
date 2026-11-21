# Story 8 · Bloque 6 (parte 2) · Rotación de la forma de onda con EINT1

**Estado:** COMPLETA. Respalda los cuatro criterios de la story 8 y el criterio 6 del Bloque 6, con lo que el
Bloque 6 queda completo (ver "Conclusión"). Sin osciloscopio: el rebote real del pulsador no se observó, y la
ventana de 150 ms se respalda con los tests de la PC.

## Procedimiento
- Spec registrada antes del código (§3.8, §3.11): la placa arranca en senoide; cuadrada, sierra y triangular se
  generan limitadas en banda (suma de senoides hasta el armónico 30, pico en 32767); una pulsación cuenta si
  pasaron 150 ms desde la última aceptada y el pin estuvo 10 ms seguidos en alto desde entonces, así que una
  pulsación mantenida cuenta una sola vez.
- Firmware de la story 8:
  - `gen_tables.py` genera `squareTable`, `sawTable` y `triangleTable`.
  - `wave_select` (lógica pura) decide qué pulsaciones cuentan y cuál es la onda siguiente. La hora entra por
    parámetro.
  - `button_input` configura P2.11 como EINT1 (pull-up, flanco descendente, prioridad 4) y lee su nivel por GPIO.
  - `sintetizador.c`: `msTicks` (lo incrementa SysTick), `EINT1_IRQHandler` (limpia el flag al entrar), y SysTick
    lee el pin para rearmar el pulsador. Cada nota toma `activeWave` al arrancar.
  - `audioOutVoiceStart(voice, note, wave)` guarda el identificador de onda en la voz. La mezcla lee
    `tables[voice.wave]` y el arreglo `waveTables[]` vive en `audio_out.c`. La posición de Python apunta a la
    senoide y la rotación la saltea (`PYTHON_WAVE_LOADED` = 0) hasta la story 11.
  - Variables para el depurador: `waveButtonCount` (pulsaciones aceptadas) y `activeWave` (0 cuadrada, 1 sierra,
    2 triangular, 3 senoide, 4 Python).
- Hardware: lo de la story 7, más un pulsador entre P2.11 y GND, sin resistencia (pull-up interno).
- Compilado en MCUXpresso 25.6, sin warnings nuevos.

## Resultado 1: orden de la rotación (criterio 1)
- Lectura del pin: `LPC_GPIO2->FIOPIN` no se evalúa en Expressions (es un macro); se usó
  `*(volatile unsigned int *)0x2009C054`. Suelto vale `0x3EFF` y apretado `0x36FF`: cambia solo el bit 11
  (`story-08-GPIO2-FIOPIN-EINT1SUELTO.jpg`, `story-08-GPIO2-FIOPIN-EINT1APRETADO.jpg`).
- `activeWave` después de cada pulsación, una captura por paso:

| Paso | `waveButtonCount` | `activeWave` | Onda | Captura |
|---|---|---|---|---|
| Arranque | 0 | 3 | senoide | `story-08-activeWave-alInicio.jpg` |
| 1.ª | 1 | 0 | cuadrada | `story-08-activeWave-despues-1era-pulsacion.jpg` |
| 2.ª | 2 | 1 | sierra | `story-08-activeWave-2daPulsacion.png` |
| 3.ª | 3 | 2 | triangular | `story-08-activeWave-3eraPulsacion.jpg` |
| 4.ª | 4 | 3 | senoide | `story-08-activeWave-4taPulsacion.jpg` |
| 5.ª | 5 | 0 | cuadrada (vuelve a empezar) | `story-08-activeWave-5taPulsacion.jpg` |

- Tablas en la placa (`story-08-20-pulsaciones.jpg`): `waveTables[0]` = `0x4180` `<squareTable>`,
  `[1]` = `0x4380` `<sawTable>`, `[2]` = `0x4580` `<triangleTable>`, `[3]` y `[4]` = `0x3F80` `<sineTable>`.
- De oído: cuadrada y sierra suenan brillantes; senoide y triangular, parecidas entre sí. Coincide con el cálculo
  sobre las tablas (respecto de la fundamental): cuadrada con h3 en −9,5 dB y sin pares, sierra con h2 en −6 dB, triangular
  con h3 en −19 dB. No es una medición en placa.

## Resultado 2: nota mantenida (criterio 2)
- De oído, con depurador: la nota que se mantenía conservó su onda hasta soltarla, y la primera nota que se tocó
  después sonó con la onda nueva (cuadrada).
- Una sola suspensión al final (`story-08-nota-mantenida.jpg`): `waveButtonCount` 1, `activeWave` 0,
  `audioUnderruns` 0. Reserva: el bit 8 de `FIOPIN` (P2.8) vale 1, así que la CPU quedó frenada dentro de la ISR
  del audio y el contador no incluye el bloque en curso (a lo sumo 1).

## Resultado 3: 20 pulsaciones y ventana de 150 ms (criterio 3)
- Sesión nueva y 20 pulsaciones contadas a mano: `waveButtonCount` 20, `activeWave` 3 (cinco vueltas de cuatro
  posiciones) y `audioUnderruns` 0 (`story-08-20-pulsaciones.jpg`).
- Doble toque rápido: el contador suma 1. Es una prueba a mano, sin captura, y no mide el límite exacto. Eso lo
  prueban los tests de la PC (a 149 ms se ignora, a 150 ms se acepta).
- Tocando teclas con una sola suspensión: `audioUnderruns` 0 (`story-08-audioUnderruns-tocando-teclas.jpg`).

## Resultado 4: pulsación mantenida (criterio 4)
- Pulsador mantenido al menos un segundo (la captura se guardó como «5seg») y soltado, con la CPU corriendo:
  `waveButtonCount` 1, `activeWave` 0, `audioUnderruns` 0 y el pin ya suelto (bit 11 de `FIOPIN` en 1)
  (`story-08-eint1-mantenido-5seg.jpg`). Cuenta una vez y el rebote al soltar no suma.

## Resultado 5: sin depurador
- Con el USB desenchufado y vuelto a enchufar, se probaron la rotación completa, la nota mantenida y la pulsación
  mantenida: se escuchó todo bien. Los contadores no se leen sin depurador.

## Resultado 6: tests de la PC
- `make -C firmware/tests test`: los nueve programas en verde (`test_block_cursor`, `test_dac_level`,
  `test_debounce`, `test_ejemplo`, `test_mixer`, `test_oscillator`, `test_tables`, `test_voice`,
  `test_wave_select`).
- Nuevos: 8 en `test_tables` (pico en 32767, simetrías, amplitud de cada armónico, nada por encima del 30),
  14 en `test_wave_select` (rotación, salto de la quinta, ventana de 150 ms, rearmado al soltar, rebote y 20
  pulsaciones simuladas) y 1 en `test_mixer` (dos voces con ondas distintas en el mismo bloque). Los 8 usos de
  `mixNextSample` de `test_mixer` se adaptaron a la firma nueva sin cambiar lo que verifican.
- Mutaciones en copias fuera del repo: se rompieron el generador de tablas (9 mutaciones), `wave_select` (11) y el
  mezclador (3), y cada rotura la detectó al menos un test. Una (tabla indexada por número de voz) se detectó
  porque el programa de test se cae, no por el test nuevo. Los tests que dieron verde de entrada se validaron así.

## Observaciones sin resolver
- **`audioUnderruns` y las suspensiones:** en la serie de una captura por pulsación (varias suspensiones y
  reanudaciones) subió 0, 0, 1, 1, 1, 2. Las cuatro capturas con una sola suspensión (20 pulsaciones, teclas,
  nota mantenida y pulsación mantenida) dan 0. Se atribuye a las suspensiones del depurador, como advierte
  `CLAUDE.md`, pero no está confirmado: falta el control de suspender y reanudar varias veces sin apretar nada.
- **Suspensión dentro de la ISR:** en `story-08-activeWave-alInicio.jpg`, `story-08-activeWave-4taPulsacion.jpg`
  y `story-08-nota-mantenida.jpg` el bit 8 de `FIOPIN` vale 1.
- **Rebote real:** sin osciloscopio no se midió. Se respalda con la simulación a 1 ms de 100 flancos con rebote al
  apretar y al soltar (20 aceptados), con las 20 pulsaciones de 20 en placa y con la pulsación mantenida.
- **Volumen entre ondas:** fuera de alcance de la story. Por cálculo sobre las tablas, el nivel (RMS sobre el pico)
  es 0,85 la cuadrada, 0,71 la senoide, 0,59 la triangular y 0,50 la sierra.
- **Compilación ARM:** en la PC solo se revisó la sintaxis de los archivos de firmware contra los headers reales de
  los drivers; la compilación fue la del IDE.

## Conclusión
- **Criterio 1 (story):** cumple (orden 3 → 0 → 1 → 2 → 3 → 0 en las capturas, tablas correctas, timbres coherentes
  de oído).
- **Criterio 2:** cumple (la nota mantenida conserva su onda, la siguiente usa la nueva, underruns 0).
- **Criterio 3:** cumple (20 pulsaciones dan 20, el doble toque suma 1; el límite de 150 ms, por tests).
- **Criterio 4:** cumple (pulsación mantenida cuenta una vez; el rebote al soltar no suma).
- **Bloque 6, criterio 6:** cumple. Con los criterios 1 a 5 de la story 7, el Bloque 6 queda completo.
