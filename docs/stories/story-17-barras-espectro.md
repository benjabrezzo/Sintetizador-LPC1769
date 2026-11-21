# Story 17: Barras del espectro en la matriz

**Bloque:** 13 · **Spec:** §3.6, §6 Bloque 13 (criterios 1 a 5) · **Estado:** en curso (el código y los tests de la PC se hacen antes de la visita al laboratorio; los criterios de placa quedan sin marcar hasta esa visita, según la excepción de la Regla de avance)

## Story
Como músico, quiero ver en la matriz 8 barras con el espectro de lo que toco, para notar los cambios de timbre y ecualización.

## Criterios de aceptación
- [ ] Dada la matriz funcionando (story 16) y una senoide sonando, cuando miramos las barras, entonces cada banda tiene su columna (de graves a agudos, de izquierda a derecha), la barra de la banda de esa frecuencia es la más alta y todas crecen desde abajo.
- [ ] Dada una nota sostenida, cuando cambiamos su amplitud (con el pote de Sustain), entonces la altura de cada barra coincide con `spectrumLevel` y sube o baja un nivel por cada 6 dB.
- [ ] Dadas notas con distintas ondas (senoide, cuadrada, sierra) y con el ecualizador, cuando cambiamos de onda o movemos las perillas, entonces las barras cambian y siguen coincidiendo con `spectrumLevel`.
- [ ] Dado el visualizador activo, cuando tocamos y soltamos notas, entonces las barras se actualizan a 20–30 FPS (`matrixFrames`) y bajan de a un nivel por cuadro de pantalla, sin saltos ni parpadeo visibles.
- [ ] Dadas cuatro voces sonando con el ecualizador, el análisis y las barras corriendo, cuando las dejamos 5 minutos, entonces `audioUnderruns` queda en 0 y `spectrumFps` se mantiene en 20 o más.

## Plan
- [x] Spec primero: la §3.6 ya registra la etapa de caída en pantalla (la barra mostrada es el mayor entre el nivel nuevo y la mostrada menos `BARS_FALL`, un nivel por cuadro de pantalla) y que en esta story el color es fijo (rojo).
- [x] Lógica pura con tests en la PC (módulo `led_bars`, `firmware/tests/test_led_bars.c`, 9 tests): `ledBarsDraw` dibuja los 8 niveles como barras, con una columna por banda, creciendo desde abajo y con el color como parámetro; `ledBarsFall` es la etapa de caída de pantalla. Una prueba encadenada: una senoide de 1 kHz pasa por `spectrumMagnitudes`, las bandas, los niveles y las barras, y se mira solo la columna de la banda 5 (la quinta, de índice 4).
- [x] Integración, compilada con `-O1 -Wall` sin warnings: un patrón nuevo, `MATRIX_PATTERN_SPECTRUM` (7), que `updateMatrix` dibuja con los niveles del análisis. Color rojo fijo. La placa sigue arrancando en 0, sin transmitir. Sin test en la PC: se prueba en el laboratorio.
- [ ] Revisar que el IDE compile `led_bars.c` (archivo nuevo en `firmware/src`: refrescar el proyecto) y que la compilación de verificación coincida con la del IDE.
- En el laboratorio, después de validar la story 16: probar los criterios 1 a 5 con la matriz alimentada como en la story 16, y evidencia.

## Fuera de alcance
- El color por onda seleccionada (story 18) y la prueba prolongada de todo el sistema (story 19).
- Cambiar las bandas, la escala o el suavizado del análisis (Bloque 11, ya cerrado).
- Brillo distinto del 25 %, una segunda matriz, cualquier mensaje por UART y medir en Release.
- Arrancar la matriz sola: se enciende eligiendo el patrón en Expressions.

## Notas
- Decisiones confirmadas el 2026-11-14:
  - Etapa de caída en pantalla con `BARS_FALL` = 1 nivel por cuadro de 40 ms (320 ms para bajar 8 niveles), como constante ajustable en el laboratorio.
  - Color rojo fijo, con `ledBarsDraw` recibiendo el color por parámetro: la story 18 solo tiene que pasar otro.
  - Prueba encadenada en la PC con una senoide de 1 kHz (banda 5, bins 16 a 31), sin asumir nada de las columnas vecinas por la fuga de la ventana Hann.
- Decisiones al implementar (2026-11-14):
  - `BARS_FALL` vive en `config.h` y `ledBarsFall` implementa la fórmula completa (`max(nivel, mostrada − BARS_FALL)`, con piso en 0), para que siga siendo correcta si en el laboratorio se sube a 2 o más. Los tests de caída (6 y 7) suponen `BARS_FALL` = 1: si se cambia, hay que revisarlos.
  - En `updateMatrix` la caída se calcula sobre una copia y `barsShown` se actualiza solo cuando `ledMatrixSend` acepta el cuadro: si el envío se reintenta varias vueltas del loop, la barra igual baja un nivel por cuadro mostrado y no por intento.
  - La banda 5 es la columna de índice 4 (la columna 0 es la banda 1): el "columna 5" de la prueba encadenada es en base 1.
- Datos de la story 15: `spectrumFps` = 55 y el análisis dura ≈ 281 000 ciclos (≈ 2,8 ms). La matriz se actualiza a 25 FPS: sin la etapa de caída en pantalla, cada cuadro de 40 ms mostraría bajar la barra unos 2 niveles (la del análisis baja uno por cuadro, cada ≈ 18 ms).
- La fila 0 es la de arriba: la barra de altura h enciende de la fila 7 hacia arriba. La columna 0 es la banda 1 (62,5–125 Hz); si la orientación de la matriz resulta distinta en la story 16, se corrige el mapeo y no esta lógica.
- Las fundamentales de Do4 a Si4 caen en la banda 3 y Do5, en la 4: las teclas no se separan por columna y lo que se ve es el timbre (spec §3.6).
- Variables para Expressions, con la CPU suspendida: `matrixPattern` (poner 7), `spectrumLevel` y `spectrumFps` (sin `[8]`), `matrixFrames`, `matrixSendMaxCycles` y `audioUnderruns` (una sola suspensión al final).
- Hardware para la visita: el de la story 16, más la etapa de audio con su cargador aparte (opcional, solo para oír) y las teclas para tocar las notas.
- Depende de la story 16: se valida después de ella en la misma visita. Si algo de la 16 falla, se corrige antes.
