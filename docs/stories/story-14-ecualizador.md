# Story 14: Ecualizador de tres bandas con perillas

**Bloque:** 10 · **Spec:** §3.5, §3.14, §6 Bloque 10 · **Estado:** cerrada con evidencia indirecta (ver `docs/evidencia/story-14-ecualizador.md`)

## Story
Como músico, quiero ajustar graves, medios y agudos con perillas en la app mientras suena, para balancear el sonido.

## Criterios de aceptación
- [ ] Dada la placa recién iniciada (las tres bandas en 0 dB) y un tono de frecuencia conocida en la zona de cada banda, cuando llevo una perilla a −12 dB y a +6 dB, primero una por vez y después las tres juntas, entonces el nivel medido a la salida cambia como predice la curva de esa configuración (±1 dB), y con las tres en 0 dB el audio es el mismo que antes del ecualizador.
- [ ] Dadas hasta cuatro notas sonando y la app conectada, cuando llevo cada perilla de un extremo al otro, entonces el timbre cambia sin clicks ni pausas, `audioUnderruns` queda en 0 y, al soltar las teclas con cualquier combinación de perillas, la salida vuelve al silencio (sin oscilación ni zumbido).
- [ ] Dadas cuatro voces sonando con el ecualizador activo, cuando leo la duración de la ISR de audio en Debug y en Release, entonces el bloque se genera con margen (menos de la mitad del bloque, ≈ 100 000 de 199 936 ciclos) y `audioUnderruns` sigue en 0.

## Plan
- Spec y protocolo primero: fijar el factor Q (§9) y definir SET_EQ en el protocolo (tres bandas de 5 coeficientes Q2.30, orden de bandas y de coeficientes, ACK al aceptar).
- Lógica del firmware con tests primero en la PC: los tres Biquad en cascada, con 0 dB como identidad. Pruebas con senoides de frecuencia conocida contra la respuesta teórica, estabilidad y ausencia de wrap-around en toda la grilla de ganancias, y estado del filtro intacto al cambiar los coeficientes.
- Placa: el parser distingue SET_EQ de UPLOAD_WAVE, valida la trama en un espacio aparte y solo con el checksum correcto los coeficientes nuevos se aplican al empezar un bloque. El ecualizador va entre la mezcla y la saturación, y se mide la duración de la ISR.
- App: cálculo de los coeficientes con validación de Q2.30 (tests con pytest) y tres perillas de −12 a +6 dB que arrancan en 0, con el envío por el mismo camino de la story 13.
- Evidencia en placa: respuesta medida en cada banda y en cascada, barrido de perillas con notas sonando, y ciclos de la ISR con 4 voces en Debug y Release.

## Fuera de alcance
- Resincronizar las perillas con la placa al conectar o tras un reinicio: la placa arranca plana y cada SET_EQ lleva las tres bandas, así que el primer movimiento resincroniza.
- NACK, PING y recuperación de envíos cortados (descartados en el Bloque 9).
- Mostrar el efecto en las matrices (stories 16 y 17).
- Frecuencias o Q ajustables, y presets de ecualización por instrumento.
- Analizar sonidos existentes.

## Notas
- Decisiones confirmadas:
  - Factor Q: 1/√2 en los dos shelves y 1 en el peaking. Hay que registrarlo en la spec (§9) antes de implementar.
  - Con esos Q y Fs = 32,01 kHz, los coeficientes entran en Q2.30 en −12…+6 dB. El más justo es el low-shelf a +6 dB, con |a1| = 1,953 contra un límite de 2: la validación en Python tiene que quedar.
  - Tolerancia de la respuesta: ±1 dB. Plazo de la ISR: menos de la mitad del bloque. Referencia: 4 voces sin ecualizador tardan 41 573 ciclos en Debug (story 6). Medir también en Release (backlog).
- Hardware:
  - Adaptador USB-TTL de 3,3 V: su TX va a P0.3 (RXD0), su RX a P0.2 (TXD0) y GND en común, sin unir VCC.
  - Etapa de audio y auriculares, teclas y potes, y lo necesario para medir el nivel de la salida.
- El parser de la story 13 no mira CMD (`docs/evidencia/story-13-tests.md`, línea 54): el despacho por CMD entra en esta story.

## Sugerencias (tu decisión)
- **Cuándo enviar desde la perilla.** `send_to_port` abre y cierra el puerto en cada envío y espera el ACK hasta 1 s en el hilo de la ventana. Recomiendo enviar al soltar la perilla (un envío por gesto), porque un salto de −12 a +6 dB es el peor caso de click. Si a oído se extraña el cambio en vivo, se limita el envío en el tiempo más adelante.
- **Medir sin hardware nuevo.** Con una onda del editor de un solo armónico tocando Do5, el armónico 2 cae en 1046 Hz (≈ 1 kHz) y el 8 en 4186 Hz (≈ 4 kHz). Para graves sirve Do4 (262 Hz), sobre la rodilla del shelf.
- **Referencias desactualizadas a la "story 14" de errores.** Resuelto el 2026-11-13: la única era la de la spec (Bloque 9). Las de la story 13 no nombraban la story de errores.
