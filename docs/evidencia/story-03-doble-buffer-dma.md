# Story 3 · Bloque 2 · Audio continuo con doble buffer por DMA

**Estado:** COMPLETA. Respalda los tres criterios de la story 3 y los cinco del Bloque 2 (ver "Conclusión").

## Procedimiento
- Firmware: `audio_out.c` (DAC + GPDMA canal 0 en M2P, dos LLI circulares con bit I, `outBuf[2][64]` y
  `dacLli[2]` en `RAM2`, y `DMA_IRQHandler` con prioridad 0, que llena la mitad que terminó) y
  `block_cursor.c` (lógica: qué mitad y desde qué índice). Build Debug (`-O0`): 0 errores y 0 advertencias;
  `RamAHB32` 544 B (512 B de buffers y 32 B de LLI).
- Placa: LPCXpresso LPC1769 por USB. CH1 en AOUT (P0.26), CH2 en P2.8, masa en GND. Los dos canales en
  acople AC: la senoide aparece centrada en 0 V (Vavg de 7 a 28 mV). Es una deducción de esos datos.
- Las horas salen del EXIF, que se quitó de las copias (tenían GPS). Son horas de la cámara, no del
  arranque de la corrida.

| Archivo | Hora | Qué es |
|---|---|---|
| `story-03-pulsos-vs-senoide.jpg` | 19:28:22 | Vista general, 1 ms/div, trigger en CH1 |
| `story-03-pulso-p28.jpg` | 19:41:43 | Ancho del pulso con cursores, 100 µs/div |
| `story-03-cambio-bloque.jpg` | 19:48:39 | Cambio de bloque, 250 µs/div, trigger en P2.8 |
| `story-03-frecuencia-previa.jpg` | 19:50:57 | Frecuencia previa a la corrida |
| `story-03-0min.jpg` | 20:25:46 | Medidas al inicio de la corrida de 5 min |
| `story-03-pulso-5min.jpg` | 20:30:51 | Pulso a los 5 min (sin cursores) |
| `story-03-5min.jpg` | 20:31:14 | Medidas a los 5 min |
| `story-03-expressions.jpg` | sin hora | `audioUnderruns` en la vista Expressions |
| `story-03-sin-depurador.jpg` | 20:40:08 | 1 min sin depurador |

## Resultado 1: 5 min continuos (criterio 1)
Esperado: 125,04 Hz (período 7,997 ms), banda de 123,79 a 126,29 Hz.

| Instante | Senoide | P2.8 |
|---|---|---|
| Inicio (20:25) | 124,9 Hz, 8,008 ms (−0,11 %) | 500,0 Hz, 2,000 ms |
| 5 min 28 s (20:31) | 124,9 Hz, 8,008 ms (−0,11 %) | 499,0 Hz, 2,004 ms |

`story-03-expressions.jpg`: `audioUnderruns` = 0, leído con Suspend después de la foto de 5 min 28 s. No hay
lectura en vivo: Live Expressions no aparece en este IDE y Real Time Expressions (PEmicro) no actualizó.
La corrida arrancó limpia (sesión relanzada, sin breakpoints) y la vimos sin cortes ni reinicios; las fotos
por sí solas no prueban continuidad.

## Resultado 2: pulso y cambios de bloque (criterio 2)
- Ancho: `story-03-pulso-p28.jpg`, cursores sobre los dos flancos: X1 = −172,0 µs, X2 = −116,0 µs,
  **Δ = 56,00 µs**, el 11 % del límite de 500 µs y el 2,8 % del bloque. `story-03-pulso-5min.jpg` da ≈ 55 µs
  contando divisiones: es una estimación, no una medición.
- Cambio de bloque: `story-03-cambio-bloque.jpg` (cruce ascendente por cero) y `story-03-pulso-p28.jpg`
  (descendente) muestran la escalera continua, sin salto de fase ni escalón fuera de patrón. Son los cruces
  donde un salto de una muestra (≈ 40 mV) se notaría más; en los picos no se vería.
- Ritmo: un pulso cada 2,000 ms al inicio y 2,004 ms a los 5 min; 4 bloques por período, un pulso cada π/2
  (`story-03-pulsos-vs-senoide.jpg`).
- Misma forma que la story 2: 124,8 a 125,2 Hz (allá 124,8 a 125,3) y Vpp de 3,36 a 3,40 V (allá 3,36 a 3,50).

## Resultado 3: tests de la PC (criterio 3)
`make -C firmware/tests test`: salida 0 con los 3 tests de `block_cursor` (continuidad, alternancia, flujo
sin saltos), `dac_level` y el de ejemplo. Con 3 versiones rotas del módulo (en un directorio temporal, sin
tocar el repo) el test sale con 1; en los pasos rojos del desarrollo `make` salió con 2.

## El contador cuenta
Breakpoint dentro de la ISR (en `fillNextBlock`), 5 s detenido, Resume y Suspend: `audioUnderruns` pasó de
0 a **2**. Esperábamos 1; el segundo incremento no se explicó. Después se relanzó la sesión.

## 1 min sin depurador
`story-03-sin-depurador.jpg`: 124,8 Hz, 8,015 ms (−0,19 %) y P2.8 a 500,0 Hz, tras desenchufar y volver a
enchufar el USB y sin abrir sesión de depuración. Que la foto es de 1 min después del arranque lo observamos
nosotros; no se ve en la imagen. El contador no se puede leer sin depurador.

## Observaciones sin resolver
- **Contador 2 en vez de 1.** Hipótesis sin probar: un fin de transferencia justo dentro de la ISR de
  reingreso (≈ 3 %), o el depurador frenando la CPU otra vez dentro de una ISR.
- **SysTick y DMA con la misma prioridad (0).** `SYSTICK_InternalInit` no la fija y la spec §3.12 pide 1 para
  SysTick. Decidimos dejarlo así; no se observó ningún efecto.
- **Copias desfasadas de la senoide con el trigger en CH2.** Explicación probable, no comprobada: hay 4 pulsos
  idénticos por período y el osciloscopio engancha en cualquiera. Con el trigger en CH1 se ve una sola traza.
- **Picos finos en la traza.** Ya estaban en la story 2, sin esta ISR; siguen sin explicar.
- **Contador de trigger del osciloscopio** (1,27 kHz, 132 Hz, 36 Hz, <20 Hz): se ignora, como en la story 2.
- **Límites de la detección de underruns** (análisis, no probado): una CPU frenada antes de entrar a la ISR
  fusiona los fines de transferencia y no se cuenta; y tras un underrun real la alternancia de mitades puede
  quedar desfasada respecto del DMA hasta el próximo reinicio.
- **"No se sobrescribe un buffer en uso" tiene evidencia indirecta:** test de alternancia, senoide sin
  discontinuidades y contador en 0. Leer `DMACCSrcAddr` con un breakpoint no sirve, porque el DMA sigue
  corriendo con la CPU frenada.

## Decisiones (06-11-2026)
- Leemos el contador con Suspend y la vista Expressions, porque no hay lectura en vivo.
- SysTick queda en prioridad 0.
- El criterio "no se sobrescribe un buffer en uso" se marca con evidencia indirecta.

## Conclusión
- **Criterio 1:** cumple (124,9 Hz a los 0 y 5 min; contador en 0).
- **Criterio 2:** cumple (pulso de 56 µs contra 500 µs; sin saltos en los cruces por cero).
- **Criterio 3:** cumple.
- **Bloque 2:** cinco criterios; el de "no se sobrescribe un buffer en uso" con evidencia indirecta.
