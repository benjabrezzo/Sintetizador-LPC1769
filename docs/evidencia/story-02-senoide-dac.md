# Story 2 · Bloque 1 · Senoide a la salida del DAC (AOUT, P0.26)

**Estado:** COMPLETA. Respalda los tres criterios de la story 2 y los cuatro del Bloque 1 (ver
"Conclusión"). Queda un pendiente que no bloquea: el link del video.

## Procedimiento
- Firmware: `firmware/src/audio_out.c` (DAC + GPDMA canal 0 en M2P, una LLI circular, `outBuf` y `dacLli`
  en `RAM2`), tabla de `firmware/src/tables.c` generada por `firmware/tools/gen_tables.py`.
- Placa: LPCXpresso LPC1769 alimentada por USB. Según Benja, todas las capturas son del firmware limpio
  (sin el diagnóstico) y con arranque en frío, sin depurador.
- Instrumento: osciloscopio, CH1 en AOUT (P0.26), masa en GND de la placa. La atenuación de la punta se
  corrigió durante la prueba: la primera lectura daba Vmax = 33,2 V porque el canal estaba en ×10 con la
  punta en ×1. Con el factor igualado, Vmax = 3,32 V.
- En las capturas del 06-11-2026 con senoide el canal está en acople **AC**: la senoide aparece centrada en
  0 V y su Vavg es de 12 a 41 mV, no 1,65 V. Es una deducción de esos datos, no una lectura de un
  indicador: el símbolo `~` junto a la escala no cambió en la foto con DC, así que no se usa para
  decidirlo. La línea del disparo (`EDGE`) sí cambió de `AC` a `DC` entre las fotos. El nivel de DC sale
  de la foto del 05-11 y del multímetro (ver más abajo).
- Se quitaron los metadatos de todas las fotos (tenían ubicación GPS). Las horas de captura, leídas del
  EXIF antes de quitarlo, son las de las tablas.

## Resultado 1: frecuencia (criterio 1)

`story-02-frecuencia.jpg` (05-11, 20:00; 2 ms/div, CH1 a 500 mV/div, acople DC):

| Dato | Medido | Esperado |
|---|---|---|
| Período (Prd) | 8,00 ms | 7,997 ms (banda de 7,918 a 8,078 ms) |
| Frecuencia (Freq) | 125 Hz | 125,04 Hz (banda de 123,79 a 126,29 Hz) |
| Vmax | 3,32 V | ≈ 3,3 V |
| Vavg | 1,65 V | ≈ 1,66 V (promedio de una senoide centrada) |

Con el multímetro en DC sobre AOUT, la senoide cargada desde el IDE dio 1,62 V.

## Resultado 2: Fs a lo largo de la prueba (criterio 2)

Fs derivada del período: la tabla tiene 256 muestras, así que Fs = 256 / T. Valor esperado:
25 MHz / 781 = 32 010 Hz (período de 7,997 ms). Capturas `story-02-medidas-*.jpg`
(AC, CH1 a 1 V/div y 1 ms/div la primera, 500 mV/div y 2,5 ms/div las otras dos):

| Instante | Hora | Período | Frecuencia | Fs = 256 / T | Desvío de Fs |
|---|---|---|---|---|---|
| Inicio | 17:43:41 | 8,016 ms | 124,8 Hz | 31 936 Hz | −0,23 % |
| ≈ 5 min | 17:47:57 | 8,002 ms | 125,0 Hz | 31 992 Hz | −0,06 % |
| ≈ 10 min | 17:54:09 | 7,997 ms | 125,0 Hz | 32 012 Hz | +0,01 % |

Todos dentro de ±1 % (Fs de 31 690 a 32 330 Hz; frecuencia de 123,79 a 126,29 Hz). Otras lecturas de
apoyo, todas dentro de la banda: `story-02-valle-a-valle.jpg` 124,8 Hz / 8,010 ms (17:09) y
`story-02-pico-25us.jpg` 125,3 Hz / 7,980 ms (17:20). Entre todas las fotos el período varía de 7,980 a
8,016 ms (±0,23 % respecto de 7,997 ms): esa dispersión, y no la resolución de 1 µs de la lectura, es la
incertidumbre práctica de esta medición.

## Resultado 3: ancho del escalón con cursores (criterio 2, método del enunciado)

`story-02-escalon-*.jpg`: 25 µs/div, 50 mV/div, acople AC, cursores X1 y X2 sobre dos saltos
consecutivos (un escalón). Esperado: 31,24 µs (banda de 30,93 a 31,55 µs).

| Instante | Hora | X1 | X2 | Δ | f = 1/Δ |
|---|---|---|---|---|---|
| Inicio | 17:41:58 | −3,000 µs | 29,00 µs | 32,00 µs | 31,25 kHz |
| ≈ 5 min | 17:47:22 | −7,000 µs | 25,00 µs | 32,00 µs | 31,25 kHz |
| ≈ 10 min | 17:53:35 | 1,000 µs | 32,00 µs | 31,00 µs | 32,26 kHz |

**Límite de esta medición:** los cursores avanzan de 1 µs en 1 µs y se midió un solo escalón. Una
resolución de 1 µs sobre 31,24 µs es ±3 %, mayor que la tolerancia de ±1 %. Los valores 32, 32 y 31 µs
son compatibles con 31,24 µs, pero **no demuestran por sí solos ±1 %**; dos de ellos (32,00 µs, +2,4 %)
caen numéricamente fuera de la banda. Lo que sí muestran las fotos es la escalera: escalones de alto
cercano a 40 mV (unos 0,8 div a 50 mV/div) y ancho de unas 1,3 divisiones (25 µs/div), igual en los tres
instantes. La medición cuantitativa de Fs es la del Resultado 2.

## Observaciones sin resolver
- **Picos finos de ruido.** Con la punta solo a GND (`story-02-solo-gnd.jpg`, 500 mV/div, 5 ms/div) el
  osciloscopio ya marca Vmax 120 mV y Vpp 260 mV. En `story-02-valle-a-valle.jpg` se ven picos finos en
  crestas y valles, y por eso el Vpp marca 3,50 V (y 3,40, 3,36 y 3,38 V en las otras): la senoide sola
  mide ≈ 3,3 V. En las fotos del escalón hay picos verticales de varias divisiones, no solo en los saltos.
  Es del mismo orden que el piso de ruido con la punta a GND, y también del orden del Vmin = −180 mV de la
  foto del 05-11. No se comprobó si parte de esos picos son glitches del DAC o solo ruido de la medición
  (punta y masa).
- **Cero en DC, verificación limitada.** `story-02-solo-gnd.jpg` (17:23, canal en AC) no dice el cero en
  DC: su Vavg (1,37 mV) sale del acople. `story-02-solo-gnd-dc.jpg` (18:01, punta a GND, 500 mV/div,
  2,5 ms/div) marca Vmax 60 mV, Vpp 120 mV y Vavg 6,02 mV. En esa foto cambió el acople del disparo
  (AC a DC) pero no el indicador del canal, así que no se puede confirmar desde la imagen que el canal
  estuviera en DC. Si lo estaba, el cero tiene un error medio de unos 6 mV (0,2 % de 3,3 V), despreciable,
  y el piso de ruido baja a ±60 mV; si no, la foto no dice nada sobre el cero. El nivel en DC se apoya en
  Vmax 3,32 V y Vavg 1,65 V (05-11) y en 1,62 V del multímetro. Los valles de la foto del 05-11 quedaron
  tapados por el cuadro de mediciones.
- **El contador de disparo del osciloscopio** (línea inferior de cada foto) marca entre 0,32 y 29 kHz según
  la captura y nunca 125 Hz. Es coherente con que dispara sobre los picos de ruido. En `story-02-medidas-5min.jpg` se ve la senoide
  superpuesta con otra desfasada; no está explicado, y se atribuye (sin comprobarlo) al disparo sobre
  esos picos y no a un corte de la señal.
- Las cifras Vovr, Vpre, Rise, Fall, +Wid, −Wid y los duty del cuadro de mediciones de la foto del 05-11 no
  se usan: no tienen sentido para una senoide.
- En `story-02-pico-25us.jpg` (pico a 25 µs/div) no se ven escalones: en el pico la senoide cambia 0 o 1
  código entre muestras (unos 3 mV), invisible a 500 mV/div. Los escalones se miden en el cruce por cero.

## Hallazgo durante la prueba: arranque en frío
Con `dacLli` en `RamLoc32` la senoide solo aparecía con la sesión del IDE; sin depurador (encendido o
reset) salía un primer ciclo y la salida quedaba plana. Se leyeron registros y RAM con un diagnóstico
temporal (ya borrado) y LinkServer en modo `--attach`:

| Dato (arranque en frío) | `dacLli` en `RamLoc32` (`0x10000028`) | `dacLli` en `RAM2` (`0x2007c400`) |
|---|---|---|
| `DMACC0LLI`, `SrcAddr`, `Control` tras la 1.ª pasada | 0, 0, 0 | `0x2007c400`, avanzando por `outBuf`, cambiando |
| `DMACC0Config` | `0x9c1` (inactivo) | `0x209c1` (activo) |
| `DACR` | fijo en `0x7cc0` (código 486, 1,58 V con multímetro) | cambia entre lecturas |
| `DMACRawIntErrStat` | 0 | 0 |

La LLI en RAM contenía los valores correctos en ambos casos. Qué hace que con la LLI en `RamLoc32` el
canal cargue ceros no está demostrado (hipótesis sin probar: el `__WFI()` del `main`); lo observado es que
con la LLI en `RAM2` circula con y sin depurador. Está registrado en `CLAUDE.md` (Memoria) y en el
comentario de `audio_out.c`.

Lecturas de apoyo: `SystemCoreClock` = 100 000 000; `DACCNTVAL` = 781 (`0x30D`); `DACCTRL` = `0xE`;
`PINSEL1` = `0x00200000` (AOUT); en el primer milisegundo el DMA llevaba unas 32 muestras (ritmo de
32 µs por muestra).

## Video
Provisto por Benja: senoide recién cargada desde el IDE y de nuevo tras desconectar y volver a conectar
la placa.
https://drive.google.com/file/d/1ElL0EKefkC_K7sBo3otfwyO6D7gsVh_E/view?usp=drive_link
No lo verificó Claude (no puede ver el video). Falta confirmar que el enlace esté abierto para quien
evalúe.

## Decisiones (06-11-2026)
- **Método de Fs (opción B, elegida por Benja).** Los cursores no alcanzan la resolución de ±1 % sobre un
  solo escalón (Resultado 3). La Fs se da por medida con el período (Resultado 2) y los escalones quedan
  como evidencia cualitativa. El texto del criterio en la story se ajustó en consecuencia.
- **10 minutos sin cortes.** Benja confirmó que no hubo cortes ni reinicios en el intervalo. Es una
  observación directa suya; las capturas muestran la señal presente a las 17:43, 17:47 y 17:54, pero por
  sí solas no prueban continuidad entre una y otra.

## Pendiente (no bloquea)
1. Comprobar que el link del video sea público para la cátedra.
2. Opcional: repetir la foto con la punta a GND confirmando en el menú del canal que el acople sea DC.

## Conclusión
- **Criterio 1 (frecuencia):** cumple. 8,00 ms y 125 Hz en la foto del 05-11; 124,8 a 125,3 Hz en las demás.
- **Criterio 2 (Fs estable):** cumple. La Fs derivada del período, 31 936, 31 992 y 32 012 Hz en el inicio,
  a los ≈ 5 min y a los ≈ 10 min, está dentro de 32 010 Hz ± 1 % y no muestra deriva (≤ 0,23 %).
- **Criterio 2 (10 min sin cortes):** cumple, según la confirmación de Benja.
- **Limitación:** la medición directa del ancho de escalón con un solo escalón y cursores de 1 µs no tiene
  resolución para ±1 % (Resultado 3). No se usa como medida cuantitativa.
