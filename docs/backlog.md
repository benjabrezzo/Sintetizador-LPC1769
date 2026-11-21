# Backlog — Sintetizador LPC1769

## Contexto

Sintetizador digital polifónico de cuatro voces sobre LPC1769, con app de escritorio para diseñar ondas y
configurar el ecualizador, y espectro en matrices LED. Es el TP integrador de EDIII: lo usa quien toca y lo
evalúa la cátedra.

**Stack:** firmware en C, CMSIS, Drivers y MCUXpresso, sin RTOS. App en Python, PySide6, numpy, pyserial y
pyqtgraph (principio 7).

**Decisiones caras:** ninguna.

## Backlog

Cada story indica su bloque (`B`) y las secciones de la spec que implementa.

1. **B0** — Como evaluador de la cátedra, quiero reconstruir el firmware desde cero y verlo correr en la placa, para comprobar que el proyecto es reproducible.
2. **B1 · §3.1** — Como evaluador, quiero medir una senoide estable a la salida del DAC, para comprobar que su frecuencia es la esperada. Requiere: #1
3. **B2 · §7.1, §7.4** — Como evaluador, quiero que esa señal se mantenga durante minutos sin pausas ni discontinuidades, para confiar en que el audio no se corta. Requiere: #2
4. **B3 · §3.7** — Como músico, quiero escuchar en auriculares la señal de la placa con volumen controlable, para oírla sin distorsión y con un nivel seguro. Requiere: #2
5. **B4 · §3.1** — Como evaluador, quiero seleccionar distintas frecuencias, compararlas con las teóricas y escucharlas, para comprobar que las notas van a estar afinadas. Requiere: #3
6. **B5 · §3.2** — Como evaluador, quiero medir y escuchar cuatro notas simultáneas en una sola salida, para comprobar que se mezclan sin desborde. Requiere: #5
7. **B6 · §3.2, §3.11** — Como músico, quiero tocar hasta cuatro de las ocho notas a la vez con las teclas, para elegir qué suena. Requiere: #6
8. **B6 · §3.8** — Como músico, quiero rotar la forma de onda con un pulsador, para cambiar el timbre de las notas que toque después. Requiere: #7
9. **B7 · §3.2, §3.4** — Como músico, quiero que cada nota suba, se sostenga y se apague con su propia envolvente, para que suene como un instrumento y no haga click al tocarla o soltarla. Requiere: #7
10. **B7 · §3.4** — Como músico, quiero ajustar con cuatro potenciómetros el ataque, decaimiento, sostenido y liberación, también mientras suena una nota, para dar forma a cada sonido. Requiere: #9
11. **B8 · §3.3** — Como músico, quiero diseñar una onda por armónicos en la app y ver su forma y espectro, para crear un timbre propio.
12. **B8 · §3.3** — Como músico, quiero partir de una configuración de guitarra, piano, órgano o flauta y saber cómo ajustar los potes de la ADSR, para acercarme al sonido de ese instrumento. Requiere: #11
13. **B9 · §3.8, §3.14** — Como músico, quiero enviar mi onda a la placa y seleccionarla como quinta forma de onda con el pulsador de rotación, para tocar mi timbre sin cortar lo que suena. Requiere: #8, #11
14. **B10 · §3.5** — Como músico, quiero ajustar graves, medios y agudos con perillas en la app mientras suena, para balancear el sonido. Requiere: #13
15. **B11 · §3.6** — Como evaluador, quiero comprobar con señales conocidas que el análisis ubica cada frecuencia en su banda sin interrumpir el audio, para confiar en lo que va a mostrar el visualizador. Requiere: #14
16. **B12 · §3.6, §3.15** — Como evaluador, quiero ver patrones y colores correctos en la matriz de 8×8 mientras suena el audio, para comprobar que funciona sin afectarlo y con un consumo seguro. Requiere: #3
17. **B13 · §3.6** — Como músico, quiero ver en la matriz 8 barras con el espectro de lo que toco, para notar los cambios de timbre y ecualización. Requiere: #15, #16
18. **B13 · §3.6, §3.8** — Como músico, quiero que las barras de la matriz tomen el color de la onda seleccionada (rojo la senoide, azul la cuadrada, verde la triangular, amarillo la sierra y morado la de Python), para saber de un vistazo con qué onda van a sonar las próximas notas. Requiere: #13, #17
19. **B14 · §7, §10** — Como evaluador, quiero ver todas las funciones operando juntas en una prueba prolongada y cada periférico demostrado en placa, para dar el proyecto por terminado. Requiere: #4, #10, #12, #13, #14, #17, #18

## Notas

- **Orden:** es secuencial por la regla de avance de la spec. `Requiere` marca solo las dependencias reales.
- **Color por onda (B13):** se agregó el 2026-11-14 como story 18, aparte de las barras (17), para que cada una quede en pocos criterios. Reemplaza la paleta por intensidad de la spec. La prueba final pasó de #18 a #19.
- **Una story por bloque, salvo los Bloques 6, 7, 8 y 13:** el principio 2 pide stories dentro de un bloque. Por eso no hay un walking skeleton que cruce varios bloques. El Bloque 6 se divide en las stories 7 (teclas) y 8 (EINT1), porque sus seis criterios no entran en una. El Bloque 7 se divide en las stories 9 (envolvente y asignación) y 10 (potenciómetros), por el mismo motivo. El Bloque 8 se divide en las stories 11 (editor) y 12 (instrumentos), por el mismo motivo y porque las amplitudes y los mensajes de los instrumentos todavía no están definidos (§9).
- **Story de errores de envío (B9), descartada por tiempo:** iba a ser la 14 y agregaba los NACK, PING y la recuperación de transferencias cortadas. El Bloque 9 se cerró con la story 13, los tests de la PC y el uso real de la app (nota en el Bloque 9 de la spec). El número 14 pasó al ecualizador.
- **Actor evaluador:** se usa en las stories donde todavía no hay nada para tocar o ver.
- **Bloque 15:** sigue en la spec como ampliación y queda fuera de este backlog.
- **Quinta onda:** cualquier onda nueva se sube desde Python y se selecciona rotando con EINT1. La app no selecciona ondas.
- **Componentes:** antes de cada prueba con hardware se indica qué tiene que estar preparado y armado.
- **Ruido en la etapa de audio:** un contacto intermitente entre la LPC y la protoboard mete ruido en los auriculares (story 4). Arreglar el hardware antes de la prueba final (B14).
- **Prioridades de DMA y SysTick (B7):** ya están fijadas desde la story 5 en `config.h`, con `DMA_IRQ_PRIORITY` = 0 y `SYSTICK_IRQ_PRIORITY` = 1 (spec §3.12). En el Bloque 7 no hace falta configurarlas. UART0 y EINT1 se agregan con su bloque.
- **Reemplazar una voz activa (story 7):** resuelto: `audioOutVoiceStart` deshabilita `DMA_IRQn` mientras escribe la voz, así que sirve sobre una voz que suena. Queda un click al reemplazar hasta el ADSR (B7).
- **Voz en Release y reutilización (story 9, B7):** en la story 7, la quinta tecla reemplaza a la voz más antigua. En la story 9, con la envolvente, se agregan antes el paso 2 de la asignación (la voz en Release con menor nivel) y la reutilización de la voz de una tecla que vuelve a sonar (§3.2). Sin envolvente, ninguna voz queda en Release.
- **Presupuesto de la ISR (B10, resuelto en la story 15):** con cuatro voces y el ecualizador, medido con el Timer0, la ISR dura 126 676 a 127 912 ciclos en Debug sin optimizar (64 % del bloque; no cumple la mitad) y 37 556 a 39 016 con `-O1` (40 560 con la captura y el análisis corriendo, el 20,3 %). El Debug del proyecto compila con `-O1`. La estimación de ≈ 20 000 ciclos de §3.12 quedó corta. Pendiente: medir también en Release (fuera de alcance de la story 15). Detalle en `docs/evidencia/story-15-linea-base-isr.md`.
- **Excepción del laboratorio (2026-11-14):** las stories 16, 17 y 18 se desarrollan en la PC antes de la visita al laboratorio, donde están la fuente regulable y el osciloscopio, y se validan en una sola visita, en ese orden. Cada una conserva sus criterios y su evidencia (spec, Regla de avance).
