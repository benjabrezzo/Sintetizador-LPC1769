# Story 16: Matriz de 8×8 con patrones y colores

**Bloque:** 12 · **Spec:** §3.6, §3.15, §6 Bloque 12 · **Estado:** en curso (el código y los tests de la PC se hacen antes de la visita al laboratorio; los criterios de placa quedan sin marcar hasta esa visita, según la excepción de la Regla de avance)

## Story
Como evaluador, quiero ver patrones y colores correctos en la matriz de 8×8 mientras suena el audio, para comprobar que funciona sin afectarlo y con un consumo seguro.

## Criterios de aceptación
- [ ] Dada la matriz alimentada a 4,5 V (bajando hasta 4,0 V si hace falta) y con DIN conectado a P0.0 por una resistencia de 330 Ω, cuando la placa recorre un píxel por vez, las filas, las columnas y los cinco colores de las ondas (rojo, azul, verde, amarillo y morado), entonces cada patrón aparece en la posición y con el color esperados, con el píxel (0,0) y la orientación comprobados a ojo.
- [ ] Dada la matriz transmitiendo, cuando medimos DIN con el osciloscopio, entonces los niveles cumplen la hoja de datos del WS2812B para la tensión usada (alto ≥ 0,7 × VDD), los pulsos "0" y "1" quedan dentro de la tolerancia (T0H 0,4 µs, T1H 0,8 µs, T0L 0,85 µs y T1L 0,45 µs, ±150 ns) y DIN queda en bajo entre cuadros.
- [ ] Dadas cuatro voces sonando con el ecualizador y el análisis corriendo, y un patrón de prueba que se actualiza a 20–30 FPS, cuando dejamos 5 minutos los 64 LED en blanco con el brillo al 25 %, entonces `audioUnderruns` queda en 0, la corriente es ≤ 1,1 A, la tensión en la matriz se mantiene ≥ 4,0 V y el dorso de la matriz y los cables de alimentación se pueden tocar 10 s sin quemar.

## Plan
- Spec primero (hecho el 2026-11-14): nivel de DIN por reducción de VDD con GPIO directo y 330 Ω (§3.6, §3.15 y tabla de la §1), límites de consumo y criterios del Bloque 12. Pendiente: el mapeo físico y la elección del patrón de prueba, una vez comprobada la orientación en la placa.
- Lógica pura con tests en la PC: superficie 8×8 con mapeo físico, tope de brillo al 25 %, orden de color GRB y codificación del buffer de 4608 B (alto, dato y bajo por bit).
- Capa de hardware, compilada y revisada antes de la visita: P0.0 en bajo en reposo, Timer2 pidiendo cada 400 ns, DMA al puerto con la máscara, y envío iniciado y consultado desde el loop principal, sin esperas. El patrón se elige con una variable en Expressions.
- Con el multímetro, antes de la visita: comprobar que P0.0 queda en 0 V en reposo y que sube mientras transmite. Dejar armados los 330 Ω y los pines soldados a la matriz.
- En el laboratorio: ajustar la fuente y su límite de corriente antes de conectar, probar patrones y colores, medir DIN con el osciloscopio y hacer la prueba de consumo, audio y temperatura. Con todo eso, evidencia y cierre del bloque.

## Lista de armado y de la visita
La variable `matrixPattern` se cambia en la vista Expressions con la CPU suspendida. Arranca en 0, que **no transmite**: con la matriz sin alimentar, DIN solo tolera 0,5 V. Patrones: 1 píxel, 2 fila, 3 columna, 4 colores (rojo, azul, verde, amarillo, morado), 5 blanco, 6 todo apagado transmitiendo.

**Armado (antes de la visita)**
- Placa con el depurador USB, matriz CJMCU-8*8 con los pines soldados en GND, +5V y DIN (lado de entrada), resistencia de 330 Ω (y un par de 150 y 470 Ω de reserva), cables, multímetro y, en el laboratorio, fuente regulable con límite de corriente y osciloscopio.
- Los 330 Ω van entre P0.0 y DIN, lo más cerca posible de la matriz. La masa es común entre fuente, placa y matriz.
- DOUT no se conecta (§3.15) y queda aislado, sin tocar GND ni +5V. Los pads +5V y GND del lado de salida tampoco se usan: la alimentación va solo en el lado de entrada.
- Cables de potencia cortos y gruesos: el peor caso es ≈ 1 A y la tensión que cuenta es la de la matriz.
- La etapa de audio es opcional (solo para oír) y va con su propio cargador, aparte de la matriz.

**Con el multímetro, sin la matriz conectada**
1. Comprobar que P0.0 esté libre y accesible en la placa.
2. Con `matrixPattern = 0`, P0.0 contra GND debe dar 0 V.
3. Con `matrixPattern = 5` debe aparecer una tensión continua pequeña (estimada en ≈ 0,1 V por el ciclo de trabajo: 1,84 ms de transmisión cada 40 ms; es un cálculo, no una medición). Solo confirma que el pin sube al transmitir.

**En el laboratorio, en orden**
1. Ajustar la fuente a 4,5 V y su límite a ≈ 1,5 A, y comprobar la tensión con el multímetro, antes de conectar la matriz.
2. Unir las masas, conectar +5V y GND de la matriz y encender la fuente, con la placa en `matrixPattern = 0`.
3. Recién entonces conectar P0.0 a DIN por los 330 Ω.
4. Patrones 6, 1, 2, 3 y 4: orientación, píxel (0,0) y colores. Si el mapeo (serpentina por columnas) no coincide, se corrige `ledSurfaceLedIndex` y la tabla de su test. Con el patrón 4, ajustar a ojo el amarillo y el morado para que se distingan del verde y del rojo (story 18; partida: (255, 160, 0) y (170, 0, 255)) y anotar los valores finales.
5. Osciloscopio en DIN, con el patrón 5 (el 63 = `00111111` lleva pulsos "0" y "1"): niveles (alto ≥ 0,7 × VDD), T0H, T1H, T0L y T1L (±150 ns) y DIN en bajo entre cuadros. Si DIN no registra, bajar la fuente de a pasos hasta 4,0 V y anotarlo como fuera de la hoja de datos.
6. Prueba de 5 minutos con el patrón 5, cuatro voces, ecualizador y análisis corriendo: corriente (≤ 1,1 A), tensión en la matriz (≥ 4,0 V) y temperatura al tacto 10 s (dorso y cables).
7. Con una sola suspensión al final, leer `audioUnderruns` (debe ser 0), `matrixFrames` (≈ 25 por segundo, unos 7500 en 5 minutos), `matrixSendMaxCycles`, `spectrumFps` y `audioIsrMaxCycles`. Un valor imposible se descarta y se repite la sesión.

## Fuera de alcance
- Mostrar el espectro en las barras (story 17) y el color por onda seleccionada (story 18).
- La prueba prolongada de todo el sistema (story 19).
- Cualquier etapa adaptadora de nivel, brillo distinto del 25 %, una segunda matriz y cualquier mensaje por UART.
- Medir en Release.

## Notas
- Decisiones confirmadas:
  - Matriz a 4,5 V con GPIO directo y 330 Ω, sin transistor: no tenemos BSS138, y un NPN deforma los pulsos de 400 ns (§3.15).
  - Fuente regulable con límite de corriente en ≈ 1,5 A, para que no oculte la medición del límite de 1,1 A.
  - Temperatura al tacto, sin LM35.
  - Se mantiene la excepción de la Regla de avance: las stories 16, 17 y 18 se validan en una sola visita al laboratorio, en ese orden.
- Hoja de datos del WS2812B (`~/Descargas/WS2812B.PDF`), confirmado:
  - Umbrales VIH = 0,7 × VDD y VIL = 0,3 × VDD, garantizados con VDD de 4,5 a 5,5 V. El rango de alimentación de la tabla de valores máximos es de +3,5 a +5,3 V.
  - Tiempos de bit y reinicio mayor a 50 µs; con 3 × 400 ns por bit, los cuatro tiempos caen dentro de la tolerancia.
  - Orden de datos GRB, bit más significativo primero.
- Entre 4,0 y 4,5 V los umbrales no están garantizados: si hace falta bajar de 4,5 V, la evidencia lo anota. Con VDD bajo, el verde y el azul pueden salir más tenues y correr el tono del amarillo y el morado.
- Ajuste de colores (2026-11-15, story 18): la tabla de los cinco colores del patrón 4 es la misma que usan las barras del espectro, así que lo que se ajuste acá vale para las dos. A igual valor de PWM el verde se ve mucho más brillante que el rojo y el azul es el más tenue: con valores completos, el amarillo tiende a verde lima y el morado a fucsia.
- Matriz CJMCU-8*8: pads de entrada GND, +5V y DIN (abajo a la derecha en la foto del dorso) y de salida DOUT, +5V y GND. Las pistas parecen una serpentina por columnas; se confirma en la prueba de orientación.
- Con la matriz apagada, DIN solo tolera 0,5 V: encender la matriz antes de transmitir.
- Corrección de `CLAUDE.md`: el peor caso con el brillo al 25 % es ≈ 1 A (decía 1,9 A).
- Hardware para la visita: placa con el depurador, matriz con los pines soldados en GND, +5V y DIN, resistencia de 330 Ω (más un par de 150 y 470 Ω por si DIN no cumple), cables, multímetro, fuente regulable y osciloscopio (en el laboratorio). La etapa de audio es opcional (solo para oír): su cargador va aparte y no se usa para la matriz.
- La foto del dorso (`20261114_142119.jpg`) trae GPS en el EXIF: limpiarla con `jpegtran` antes de commitearla.

## Sugerencias (tu decisión)
- Si el criterio 2 se traba, separarlo en una story chica de "DIN medido" antes de la de patrones.
