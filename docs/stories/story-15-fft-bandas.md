# Story 15: FFT y cálculo de las 8 bandas

**Bloque:** 11 · **Spec:** §3.6, §3.12, §6 Bloque 11 · **Estado:** terminada (ver `docs/evidencia/story-15-tests.md`, `story-15-linea-base-isr.md` y `story-15-fft-bandas.md`)

## Story
Como evaluador, quiero comprobar con señales conocidas que el análisis ubica cada frecuencia en su banda sin interrumpir el audio, para confiar en lo que va a mostrar el visualizador.

## Criterios de aceptación
- [x] Dado un tono de frecuencia conocida sonando (una nota, o un armónico solo de la onda del editor) con distintas amplitudes, cuando leemos el análisis en el depurador, entonces el pico está en el bin más cercano a esa frecuencia y la banda más alta es la que contiene ese bin; las vecinas pueden subir por la ventana, con menos nivel. Con una onda de varios armónicos, suben las bandas de cada armónico.
- [x] Dada una nota sostenida, cuando leemos los niveles de las 8 bandas varias veces, entonces cada una queda entre 0 y 8 y no varía más de ±1 nivel entre lecturas, y al soltar la tecla todas vuelven a 0.
- [x] Dadas cuatro voces sonando con el ecualizador activo y el análisis corriendo, cuando leemos en Debug los ciclos de la ISR de audio y del análisis, entonces la ISR sigue por debajo de la mitad del bloque (≈ 100 000 de 199 936 ciclos), el análisis completa al menos 20 cuadros por segundo y `audioUnderruns` queda en 0.

## Plan
- Spec primero: medir la ISR con el ecualizador y sin captura como línea base (pendiente de la story 14), y fijar en la spec la escala y el mapeo a 0–8, el suavizado con la caída y la tasa de análisis. El cambio a una matriz de 8×8 con 8 bandas de una octava ya está registrado (§3.6).
- Tablas de ventana Hann y factores de giro generadas en la PC, junto con las demás.
- Lógica con tests primero en la PC: FFT de 512 puntos en Q15 y cálculo de bandas. Tests con senoides en bins conocidos y entre bins, varios armónicos, distintas amplitudes, agrupación de bins, niveles estables con una señal fija y caída gradual al cortarla.
- Placa: la ISR copia cada bloque al buffer de captura y deja de escribir cuando se llena. El loop principal, después del parser, lo procesa y lo libera. Contadores de ciclos del análisis, como los de la ISR.
- Evidencia en placa con el depurador: notas y armónicos sueltos contra la banda esperada, niveles con una nota sostenida y al soltarla, y ciclos con 4 voces y el ecualizador activo.

## Fuera de alcance
- La matriz, Timer + DMA y la codificación del buffer de LEDs (Bloques 12 y 13).
- Mostrar el espectro en la app.
- Medir la respuesta del ecualizador con la FFT (posible evidencia extra para la story 14, no es criterio de esta).
- Cambiar la frecuencia de muestreo o el tamaño de la FFT.
- Medir en Release.

## Notas
- Decisiones confirmadas:
  - Una sola matriz de 8×8: 8 bandas de una octava desde 62,5 Hz hasta 16 kHz, una por columna (bins 1, 2–3, 4–7, 8–15, 16–31, 32–63, 64–127 y 128–255; spec §3.6).
  - Estable: cada banda varía como mucho ±1 nivel entre lecturas con una nota sostenida.
  - Solo Debug, que es lo que se carga hoy. Tasa mínima: 20 cuadros por segundo.
- Lo que se apartó del plan (2026-11-14):
  - **Medición de los ciclos del análisis:** no con SysTick "como los de la ISR". SysTick da la vuelta cada 1 ms y la ISR con ecualizador dura más: las primeras lecturas de `audioIsrMaxCycles` eran falsas (≈ 96 mil con 0 voces). Los dos contadores usan ahora el Timer0 (módulo `cycle_counter`).
  - **Debug con `-O1`:** con el Debug sin optimizar (`-O0`) la ISR con 4 voces y ecualizador tardaba ≈ 127 000 ciclos (64 % del bloque) y no cumplía el criterio 3. Con `-O1` (cambio en `.cproject`) tarda ≈ 40 000. Las stories 1 a 14 se midieron en `-O0`.
  - **Redondeo en las mariposas de la FFT**, que no estaba en el plan: lo agregó el test de escala máxima.
  - **La nota de los bordes de banda** se corrigió con los tests: Do4 deja la banda 2 a 8,6 dB de la 3 (no 10 a 13), Si4 sube la banda 4 4,8 dB sobre la 3 y Sol4 queda casi en una sola banda (22,8 dB).
  - **Sin refactor ni revisión de código** antes de pasar a la placa.
- Bordes de banda: la ventana Hann reparte la energía de un tono en unos ±2 bins. Por un cálculo a mano (a confirmar con los tests), con la senoide Do4 (bin 4,19) la banda 2 queda unos 8,6 dB por debajo de la 3, y Si4 (493,9 Hz, bin 7,90) sube la banda 4 unos 4,8 dB sobre la 3. Sol4 (392 Hz, bin 6,27) queda casi en una sola banda (la 3, 22,8 dB sobre la 4). Confirmado con los tests de la PC.
- Señales de prueba sin hardware nuevo: las notas Do4…Do5 con la senoide (bins 4 a 8) y armónicos sueltos del editor sobre Do5, hasta el 30 (15,7 kHz, cerca de los 16 kHz de Nyquist). Las otras ondas suben varias bandas por sus armónicos.
- Hardware: placa, adaptador USB-TTL con la app (para las ondas del editor), teclas y potes, y depurador. La etapa de audio es opcional (solo para oír).

## Sugerencias (tu decisión)
- Medir los ciclos del análisis igual que `audioIsrMaxCycles`, que ya se lee en Expressions y tiene el procedimiento documentado.
