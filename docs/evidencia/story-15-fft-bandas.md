# Story 15 · Bloque 11 · FFT y cálculo de las 8 bandas

**Estado:** EVIDENCIA COMPLETA (2026-11-14) para los tres criterios de la story. Los criterios todavía no están marcados en la story ni en la spec. La línea base de la ISR está en `story-15-linea-base-isr.md`; los tests de la PC y sus resultados, en `story-15-tests.md` (por escribir).

## Procedimiento común
- Firmware de `main` con la captura en la ISR (`audioOutCapture`) y el análisis en el loop principal (`analyzeSpectrum`), compilado en **Debug con -O1** (la configuración quedó así; ver `story-15-linea-base-isr.md`).
- Los resultados se leen en la vista Expressions con la CPU suspendida. Variables: `spectrumPeak` (bin del pico), `spectrumBand` y `spectrumLevel` (se agregan **sin `[8]`**; `nombre[8]` evalúa un elemento fuera del arreglo), `spectrumFrames`, `spectrumFps`, `spectrumLastCycles` y `spectrumMaxCycles`.
- Cada bin de la FFT mide 62,5 Hz. Cada banda es el máximo de sus bins; los niveles son de 0 a 8, con un nivel por cada duplicación de la magnitud (el nivel 1 desde 128; el 8, desde 16 384).

## Prueba 1: Do4 sostenida con la senoide (`story-15-notaSostenida.png` y `story-15-nota-suspendida-variables-faltante.jpg`)
Una voz, EQ plano, tecla Do4 mantenida unos segundos, una sola suspensión con la tecla apretada. El segundo archivo muestra el mismo estado con los arreglos expandidos (el nombre dice "suspendida" por un typo: es la nota sostenida).

| Variable | Valor |
|---|---|
| `spectrumPeak` | 4 |
| `spectrumBand[0]` a `[7]` | 36 · **512** · **1 376** · 8 · 4 · 4 · 8 · 4 |
| `spectrumLevel[0]` a `[7]` | 0 · 3 · 4 · 0 · 0 · 0 · 0 · 0 |
| `spectrumFrames` · `spectrumFps` | 793 · 55 |
| `spectrumLastCycles` · `spectrumMaxCycles` | 272 068 · 272 328 |
| `audioUnderruns` · `audioSaturations` | 0 · 0 |
| `audioIsrMaxCycles[0]` · `[1]` | 28 120 · 31 576 |

- **Pico:** Do4 (261,63 Hz) cae en la posición 4,19 y el pico está en el bin 4, el más cercano.
- **Bandas:** la banda 3 (bins 4 a 7, `spectrumBand[2]`) es la más alta. La banda 2 queda en 0,372 de la 3 (512 / 1 376), es decir −8,59 dB, igual que los −8,6 dB de la teoría de Hann para Do4 (`test_spectrum.c`). Las demás quedan en el ruido (4 a 8), salvo la banda 1, con 36 (el nivel sigue en 0, porque el piso del nivel 1 es 128).
- **Niveles:** 1 376 está entre 1 024 y 2 048, nivel 4; 512 es justo el umbral del nivel 3. Son los niveles que da la escala.
- **Sustain:** no anotamos la posición del pote. La magnitud de 1 376 corresponde a un Sustain cercano al 9 % (amplitud 16 383 · S, por 0,98 de la ventana).
- **Cuadros por segundo:** 55, contra el mínimo de 20. Los 272 068 ciclos del análisis incluyen el tiempo que le quitan la ISR del DMA y SysTick; con 55 cuadros por segundo son 16 ms de llenado de la captura más unos 2,7 ms de análisis.
- **ISR con captura y análisis:** `audioIsrMaxCycles[1]` 31 576 contra 30 840 de la línea base con -O1 sin captura (+2,4 %); `[0]`, 28 120 contra 27 896 (+0,8 %). Con 4 voces se mide en el criterio 3.

## Prueba 2: estabilidad con la nota sostenida y vuelta a 0 al soltar (capturas `Captura de pantalla_20261114_105021` a `_105231`)
Misma configuración. Do4 mantenida y **cinco lecturas**, cada una con una suspensión y su reanudación, **con la tecla apretada**; después se soltó la tecla y se leyó una sexta vez. No se respetaron los 2 a 3 s entre lecturas que habíamos planteado: llevar la captura y la reanudación mientras se mantiene el pulsador toma unos 25 a 30 s (las capturas son de las 10:50:21, 10:50:51, 10:51:15, 10:51:39 y 10:52:08). No cambia el resultado: son lecturas más independientes entre sí.

| Lectura | `spectrumBand` (banda 1 a 8) | `spectrumLevel` (1 a 8) | `spectrumPeak` |
|---|---|---|---|
| 1 (`_105021.jpg`) | 28 · 520 · 1 396 · 8 · 8 · 4 · 8 · 4 | 0 · 3 · 4 · 0 · 0 · 0 · 0 · 0 | 4 |
| 2 (`_105051.jpg`) | 40 · 520 · 1 400 · 4 · 8 · 8 · 8 · 4 | 0 · 3 · 4 · 0 · 0 · 0 · 0 · 0 | 4 |
| 3 (`_105115.jpg`) | 44 · 520 · 1 400 · 4 · 8 · 8 · 4 · 4 | 0 · 3 · 4 · 0 · 0 · 0 · 0 · 0 | 4 |
| 4 (`_105139.jpg`) | 68 · **472** · 1 452 · 32 · 8 · 8 · 8 · 8 | 0 · **2** · 4 · 0 · 0 · 0 · 0 · 0 | 4 |
| 5 (`_105208.jpg`) | 40 · 520 · 1 400 · 4 · 4 · 8 · 8 · 8 | 0 · 3 · 4 · 0 · 0 · 0 · 0 · 0 | 4 |
| Tecla suelta (`_105231.png`) | 0 · 0 · 0 · 0 · 0 · 0 · 0 · 0 | 0 · 0 · 0 · 0 · 0 · 0 · 0 · 0 | 0 |

- **Estable:** en las cinco lecturas con la tecla apretada, todas las barras valen entre 0 y 8 y ninguna varía más de 1 nivel. La banda 3 queda en 4 todas las veces (1 396 a 1 452), las bandas 1 y 4 a 8 en 0, y la banda 2 en 3 salvo la lectura 4, que dio 2.
- **El único cambio de nivel** es la banda 2 en la lectura 4: 472 contra 520 en las demás. 512 es justo el umbral del nivel 3, así que una variación de ±10 % la pasa para un lado o para el otro. Esa variación (±24 en magnitud) es del orden del ruido de cuantización del DAC de 10 bits sobre una nota de ≈ 22 cuentas (el Sustain seguía en ≈ 9 %, porque la banda 3 sigue en 1 400): también aparece en la banda 1 (28 a 68) y en la banda 4 de la lectura 4 (32, el segundo armónico de la cuantización).
- **Al soltar la tecla** (3 s después, con Release corto): el pico en 0, las 8 magnitudes en 0 y los 8 niveles en 0.
- **Sustain:** el pedido era subirlo por encima del 60 %, y no se cumplió: la magnitud de la banda 3 no cambió respecto de la prueba 1. Las lecturas se hicieron con niveles bajos (3 y 4). Queda anotado: el borde del umbral no se vio con una nota fuerte.

### Hallazgo del depurador: `audioIsrMaxCycles[1]` = 2 805 223 308
Desde la lectura 2, `audioIsrMaxCycles[1]` marca 2 805 223 308 ciclos (≈ 28 s), contra 31 576 en la lectura 1. No es un costo real de la ISR: una ISR dura como mucho un bloque (199 936). `spectrumMaxCycles` no aparece en estas capturas. Se atribuye (sin confirmar) a que la CPU quedó frenada dentro de la ISR del DMA durante la primera suspensión mientras el Timer0 seguía contando: el valor coincide con el tiempo entre las capturas 1 y 2 (30 s). Por eso `audioIsrMaxCycles` solo se lee con **una** suspensión por sesión, y cualquier valor mayor que un bloque se descarta y se repite la sesión. Los 31 576 de la lectura 1 son el valor válido.

## Prueba 3: cuatro voces con el ecualizador y el análisis corriendo (`story-15-criterio3.jpg`)
Sesión desde cero, las 4 teclas (Do4, Mi4, Sol4 y Do5) apretadas a la vez, Sustain en 63 % (`envParams.sustainLevel` = 20 693), **una sola suspensión** con las teclas apretadas.
- **Configuración (confirmada por quien midió; la captura no la muestra):** ecualizador exigente cargado (graves −12 dB, medios +6 dB y agudos +6 dB), las 4 ondas recorridas con las 4 teclas y la suspensión en la última tanda.

| Variable | Valor |
|---|---|
| `audioUnderruns` · `audioSaturations` | **0** · 93 087 |
| `audioIsrMaxCycles[0]` a `[4]` | 28 944 · 33 076 · 35 812 · 38 468 · **40 560** |
| `spectrumFrames` · `spectrumFps` | 9 028 · **55** |
| `spectrumLastCycles` · `spectrumMaxCycles` | 280 952 · 281 836 |
| `spectrumPeak` | 8 |
| `spectrumBand` (banda 1 a 8) | 16 · 2 200 · 7 916 · 9 416 · 1 840 · 360 · 200 · 72 |
| `spectrumLevel` (banda 1 a 8) | 0 · 5 · 6 · 7 · 4 · 2 · 1 · 0 |

- **ISR por debajo de la mitad del bloque (99 968):** con 4 voces, 40 560 ciclos (20,3 % del bloque), a 59 408 del límite. Sobre la línea base con -O1 y EQ exigente sin captura (39 016), sube 1 544 ciclos (+4,0 %). Ningún valor pasa de un bloque: sin artefactos del depurador.
- **Análisis:** 55 cuadros por segundo (mínimo 20). El peor análisis (`spectrumMaxCycles`) fue de 281 836 ciclos, 2,8 ms con el tiempo que le quitan las ISR (con una voz: 272 328, +3,5 %). Cruce: 16 ms de llenado más 2,8 ms de análisis dan 18,8 ms por cuadro, ≈ 53 por segundo, coherente con el promedio de 55.
- **`audioUnderruns`:** 0.
- **Bandas:** el pico cae en el bin 8 (Do5) y la banda 4 es la más alta (9 416, contra 7 916 de la banda 3). Con EQ exigente y ondas con armónicos, es esperable que las notas más agudas pesen más (low-shelf de −12 dB en los graves; +6 dB en medios y agudos) y que las bandas 5 a 8 reciban energía de los armónicos. No es parte del criterio y no lo medimos con otra configuración.

## Prueba 4: notas Re4 a Do5 con la senoide y distintas amplitudes (`story-15-criterio1-*.jpg`)
Una voz, senoide, EQ plano, una tecla mantenida y una suspensión por nota; la nota La4 se repitió con tres Sustain. En cada captura se ve `envParams.sustainLevel`. La magnitud esperada de la banda más alta es `sustainLevel / 2` (la amplitud de la nota después de la ganancia de 1/2 de la mezcla) por la respuesta de la ventana Hann, |W(δ)| = |sinc(δ) / (1 − δ²)|, con δ la distancia entre el bin y la frecuencia (calculada a mano; es la misma que usan los tests de la PC).

| Nota | Hz | `sustainLevel` | `spectrumPeak` (esperado) | Banda más alta: medida · esperada | Otra banda que sube |
|---|---|---|---|---|---|
| Re4 (`-Re4.jpg`) | 293,66 | 20 693 | **5** (5) | banda 3: 9 744 · 9 746 | banda 2: 844 (−21,2 dB; teoría −21,3) |
| Mi4 (`-Mi4.jpg`) | 329,63 | 20 669 | **5** (5) | banda 3: 9 788 · 9 857 (−0,7 %) | banda 4: 136; banda 2: 264 |
| Fa4 (`-Fa4.jpg`) | 349,23 | 20 693 | **6** (6) | banda 3: 9 244 · 9 249 | banda 4: 268; banda 2: 212 |
| Sol4 (`-sol4.jpg`) | 392,00 | 20 693 | **6** (6) | banda 3: 9 864 · 9 860 | banda 4: 716 (−22,8 dB; teoría −22,8) |
| La4 Sustain casi máximo | 440,00 | 28 173 | **7** (7) | banda 3: 14 072 · 14 072 | banda 4: 7 436 (−5,5 dB; teoría −5,5) |
| La4 Sustain a la mitad | 440,00 | 13 032 | **7** (7) | banda 3: 6 504 · 6 510 | banda 4: 3 436 (−5,5 dB) |
| La4 Sustain casi mínimo | 440,00 | 1 861 | **7** (7) | banda 3: 932 · 930 | banda 4: 492 (−5,5 dB) |
| Si4 (`-Si4.jpg`) | 493,88 | 20 603 | **8** (8) | **banda 4**: 10 228 · 10 230 | banda 3: 5 928 (−4,7 dB; teoría −4,8) |
| Do5 (`-Do5.jpg`) | 523,25 | 20 578 | **8** (8) | **banda 4**: 9 412 · 9 411 | banda 3: 2 508 (−11,5 dB; teoría −11,5) |

Niveles `spectrumLevel` de las bandas 2 · 3 · 4 (las otras cinco valen 0 en todas las capturas):

| Captura | Banda 2 | Banda 3 | Banda 4 |
|---|---|---|---|
| Re4 | 3 | 7 | 0 |
| Mi4 | 2 | 7 | 1 |
| Fa4 | 1 | 7 | 2 |
| Sol4 | 0 | 7 | 3 |
| La4 Sustain casi máximo | 0 | 7 | 6 |
| La4 Sustain a la mitad | 0 | 6 | 5 |
| La4 Sustain casi mínimo | 0 | 3 | 2 |
| Si4 | 0 | 6 | 7 |
| Do5 | 0 | 5 | 7 |

- **Pico:** en las 9 capturas cae en el bin más cercano a la frecuencia.
- **Banda más alta:** es la que contiene ese bin: la 3 de Re4 a La4 y la 4 con Si4 y Do5 (el bin 8). Las vecinas suben con menos nivel, y con la diferencia en dB que predice la ventana, a ±0,1 dB.
- **Magnitud:** coincide con la esperada a ±1 % en las 9 capturas (el peor caso, Mi4, −0,7 %).
- **Distintas amplitudes (La4):** al bajar el Sustain de 28 173 a 13 032 y a 1 861, la banda 3 baja de 14 072 a 6 504 y a 932 (la proporción que sigue al Sustain), el pico queda en el bin 7 y los niveles bajan de 7 a 6 y a 3.

### Hallazgos del depurador en esta sesión
Esta sesión usó varias suspensiones (una por captura):
- **`spectrumMaxCycles` = 14 510 916** desde la captura de Fa4. Es imposible como costo de un análisis (≈ 145 ms): es la suspensión del depurador dentro del análisis. No se usa.
- **`audioUnderruns` sube con las suspensiones:** 0 hasta La4 (Sustain casi máximo), 2 con La4 a la mitad, 3 con La4 casi mínimo y con Si4 y 4 con Do5. Es el mismo efecto de las stories 6 a 8 (se atribuye al depurador, sin confirmar). La prueba del criterio 3, con una sola suspensión, dio 0.

## Prueba 5: armónicos sueltos de la onda del editor sobre Do5 (`story-15-armonico2.png`, `-armonico5.jpg`, `-armonico8.jpg`, `-armonico16.jpg` y `-armonico30.jpg`)
Una voz, EQ plano, la tecla Do5 mantenida y una suspensión por armónico. En la app (Python, `editor_window.py`): fundamental Do5, el armónico 1 en 0 % y un único armónico k al 100 %, enviado a la placa ("Onda enviada: la placa confirmó la recepción"), con la quinta onda activa (`activeWave` = 4) y una suspensión por captura. En cada captura se ven la ventana de la app con los sliders y el espectro del editor, y las variables del depurador. Con el armónico k solo, la placa suena un seno a k · 523,25 Hz. Cada bin de la FFT mide 62,52 Hz.

| Armónico | Frecuencia | Posición | `spectrumPeak` (esperado) | Banda más alta | `sustainLevel` | Magnitud medida · esperada |
|---|---|---|---|---|---|---|
| 2 | 1 046,50 Hz | 16,74 | **17** (17) | banda 5 (`[4]`) | 20 587 | 9 836 · 9 849 (−0,1 %) |
| 5 | 2 616,25 Hz | 41,85 | **42** (42) | banda 6 (`[5]`) | 20 595 | 10 132 · 10 143 (−0,1 %) |
| 8 | 4 186,00 Hz | 66,96 | **67** (67) | banda 7 (`[6]`) | 20 587 | 10 200 · 10 280 (−0,8 %) |
| 16 | 8 372,00 Hz | 133,91 | **134** (134) | banda 8 (`[7]`) | 20 595 | 10 224 · 10 244 (−0,2 %) |
| 30 | 15 697,50 Hz | 251,08 | **251** (251) | banda 8 (`[7]`) | 20 603 | 10 000 · 10 260 (−2,5 %) |

La magnitud esperada es `sustainLevel / 2` por la respuesta de la ventana Hann en el bin más cercano (|W(δ)| = 0,957, 0,985, 0,999, 0,995 y 0,996). Niveles de la banda más alta: 7 en las cinco capturas.

Las otras bandas (magnitud · nivel), todas mucho más bajas que la del armónico:

| Armónico | Bandas 1 a 8 (magnitud) | Niveles de las bandas 1 a 8 |
|---|---|---|
| 2 | 24 · 0 · 4 · **684** · 9 836 · 16 · 80 · 36 | 0 · 0 · 0 · 3 · 7 · 0 · 0 · 0 |
| 5 | 36 · 36 · 20 · 24 · 16 · 10 132 · 48 · 176 | 0 · 0 · 0 · 0 · 0 · 7 · 0 · 1 |
| 8 | 36 · 16 · 28 · 68 · 336 · 44 · 10 200 · 220 | 0 · 0 · 0 · 0 · 2 · 0 · 7 · 1 |
| 16 | 180 · 8 · 8 · 36 · 52 · 564 · 280 · 10 224 | 1 · 0 · 0 · 0 · 0 · 3 · 2 · 7 |
| 30 | 24 · 40 · 164 · 72 · 332 · 648 · 520 · 10 000 | 0 · 0 · 1 · 0 · 2 · 3 · 3 · 7 |

- **Pico y banda:** en los cinco armónicos el pico cae en el bin más cercano y la banda más alta es la que lo contiene (5, 6, 7, 8 y 8). Cubre las bandas 5 a 8; la 4 con el Do5 solo (Prueba 4) y la 3 con las notas Do4 a La4.
- **Armónico 2, banda 4 en 684 (nivel 3):** es la vecina que predice la ventana, no una espuria: el bin 15 está a 1,74 bins de la frecuencia y |W| = 0,066, o sea 10 293 · 0,066 ≈ 680 (−23,2 dB).
- **Espurias:** las otras bandas quedan entre 25 y 35 dB por debajo (por ejemplo, 564 con el armónico 16: −25,2 dB; 648 con el 30: −23,8 dB) y crecen con la frecuencia del armónico. Se atribuyen (sin confirmar) a que el DDS lee la tabla de 256 muestras sin interpolación: un seno de 30 ciclos tiene 8,5 muestras por ciclo. No cambian el pico ni la banda más alta.
- **Armónico 30:** la magnitud es 2,5 % menor que la esperada (10 000 contra 10 260), el peor de los cinco. Se atribuye (sin confirmar) a la misma lectura sin interpolación. El pico cae en el bin 251, a 15 697,5 Hz, cerca del límite de 16 kHz (Nyquist, 16 005 Hz).

### Hallazgos del depurador (misma sesión)
- `spectrumMaxCycles` = 690 290 844 (armónicos 2 y 5) y 1 654 046 760 (8, 16 y 30): imposibles como costo de un análisis. Es la suspensión dentro del análisis. No se usan.
- `audioUnderruns`: 1 (armónicos 2 y 5) y 2 (8, 16 y 30), el mismo efecto de las suspensiones repetidas que en la Prueba 4.

## Prueba 6: una onda de varios armónicos, la cuadrada de Do4 (`story-15-varios-armonicos.jpg`)
Una voz, EQ plano, onda cuadrada del firmware (`activeWave` = 0), tecla Do4 mantenida y una suspensión; `sustainLevel` = 20 611. La cuadrada es una suma de armónicos impares con amplitud 1/k hasta el 29.º (7 587 Hz), así que tiene que subir la banda de cada armónico, con las relaciones que da la ventana (calculadas a mano con |W(δ)| en el bin más cercano de cada armónico, sin usar el firmware).

| Banda | Armónicos que la forman | `spectrumBand` | Relativa a la banda 3: medida · esperada | `spectrumLevel` |
|---|---|---|---|---|
| 1 | — | 84 | 0,008 · 0 | 0 |
| 2 | fuga de la fundamental | 4 080 | 0,373 · 0,372 (−8,6 dB) | 5 |
| **3** | 1.º (261,63 Hz) | **10 928** | 1 · 1 | **7** |
| 4 | 3.º (784,9 Hz) | 3 272 | 0,299 · 0,299 (−10,5 dB) | 5 |
| 5 | 5.º y 7.º (1 308 y 1 831 Hz) | 2 220 | 0,203 · 0,203 (−13,8 dB) | 5 |
| 6 | 9.º a 15.º | 1 144 | 0,105 · 0,105 (−19,6 dB) | 4 |
| 7 | 17.º a 29.º | 644 | 0,059 · 0,059 (−24,6 dB) | 3 |
| 8 | ninguno (el 30.º queda fuera: la cuadrada solo tiene impares y llega al 29.º) | 40 | 0,004 · 0 | 0 |

- **Pico:** `spectrumPeak` = 4, el bin de Do4.
- **Bandas:** suben las bandas de cada armónico, y decrecen con la frecuencia como 1/k. Las relaciones coinciden con las calculadas a mano a tres cifras (0,373, 0,299, 0,203, 0,105 y 0,059). La banda 8 queda en 40 (solo ruido) y la banda 1, en 84 (fuga lejana y ruido): los dos niveles son 0.
- **Valores sin artefactos:** `spectrumMaxCycles` = 278 524 y `spectrumLastCycles` = 272 280, un costo válido de un análisis. `audioUnderruns` = 2 y `audioSaturations` = 1 898 vienen de las suspensiones y de las otras notas de esa sesión; no se usan.

## Estado de los criterios de la story 15
- **Criterio 1 (pico en el bin más cercano, banda más alta la que lo contiene, bandas de cada armónico):** Pruebas 1 y 4 (notas Do4 a Do5 y distintas amplitudes), 5 (armónicos sueltos 2, 5, 8, 16 y 30) y 6 (onda de varios armónicos).
- **Criterio 2 (entre 0 y 8, ±1 nivel entre lecturas, vuelta a 0 al soltar):** Prueba 2.
- **Criterio 3 (ISR por debajo de la mitad del bloque, al menos 20 cuadros por segundo y `audioUnderruns` en 0, con 4 voces y ecualizador):** Prueba 3 y `story-15-linea-base-isr.md`.
