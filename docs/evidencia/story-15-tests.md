# Story 15: resultados de los tests (2026-11-14)

**Entorno:** gcc con `-std=c99 -Wall -Wextra -Werror` para la lógica en la PC; `arm-none-eabi-gcc` del IDE (MCUXpresso 25.6.136) con `-std=gnu11 -mcpu=cortex-m3 -mthumb -Wall -Wextra` para los módulos de hardware; pytest en el venv de `app/`.

## Resultado final

| Suite | Comando | Resultado |
|---|---|---|
| Firmware (lógica en la PC) | `make -C firmware/tests test` | 15 suites OK (14 anteriores y `test_spectrum`), 0 `FALLA` |
| Aplicación | `app/.venv/bin/pytest -q` | 85 passed (sin cambios respecto de la story 14) |
| Firmware (hardware, solo compila) | `arm-none-eabi-gcc -c … -Wall -Wextra` con `-O0` y `-O1` sobre `cycle_counter.c`, `audio_out.c`, `sintetizador.c`, `spectrum.c` y `dac_level.c` | 0 diagnósticos en los 10 casos. El enlace y la prueba en placa son del IDE (ver "Verificación en placa") |

Tests nuevos: 19 en `test_spectrum.c`, 4 en `test_tables.c` (ventana Hann y factores de giro) y 3 en `test_dac_level.c`.

## Tests de la lista (ciclo rojo → verde)

"Rojo" es lo que vimos al correr el test antes de escribir el código, con un stub que devuelve ceros cuando la función ya estaba declarada. Cuando un test pasó en la primera corrida, no hubo rojo ni código nuevo, y se indica. Esos tests son guardas (fijan comportamiento que ya daba el código anterior), y en cada uno hicimos al menos una mutación de prueba (una copia del código con un error puesto a propósito) para ver que el test lo detecta.

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 18 | `testHannEsSimetricaConExtremoEnCeroYMaximoEnElCentro` y `testHannVale0_5ALaMitadDelFlancoYSuGananciaCoherenteEs0_5` (`test_tables.c`) | C1 | No compilaba: sin `hannWindow` ni `FFT_SIZE` | Ventana Hann periódica de 512 puntos en Q15 en `gen_tables.py` |
| 19 | `testFactoresDeGiroCoincidenConCosenoYSenoDeLaDefinicion` y `testFactoresDeGiroEnLosAngulosConocidos` | C1 | No compilaba: sin `fftTwiddleCos` ni `fftTwiddleSin` | Dos tablas de 256 valores Q15, con el seno positivo (el signo del giro lo pone la FFT) |
| 1 | `testPicoEnBinExacto` (`test_spectrum.c`) | C1 | 10 `FALLA`: el stub dejaba el pico en el bin 0 en los 5 casos (bins 4, 8, 16, 100 y 255) | `spectrumMagnitudes`: Hann, bit-reverse, mariposas con escalado de 1/2 por etapa y módulo con raíz entera |
| 2 | `testPicoEntreBins` | C1 | Pasó sin código nuevo. Las 8 notas dan el bin más cercano | — (mutación con bins esperados equivocados: 2 fallas) |
| 3 | `testFugaDeHannAcotada` | C1 | Pasó sin código nuevo. A más de 3 bins del pico, todo queda al menos 30 dB abajo | — (mutación sin ventana: 15 fallas) |
| 4 | `testMagnitudIgualALaAmplitudEnBinCentrado` | C1 | 8 `FALLA`: la magnitud salía en A/4 (500 → 125) | `MAGNITUDE_GAIN = 4`: Hann ½ y escalado por 1/512 |
| 5 | `testSinDesbordeAEscalaMaxima` | C1 | 3 `FALLA`: ruido de truncado de hasta 24 en bins que valen 0 (más que la tolerancia de 16) y la continua de −32768 daba 0 en vez de ≈ 65 535 | Redondeo (`+ 1` antes de cada `>> 1`) en las mariposas: el ruido baja a 4–12 |
| 6 | `testContinuaNoEntraEnNingunaBanda` | C1 | 2 `FALLA`: la banda 1 valía 0 (el stub) y tiene que valer D | `spectrumBands`: máximo por banda, con los bins `2^b … 2^(b+1) − 1` |
| 7 | `testAgrupacionDeBins` | C1 | Pasó sin código nuevo | — (mutación con cada banda sin su último bin: 8 tonos fallan) |
| 8 | `testDo4Banda3SobreBanda2` | C1 | Pasó sin código nuevo. 8,6 dB | — |
| 9 | `testSi4Banda4SobreBanda3` | C1 | Pasó sin código nuevo. 4,7 dB (teoría 4,8) | — |
| 10 | `testSol4CasiEnUnaSolaBanda` | C1 | Pasó sin código nuevo. La banda 3 queda 22,8 dB sobre la 4 | — (mutaciones sin ventana y con suma en vez de máximo: fallan) |
| 11 | `testVariosArmonicosSubenSusBandas` | C1 | Pasó sin código nuevo | — (mutación con cada banda sin su primer bin: 6 bandas de la serie de octavas fallan) |
| 12 | `testNivelesEntre0Y8` | C2 | 9 `FALLA`: el tono a fondo de escala y las 8 barras con la magnitud máxima no llegaban a 8 con el stub | `levelOf` y `spectrumLevels`: un nivel por cada umbral (128, 256 … 16 384) |
| 13 | `testSilencioDaCero` | C2 | Pasaba con el stub (vacuo); con la implementación, sigue verde | — |
| 14 | `testNivelSubeUnPasoPorCada6dB` | C2 | 16 `FALLA`: 8 de los 9 escalones y 8 de los 10 bordes exactos (el 90 y los bordes que esperan 0 pasaban con el stub). Entre el 12 y el 14, 25 `FALLA` | Igual que el 12. Mutación con el umbral inicial en 256: 15 fallas |
| 15 | `testNivelesEstablesConTonoFijo` | C2 | Pasó sin código nuevo. Con 16 fases de arranque, cada barra varía como mucho 1 nivel | — (mutación sin ventana: varían hasta 3 niveles) |
| 16 | `testNivelEstableConPocaAmplitud` | C2 | Pasó sin código nuevo. Un tono de 5 cuentas del DAC (cuantizado) se ve y es estable | — (mutación con el umbral inicial en 16: falla) |
| 17 | `testCaidaGradualAlCortar` | C2 | 8 `FALLA`: la barra caía de 8 a 0 de golpe | `spectrumLevels` baja un nivel por cuadro y sube de inmediato |
| 20 | `testDacValueToQ15EnElCentroYLosExtremos`, `…SubeDeA64PorCuenta` y `…ConLaConversionDelPaso4` (`test_dac_level.c`) | C1 | 12 `FALLA` con el stub | `dacValueToQ15`: `(v − 512) · 64`, de −32768 a 32704 |
| 21 | `testSpectrumPeakBinElMayorSinContarLaContinua` y `testSpectrumPeakBinDeUnaNota` (`test_spectrum.c`) | C1 | 7 `FALLA` con el stub | `spectrumPeakBin`: el bin de mayor magnitud entre el 1 y el 255. Mutaciones incluyendo el bin 0 y con el empate al más alto: fallan |

Los tests que pasaron en la primera corrida (2, 3, 7 a 11, 13, 15 y 16) son guardas. Los criterios 1 a 3 de la story 15 dependen en parte del hardware: se verifican en placa (ver abajo). El test 13 pasaba con el stub porque un stub que no hace nada deja las barras en 0; quedó verificado con la implementación real.

## Decisiones tomadas durante el ciclo

- **Muestras que se capturan:** las que van al DAC (ya saturadas a 10 bits), llevadas a Q15 con `dacValueToQ15`. Registrado en la spec §3.6 antes de empezar.
- **Magnitud, escala y suavizado:** magnitud de banda = máximo de los bins; la magnitud de un bin se normaliza para que una senoide centrada dé su amplitud; un nivel por cada 6 dB, con el nivel 1 desde 128 y el 8 desde 16 384 (fondo de escala = 32 768); sube de inmediato y baja un nivel por cuadro. Registrado en la spec §3.6.
- **Redondeo en las mariposas:** `+ 1` antes de cada `>> 1`. No estaba en el plan: lo agregó el test 5 (ruido de truncado de −62 dB contra −78 dB redondeando). Se consultó antes de agregarlo.
- **La continua no entra en ninguna banda, pero la Hann la reparte en el bin 1:** el test 6 de la lista decía "todas las bandas en 0" y era incorrecto (con Hann, una continua D da 2D en el bin 0 y D en el bin 1). El test comprueba que la banda 1 vale D (y no 2D) y que las demás valen 0. Un offset sostenido en la captura subiría la banda 1; no se corrige (la captura es `v − 512`).
- **API de niveles:** `spectrumLevels(bands, levels)`: el arreglo `levels` es a la vez el estado anterior (entrada) y el resultado (salida), así el suavizado no necesita otra estructura.
- **Nota de la story corregida:** la story decía que con Do4 la banda 2 queda "unos 10 a 13 dB" por debajo de la 3. La cuenta a mano y los tests dan 8,6 dB (y 4,8 dB para Si4, 22,8 dB para Sol4). Se corrigió en la story.
- **Contador de ciclos:** `audioIsrMaxCycles` se midió al principio con SysTick, que da la vuelta cada 100 000 ciclos, y las lecturas eran falsas (96 mil con 0 voces, 25 mil con 4). Se reemplazó por el Timer0 a 25 MHz (módulo `cycle_counter`, resolución de 4 ciclos), usado también por el análisis.
- **Debug compila con `-O1`** (`.cproject`, solo la configuración Debug): con `-O0` la ISR con ecualizador y 4 voces tarda ≈ 127 000 ciclos (64 % del bloque, 27 a 28 % sobre el límite de la story); con `-O1`, ≈ 40 000.
- **Análisis en el loop principal:** `analyzeSpectrum` toma la captura, la libera apenas `spectrumMagnitudes` copió las muestras (así la ISR llena el cuadro siguiente mientras se calculan las bandas) y publica el resultado en variables globales `volatile` para el depurador (`spectrumPeak`, `spectrumBand`, `spectrumLevel`, `spectrumFrames`, `spectrumFps`, `spectrumLastCycles` y `spectrumMaxCycles`).
- **Handshake de la captura sin bloqueos:** la ISR solo sube `captureCount` (y solo si no está lleno), y el loop solo lo pone en 0 (y solo si está lleno). La precarga de `audioOutInit` no cuenta: se reinicia antes de arrancar el DMA.
- **Nombres:** identificadores en inglés (constitución, principio 8). El comentario de la variable global `spectrumPeak` evita el nombre de la función `spectrumPeakBin`.
- **Sin refactor ni revisión de código:** decidimos no hacer el refactor de `test_spectrum.c` (cerca de 700 líneas, con helpers parecidos) ni el `/code-review` antes de pasar a la placa.

## Fuera de esta story

- Medir en Release (fuera de alcance de la story; sigue en el backlog).
- Una señal con offset sostenido sube la banda 1 (ver arriba).
- Mostrar el espectro en la app y las matrices (Bloques 12 y 13).

## Verificación en placa

Los tres criterios de la story se verificaron con el depurador, con la CPU suspendida. Los resultados y las capturas están en `docs/evidencia/story-15-linea-base-isr.md` (la ISR con ecualizador, en -O0 y -O1) y `docs/evidencia/story-15-fft-bandas.md` (pruebas 1 a 6). Las variables `audioIsrMaxCycles`, `spectrumMaxCycles` y `audioUnderruns` solo se leyeron con una suspensión por sesión para el criterio 3. Con varias suspensiones, el depurador deja valores imposibles (por ejemplo, 2 805 223 308 ciclos), y esos valores se descartan.
