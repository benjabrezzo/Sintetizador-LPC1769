# CLAUDE.md — Sintetizador LPC1769

TP integrador de Electrónica Digital III (FCEFyN-UNC). Es didáctico: se prioriza que se entienda y se pueda
defender en la presentación.

**Orden de prioridad:** `docs/constitution.md` → `docs/especificacion_sintetizador_lpc1769.md` → este archivo.
Si algo de acá choca con la spec, manda la spec. Un cambio de comportamiento se registra primero en la spec.

> Este archivo pisa al `CLAUDE.md` de `examenElecDigIII/`. Ese pide registros directos; **acá se usan los
> drivers** (principio 9).

## Forma de trabajo

- Una sola story por vez, en `docs/stories/`, dentro de un bloque de la spec (secciones "Stories" y "Regla de avance").
- Un bloque se aprueba con todos sus criterios marcados en la spec y la evidencia en `docs/evidencia/`.
- Las fotos de evidencia traen GPS en el EXIF: antes de commitear, limpiarlas con `jpegtran -copy none -outfile <destino> <origen>`.
- Los tests pasan antes de cerrar una story (spec §7.6).
- No ejecutar comandos sin el OK de Benja (regla general del equipo).
- Los commits los hace Benja: Claude prepara los cambios y sugiere el mensaje, pero no corre `git commit`.
- Antes de cada prueba con hardware, avisar a Benja qué componentes tienen que estar armados (backlog, notas).

## Estructura

```
sintetizador/                                raíz del repo y del proyecto de MCUXpresso
  firmware/
    src/                                     .c (los compila MCUXpresso), incluido el startup
      sintetizador.c                         main
      (módulos de hardware y de lógica: spec §5, cada uno nace con su bloque)
      tables.c                               GENERADO por firmware/tools/gen_tables.py, no editar
    inc/                                     .h
      config.h                               pines, canales, Fs, prioridades, tamaños
      tables.h                               GENERADO, no editar
    tests/                                   tests de lógica en la PC (gcc + make)
    tools/gen_tables.py                      ondas, incrementos, ventana y factores de giro
  app/                                       aplicación de escritorio
    ui/                                      lo único que importa PySide6
    tests/                                   pytest
  hardware/                                  esquemáticos, BOM y scripts de cálculo (desde el Bloque 3)
  docs/
    constitution.md                          principios innegociables
    especificacion_sintetizador_lpc1769.md   fuente de verdad
    backlog.md                               historias priorizadas
    protocolo.md                             tramas UART (por crear, Bloque 9)
    stories/  evidencia/                     una story por vez / mediciones y capturas
  README.md                                  entorno y pasos para reconstruir (al final del proyecto)
```

**Separación de capas (principio 5):** los módulos de lógica de la spec §5 no incluyen `LPC17xx.h` ni
`lpc17xx_*.h`. Reciben datos por parámetro y usan `stdint.h`, para compilar igual en la PC.

## Build y entorno

- MCUXpresso 25.6 dentro del distrobox `nxp-mcuxpresso`. El proyecto depende de `CMSISv2p00_LPC17xx`.
- El include path `${workspace_loc:/CMSISv2p00_LPC17xx/Drivers/inc}` ya está en `.cproject`. Al mover los
  fuentes, el IDE también tiene que apuntar a `firmware/src` (Source Location) y a `firmware/inc` (Includes).
- La biblioteca de C es Redlib (semihost). El `printf` va al depurador y solo se llama si `CoreDebug->DHCSR`
  indica `C_DEBUGEN`; sin depurador, el `BKPT` colgaría el arranque. El reinicio sin depurador se prueba
  desenchufando el USB, no con el botón de reset.
- Depurador: no hay lectura en vivo (Live Expressions no existe en este IDE y Real Time Expressions de
  PEmicro no actualiza). Las variables se leen en la vista Expressions con la CPU suspendida. Frenar la CPU
  dentro de la ISR del DMA desfasa las mitades: relanzar la sesión antes de medir.
  Leer `audioUnderruns` con una sola suspensión al final de la prueba: suspender y reanudar varias veces lo hizo subir (0 → 2 en la story 8; se atribuye al depurador, sin confirmar).
  Lo mismo vale para `audioIsrMaxCycles`: con varias suspensiones llegó a marcar 2 805 223 308 ciclos (story 15), un valor imposible (un bloque dura 199 936); se atribuye a una suspensión dentro de la ISR con el Timer0 contando, sin confirmar. Un valor mayor que un bloque se descarta y se repite la sesión.
  Expressions no evalúa macros de registros (`LPC_GPIO2->FIOPIN` falla): usar la dirección, p. ej. `*(volatile unsigned int *)0x2009C054` (FIO2PIN; el bit 11 es P2.11).
  Después de Terminate, el audio queda con ruido (probablemente la CPU frenada y el DMA repitiendo las dos
  mitades): para escuchar, desenchufar y volver a enchufar el USB.
- Tests: `make -C firmware/tests test` (cada `test_*.c` es un programa) y, en `app/`, `.venv/bin/pytest`.
- Los drivers viven en un repo aparte (`LPC17xx-CMSIS-Driver-Enhancement`), fuera de este. El IDE los resuelve por
  nombre de proyecto en el workspace, así que la ruta en disco cambia según la máquina. En Linux requieren
  `LPC17xx.h` con mayúsculas en `lpc17xx_clkpwr.h` y `lpc17xx_pinsel.h` (ver README).
- `libCMSISv2p00_LPC17xx.a` ya trae los drivers compilados. Si se cambia un driver, hay que recompilar la lib.
- CCLK = 100 MHz (`SystemInit`; verificar `SystemCoreClock`). `DAC_Init()` y `TIM_InitTimer` dejan su PCLK en CCLK/4 = 25 MHz.
- La configuración Debug compila con `-O1` (story 15, `.cproject`): con `-O0` la ISR con 4 voces y el ecualizador tarda ≈ 127 000 ciclos (64 % del bloque) y con `-O1`, ≈ 40 000 (20 %). Las mediciones de las stories 1 a 14 son de `-O0`.
- Estilo: identificadores en inglés y camelCase como en el driver, comentarios en español, `static` para lo
  privado de cada módulo y `volatile` para lo que comparte una ISR.
- Compilar con `-Wall` y sin warnings nuevos.
- Compilación de verificación en la PC: `distrobox enter nxp-mcuxpresso -- <ide>/tools/bin/arm-none-eabi-gcc -c -o /dev/null -std=gnu11 -mcpu=cortex-m3 -mthumb -Wall -D__USE_CMSIS=CMSISv2p00_LPC17xx -DCORE_M3 -D__LPC17XX__ -D__REDLIB__ -I firmware/inc -I <drivers>/CMSISv2p00_LPC17xx/inc -I <drivers>/CMSISv2p00_LPC17xx/Drivers/inc <archivo>.c` (no enlaza; el enlace es del IDE).
- Referencia de DAC + DMA con LLI circular: `docs/referencia/main_dac_sine_dma.c` (solo se lee, no se compila). Ojo: ahí
  `dstMemAddr = (uint32_t)LPC_DAC->DACR` lee el valor y no la dirección. Es inofensivo en M2P, pero no hay que copiarlo.

## Memoria (spec §3.10)

| Región | Contenido |
|---|---|
| Flash | `tables.c`: 4 ondas (`int16_t`, 256 muestras), incrementos, Hann y factores de giro |
| `RamLoc32` | pila, voces, filtros, FFT (~3 KB), buffer UART y quinta onda (activa + recepción) |
| AHB banco 0, `0x2007C000` | `outBuf`, buffers de audio |
| AHB banco 1, `0x20080000` | buffer de la matriz (4608 B) |

El proyecto ve los dos bancos como una sola región `RamAHB32` (alias `RAM2`). `__BSS(RAM2)`
(`<cr_section_macros.h>`) no elige banco: hay que ajustar la configuración de memoria.

Lo que lee el DMA (buffers y LLIs) va en `RAM2`. Una LLI en `RamLoc32` cargó ceros tras la primera pasada
al arrancar sin depurador (story 2); con depurador funcionaba.

## Pines (spec §3.13; viven en `config.h`)

| Función | Pin | Notas |
|---|---|---|
| Teclas Do4…Do5 | P2.0–P2.7 | pull-up interno, activas en bajo, sin interrupciones de GPIO |
| EINT1 (onda) | P2.11 | flanco descendente, pull-up |
| Attack / Decay / Sustain | AD0.0 P0.23 / AD0.1 P0.24 / AD0.2 P0.25 | potes de 10 k entre 3,3 V y GND |
| Release | AD0.4 P1.30 | ⚠️ AD0.3 no: es P0.26 = AOUT |
| DAC | AOUT P0.26 | |
| UART0 | TXD0 P0.2 / RXD0 P0.3 | USB-TTL de 3,3 V; ⚠️ ocupa AD0.7/AD0.6 |
| Datos de matrices | P0.0 | con 330 Ω en serie a DIN (§3.15); confirmar que esté libre en la placa |
| LED de la placa | P0.22 | prueba de vida (Bloque 0), alterna con SysTick |
| Medición de la ISR de audio | P2.8 | salida (Bloque 2); sube mientras la ISR del DMA genera un bloque. Fuera de P0.0–P0.7 por `FIOMASK0` |

## Audio

Cadena de un bloque, en `DMA_IRQHandler`: voces → mezcla → ecualizador → saturación → `outBuf` + captura FFT.

- **Fs:** 32 kHz para empezar. `DACCNTVAL = 781`, lo que da 32,01 kHz reales: `gen_tables.py` calcula los incrementos con ese valor.
- **DAC:** `doubleBuffer`, `dmaCounter` y `dmaRequest` en ENABLE.
- **GPDMA ch0:** M2P a `DACR`, dos LLI circulares sobre `uint32_t outBuf[2][64]`, un IRQ cada 2 ms. La mitad a llenar se sigue con un toggle por software.
- **Palabras del buffer:** `DAC_VALUE(v)`. El DMA escribe `DACR` entero, así que BIAS queda en 0 (700 µA).
- **DDS:** posición en Q16.16 con vuelta en 2^24; el índice son los bits 23:16, sin interpolación. `step = f · 2^24 / Fs`, precalculado.
- **Formatos:** muestras y envolventes en Q15. Biquad en forma directa I con coeficientes Q2.30 y acumulador de 64 bits (`SMLAL`).
- **Niveles:**
  1. `acc = Σ ((sample · env) >> 15)`, en 32 bits: cada producto se desplaza antes de sumar.
  2. `acc >>= 1` (ganancia 1/2).
  3. Ecualizador.
  4. `v = (y >> 6) + 512`, saturado a 0…1023, incrementando el contador si recorta.
- **Underruns:** al terminar el bloque, si el bit TC del canal 0 volvió a activarse, se incrementa el contador (spec §7.1).
- **Contadores para el depurador:** `audioUnderruns`, `audioSaturations` y `audioIsrMaxCycles[n]` (ciclos de CPU de la ISR con `n` voces no libres; un bloque dura 199 936; se miden con `cycle_counter`, el Timer0 a 25 MHz con resolución de 4 ciclos: SysTick daba la vuelta cada 1 ms y desbordaba la medición cuando la ISR superaba los 100 000 ciclos, story 15), `keyPressCount[8]` (pulsaciones que detecta el antirrebote, una por tecla), `waveButtonCount` (pulsaciones de EINT1 que la lógica aceptó), `activeWave` (onda activa: 0 cuadrada, 1 sierra, 2 triangular, 3 senoide, 4 Python), `envStage[4]` (etapa de la envolvente de cada voz: 0 libre, 1 Attack, 2 Decay, 3 Sustain, 4 Release) y `envLevel[4]` (su nivel Q15, 0…32767; con S = 50 % el Sustain es 16383). Se leen en Expressions con la CPU suspendida.
- **Quinta onda:** se valida en un buffer aparte y el puntero cambia entre bloques. Las voces que la usan pasan a la tabla nueva, con un posible click aceptado.
- **Ondas armónicas:** cuadrada, sierra y triangular se generan limitadas en banda (suma de senoides hasta el armónico 30, spec §3.8), arrancan en cero y su pico es 32767.

## Voces, ADSR y controles

- **Asignación** (`voice`; mira la envolvente de cada voz: libre = `ENV_IDLE`, en Release = `ENV_RELEASE`, apretada = Attack, Decay o Sustain): la voz de la tecla si sigue activa (apretada o en Release) → voz libre → voz en Release con menor nivel → apretada más antigua. `voiceStart`: una voz libre arranca en fase 0 y nivel 0; si no estaba libre, la nota sigue desde su fase y su nivel (sin click, también al reemplazar a la más antigua).
- **Acceso a las voces:** solo `audio_out` las escribe, desde SysTick y con la IRQ del DMA deshabilitada (`audioIsrLock`/`audioIsrUnlock`): la ISR puede interrumpir a SysTick. `main` las lee con `audioOutVoices()` para el asignador.
- **Onda:** se fija al iniciar la nota. La placa arranca en senoide. EINT1 rota cuadrada → sierra → triangular → seno → Python (saltea la quinta si no llegó ninguna).
- **`SysTick_Handler` cada 1 ms:**
  - Lee `ADDR0/1/2/4` con `ADC_ChannelGetData` (el burst ya corre) y los filtra (`pot_filter`: EMA con α = 1/16, arranca en la primera lectura).
  - Mapea A, D y R a 2–2000 ms, y S a 0–100 % como nivel Q15. El mapeo es lineal, con 50 cuentas de margen en cada punta.
  - Avanza las envolventes (`audioOutEnvelopeStep`), antes de leer las teclas.
  - Lee las teclas (`debounce`, 10 ms estables) y genera pulsaciones y liberaciones.
- **Envolvente** (`envelope`, lógica pura): rampas lineales, y cada etapa dura su tiempo completo desde el nivel en que arranca (soltar a mitad del Attack igual tarda R). Al soltar, Release desde el nivel actual. Al volver a tocar, Attack desde el nivel actual. Con S = 0 % y la tecla apretada la voz sigue ocupada, y queda libre recién al terminar el Release. Los parámetros salen de los potes en cada ms. Si A, D o R cambian a mitad de una etapa, se conserva lo recorrido (`progress`, fracción en Q24) y lo que falta se hace a la velocidad nueva. El Sustain sigue al pote, en Sustain y en Decay.
- **EINT1:** el flanco descendente cuenta si pasaron 150 ms desde la última pulsación aceptada y el pin estuvo 10 ms seguidos en alto desde entonces (SysTick lo lee por GPIO): una pulsación mantenida cuenta una sola vez (spec §3.11). Limpiar con `EXTI_ClearFlag` antes de salir, también al ignorar el flanco.

## NVIC (más bajo = más prioritario; spec §3.12; viven en `config.h`)

| IRQ | Prioridad |
|---|---|
| `DMA_IRQn` | 0 |
| `SysTick_IRQn` | 1 |
| `UART0_IRQn` | 2 |
| `EINT1_IRQn` | 4 |

Loop principal, sin RTOS: parser UART → FFT → codificación de matrices.

## Ecualizador

- Low-shelf a 200 Hz, peaking a 1 kHz y high-shelf a 4 kHz, de −12 a +6 dB. Arranca plano.
- Python calcula los coeficientes y comprueba que entren en Q2.30. La LPC no calcula en punto flotante.
- Los coeficientes nuevos se aplican entre bloques.

## FFT y visualizador

- FFT propia: radix-2 de 512 puntos en Q15, con escalado por etapa y tablas en Flash.
- La ISR copia muestras a un buffer de captura de 512 `int16_t`. Cuando se llena, deja de escribirlo hasta que el loop principal termina.
- Hann → FFT → magnitudes → 8 bandas de una octava (62,5 Hz a 16 kHz, bins 1, 2–3, 4–7… 128–255; spec §3.6) → alturas de 0 a 8, con suavizado. Objetivo de 20–30 FPS.
- El loop principal corre `analyzeSpectrum` después del parser: toma la captura (`audioOutCapture`), la libera apenas la FFT copió las muestras y publica el resultado. Contadores para Expressions (story 15): `spectrumPeak` (bin del pico, 1 a 255; 0 si no hay), `spectrumBand[8]` (magnitud Q15 por banda), `spectrumLevel[8]` (0 a 8), `spectrumFrames`, `spectrumFps` (promedio desde el arranque), `spectrumLastCycles` y `spectrumMaxCycles` (ciclos de CPU del análisis, con el tiempo que le quitan las ISR). La captura son las muestras que van al DAC, ya saturadas y en Q15.
- Una sola matriz de 8×8 (antes eran dos en cascada): una columna por banda.
- Color de los LED encendidos según `activeWave` (la onda seleccionada, no la de cada nota): rojo senoide, azul cuadrada, verde triangular, amarillo sierra, morado Python. Sin paleta por intensidad (spec §3.6, Bloque 13, story 18).

## Matrices WS2812B (spec §3.15 y Bloque 12; todavía sin probar en placa)

- **Timer2:** `MR0 = 9`, reinicio al coincidir, PR = 0 y PCLK de 25 MHz, para una petición cada 400 ns (`GPDMA_MAT2_0`). No usa interrupción.
- **GPDMA ch1:** de a 1 byte hacia `LPC_GPIO0->FIOPIN0`, con `FIOMASK0 = 0xFE`. Son 3 bytes por bit (alto, dato, bajo), 4608 B en total (64 LED) y 2 tramos LLI (máximo 4095 por tramo).
- ⚠️ **Driver:** en M2P, `GPDMA_SetupChannel` ignora `dstMemAddr` y usa la tabla `GPDMA_LUTPerAddr`, que para `GPDMA_MAT2_0` es `MR0`. Hay que corregirlo antes de arrancar el canal, con comentario que lo justifique:
  ```c
  GPDMA_SetupChannel(&cfg);
  /* El driver apunta M2P con MAT2.0 a MR0: el destino real es el GPIO */
  LPC_GPDMACH1->DMACCDestAddr = (uint32_t)&LPC_GPIO0->FIOPIN0;
  GPDMA_ChannelStart(GPDMA_CH_1);
  ```
- **Antes de habilitar el canal:** configurar el Timer y limpiar la petición pendiente escribiendo en su `IR` (UM10360 §21.6.12).
- **Reposo:** P0.0 como salida en bajo. El buffer se codifica solo cuando terminó la transmisión.
- **Brillo:** limitado al 25 % (peor caso ≈ 1 A). Límites tras 5 min con los 64 LED en blanco: corriente ≤ 1,1 A, tensión en la matriz ≥ 4,0 V, y dorso y cables tocables 10 s sin quemar (spec §3.6). Ajustar y medir la fuente antes de conectar.
- **Línea de datos:** GPIO directo a DIN con 330 Ω en serie, sin adaptador de nivel. La matriz se alimenta a 4,5 V (el mínimo con umbrales garantizados: VIH = 0,7 × VDD = 3,15 V); si DIN no registra, bajar hasta 4,0 V y anotarlo como fuera de la hoja de datos (spec §3.15). Encender la matriz antes de transmitir: apagada, DIN solo tolera 0,5 V. Orden de datos GRB, bit más significativo primero.

## UART (spec §3.14)

- UART0 por driver (`UART_Init`, `UART_FIFOConfig`, `UART_IntConfig`, `UART_Send`), 115200 8N1. RX por interrupción hacia un buffer circular de 256 B, y parser en el loop principal.
- Trama: `0xA5 | CMD | LEN_L | LEN_H | payload | CHK`, con CHK = suma de 8 bits de CMD a payload, en complemento a dos.

| CMD | Mensaje | Payload |
|---|---|---|
| `0x01` | UPLOAD_WAVE | 256 `int16_t` (512 B) |
| `0x04` | SET_EQ | 15 coeficientes Q2.30 (60 B) |
| `0x06` | ACK | vacío (LEN = 0) |

Una trama inválida se descarta sin responder y no modifica nada activo. No hay NACK ni PING (spec, Bloque 9).

## Aplicación (`app/`)

- Python con venv propio: PySide6, numpy, pyqtgraph y pyserial; pytest para los tests. Verificar que haya wheel de PySide6 para Python 3.14 y, si no, usar 3.13. Funciona igual en Fedora y en CachyOS.
- **Nombres:** identificadores en inglés en el código nuevo (constitución, principio 8). La UI de la story 11 usa nombres en español y se deja así hasta que se pida renombrarla.
- **Editor:** el usuario solo edita amplitudes. Dentro de la octava siempre se cargan 30 armónicos (límite de Do5 a 32 kHz); elegir la frecuencia cambia los Hz que se muestran.
- **Instrumentos:** guitarra, piano, órgano y flauta, con su mensaje de ajuste de ADSR. Las amplitudes se definen en el Bloque 8.
- **Ecualizador:** perillas de bajos, medios y altos; solo configura.
- **Fuera de alcance:** analizar sonidos existentes.

## Etapa analógica (spec §3.7; validada en el Bloque 3, story 4)

AOUT → Sallen-Key con LM358 (fc ≈ 13 kHz) → 1 µF → 150 kΩ y pote de 10 kΩ log → LM386 → 220 µF → 100 Ω por canal → jack.
Se alimenta con 5 V de una línea distinta de la de las matrices.

## Gotchas

- `DMA_IRQHandler` es uno solo para todos los canales: revisar `DMACIntTCStat` bit por bit y limpiar cada uno.
- Hay que limpiar los flags de interrupción (EXTI, DMA, Timer) antes de salir de la ISR, o se vuelve a disparar.
- `FIOMASK0` afecta a toda escritura en ese byte del puerto 0, no solo a las del DMA.
- Fuera de alcance, no reintroducir: grabación de voz, EINT2 y SPI.
