# Story 15 · Bloque 11 · Línea base de la ISR de audio con ecualizador y sin captura

**Estado:** MEDIDA (2026-11-14) en Debug sin optimizar (-O0) y con -O1. Es la línea base que pedía el plan de la story 15, antes de agregar la captura de la FFT.
**Resultado:** con 4 voces y el ecualizador, la ISR dura 126 676 ciclos (EQ plano) y 127 912 (EQ exigente) en **-O0**, el 63,4 % y el 64,0 % de un bloque: supera la mitad (99 968) que pide el criterio 3. Con **-O1** baja a **37 556** y **39 016** ciclos (18,8 % y 19,5 %), y el criterio se cumple con margen, todavía sin captura.

## Procedimiento
- Firmware de `main` al 2026-11-14 en configuración **Debug** con el nivel por defecto de MCUXpresso (sin optimizar, -O0; no lo verificamos en el IDE, pero el `.cproject` no tenía ningún valor de optimización para Debug). El módulo `spectrum` está en el proyecto pero todavía nada lo usa: no hay captura ni análisis.
- Hardware: la placa con el depurador USB, las teclas Do4, Mi4, Sol4 y Do5, los potes con Sustain por encima de 0 % y, en la sesión B, el adaptador USB-TTL con la app. Sin etapa de audio.
- Cada sesión arranca desde cero (Terminate y relanzar), así los máximos parten de 0. Las cuatro teclas apretadas a la vez con cada onda, una sola suspensión al final y lectura de `audioUnderruns`, `audioSaturations` y `audioIsrMaxCycles[0]` a `[4]` en la vista Expressions.
  - **Sesión A:** ecualizador plano (lo que arranca).
  - **Sesión B:** graves −12 dB, medios +6 dB y agudos +6 dB, con el ecualizador cargado desde la app.

## Primer intento: el contador desbordaba
Con el contador de SysTick (que da la vuelta cada 1 ms, es decir, cada 100 000 ciclos) las lecturas no eran creíbles (`story-15` primeras capturas: `ecualizadorplano.jpg` y `ecualizadoexigente.jpg`):

| | [0] | [1] | [2] | [3] | [4] |
|---|---|---|---|---|---|
| Plano | 95 838 | 3 710 | 10 942 | 18 046 | 25 539 |
| Exigente | 97 376 | 5 159 | 12 519 | 19 674 | 26 857 |

El caso sin voces daba cerca de 100 000 y los demás, valores chicos. Sumando 100 000 a [1] a [4] quedaban lineales con el costo por voz. Era el desborde del contador: el comentario de `audio_out.h` ya advertía que valía solo con una ISR de menos de 1 ms. Se reemplazó por un contador sobre el Timer0 a 25 MHz (módulo `cycle_counter`, resolución de 4 ciclos), que no da la vuelta durante una ISR.

## Resultado: con el Timer0 (`ecualizadorPlanoTimer.jpg` y `ecualizadorExigenteTimer.jpg`)
Ciclos de CPU de la ISR por cantidad de voces no libres (un bloque dura 199 936; la mitad, 99 968):

| Voces | Plano | Exigente |
|---|---|---|
| 0 | 96 064 | 96 832 |
| 1 | 104 064 | 105 580 |
| 2 | 111 552 | 113 112 |
| 3 | 118 784 | 120 284 |
| 4 | **126 676** (63,4 %) | **127 912** (64,0 %) |

| Variable | Plano | Exigente |
|---|---|---|
| `audioUnderruns` | 0 | 0 |
| `audioSaturations` | 48 642 | 49 106 |

- Coincide con el primer intento módulo 100 000 (por ejemplo, [2] plano: 111 552 contra 10 942 + 100 000, con unos 600 ciclos de diferencia entre sesiones), así que el diagnóstico del desborde se confirma.
- Costo de cada voz: de 7 100 a 8 700 ciclos por bloque (plano: 8 000, 7 488, 7 232 y 7 892; exigente: 8 748, 7 532, 7 172 y 7 628). En la story 6 eran de 5 900 a 6 300, antes de agregar la multiplicación por la envolvente.
- El ecualizador exigente suma poco sobre el plano: 1 236 ciclos con 4 voces (+1,0 %). La diferencia entre plano y exigente es chica frente al costo base.
- Costo base: con 0 voces la ISR ya dura 96 064 ciclos (48,0 % del bloque). La story 6 daba unos 17 000 para ese mismo caso (extrapolando su [1] = 23 182 menos una voz), sin ecualizador. Eso deja al ecualizador en unos **79 000 ciclos por bloque** (≈ 1 230 por muestra, ≈ 410 por Biquad). Es una estimación: el firmware cambió también en otros puntos desde la story 6.
- `audioUnderruns` en 0 con una ISR de hasta 64 % del bloque: coherente, porque sigue por debajo del bloque.

## Qué significa para la story 15
- **Criterio 3 (ISR por debajo de la mitad del bloque con 4 voces y ecualizador, en Debug):** no se cumple. Ya está en 126 676 y 127 912 ciclos sin captura (≈ 27 % y 28 % sobre el límite de 99 968), y la captura todavía suma.
- **Presupuesto del loop principal:** con 4 voces la ISR deja unos 73 000 ciclos por bloque (36,6 %) a SysTick, el parser y el análisis.
- **Story 14, criterio 3:** había quedado marcado sin medir (`story-14-ecualizador.md`). Con estos números, el plazo de la ISR con ecualizador en Debug no se cumple.
- **Spec §3.12:** la estimación de unos 20 mil ciclos (≈ 10 % de la CPU) para la ISR completa no se sostiene en Debug.

## Con -O1 (Debug): `story-15-prueba-nivel-optimización.jpg`, `story-15-ecualizadorPlano-conTimer-y-optimizacion.jpg` y `story-15-ecualizadorExigente-conTimer-y-optimizacion.jpg`
Mismo procedimiento y mismo hardware, con **Optimization Level = Optimize (-O1)** en la configuración Debug (Properties › C/C++ Build › Settings › MCU C Compiler › Optimization), Clean y Build, y sesiones nuevas desde cero. Otras opciones de la captura: `-fno-common`, sin LTO. El cambio quedó en `.cproject` (`gnu.c.optimization.level.optimize`, solo en Debug).

| Voces | Plano | Exigente |
|---|---|---|
| 0 | 27 896 | 29 432 |
| 1 | 30 840 | 32 036 |
| 2 | 33 528 | 34 356 |
| 3 | 35 512 | 37 184 |
| 4 | **37 556** (18,8 %) | **39 016** (19,5 %) |

| Variable | Plano | Exigente |
|---|---|---|
| `audioUnderruns` | 0 | 0 |
| `audioSaturations` | 56 889 | 55 322 |

- Con 4 voces la ISR es 3,4 veces más rápida (plano: 126 676 → 37 556; exigente: 127 912 → 39 016) y queda a unos 61 000 ciclos del límite de 99 968.
- Costo de cada voz: de 1 800 a 2 900 ciclos por bloque (en -O0 eran de 7 100 a 8 700). El caso sin voces (ecualizador y conversión de 64 muestras) pasa de 96 064 a 27 896 en plano y de 96 832 a 29 432 en exigente.
- El ecualizador exigente sigue sumando poco sobre el plano: 1 460 ciclos con 4 voces (+3,9 %).
- Con 4 voces, la ISR deja libre al loop principal unos 162 000 ciclos por bloque (81 %), contra los 73 000 de -O0.
- No registramos la escucha con la etapa de audio en esta sesión.

## Conclusión
- **Criterio 3 de la story 15 (ISR por debajo de la mitad del bloque con 4 voces y ecualizador):** la línea base cumple en Debug con -O1 (39 016 de 99 968 en el peor caso), y la captura de la FFT tiene un margen de unos 60 000 ciclos. No cumple en Debug sin optimizar (-O0).
- **Pendiente:** confirmar si el Debug queda en -O1 en el repositorio (el cambio está en `.cproject`). Los números de las stories 1 a 14 se midieron en -O0.
- **Story 14, criterio 3:** con -O1 el plazo de la ISR con ecualizador queda respaldado en Debug (37 556 y 39 016 ciclos con 4 voces). El Release sigue sin medirse (fuera de alcance de esta story).
- **Spec §3.12:** la estimación de unos 20 mil ciclos para la ISR completa quedó por debajo de lo medido: ≈ 38 000 con 4 voces y ecualizador en -O1 (≈ 28 000 sin voces) y ≈ 126 000 en -O0.
