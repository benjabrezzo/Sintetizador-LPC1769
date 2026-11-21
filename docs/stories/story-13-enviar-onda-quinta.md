# Story 13: Enviar la onda y tocarla como quinta

**Bloque:** 9 · **Spec:** §3.8, §3.14, §6 Bloque 9 · **Estado:** terminada (ver `docs/evidencia/story-13-tests.md`)

## Story
Como músico, quiero enviar mi onda a la placa y seleccionarla como quinta forma de onda con el pulsador de rotación, para tocar mi timbre sin cortar lo que suena.

## Criterios de aceptación
- [x] Dada la placa con dos notas sonando y la app conectada a su puerto, cuando envío la onda del editor, entonces la app confirma que la placa la recibió, la tabla de la placa es igual a la enviada y el audio no se corta (sin pausas audibles ni underruns).
- [x] Dada una onda recibida, cuando roto con el pulsador, entonces después de la senoide aparece mi onda como quinta y las notas que toco desde ahí suenan con ella. Mientras no llegue ninguna, la rotación la sigue salteando.
- [x] Dada una nota sonando con la quinta onda, cuando envío otra, entonces esa nota pasa a la onda nueva sin cortarse (se acepta un click) y las notas con las otras ondas no cambian.

## Plan
- Spec y protocolo primero:
  - Crear el documento de protocolo con la trama UPLOAD_WAVE (256 `int16_t` little-endian, LEN = 512) y su ACK vacío.
  - Registrar en §3.8 que la quinta entra en la rotación con la primera onda válida.
  - Una trama inválida se descarta sin responder. Los NACK se descartaron por tiempo (backlog, Bloque 9).
- Lógica del firmware, con los tests primero en la PC:
  - Un parser byte a byte, porque la trama (517 B) no entra en el buffer circular de 256 B.
  - Escribe el payload en el buffer de recepción que recibe por parámetro y avisa solo si el checksum cierra.
  - Tests: trama válida (la tabla queda igual a la enviada), checksum malo y trama partida en varios llamados.
- Hardware del firmware:
  - UART0 por driver a 115200 8N1, con RX por interrupción (prioridad 2) hacia el buffer circular. El loop principal se lo pasa al parser.
  - Con una tabla válida: con la IRQ del DMA bloqueada, se cambia el puntero de la quinta y se intercambian los dos buffers de 512 B (activa ↔ recepción).
  - Después se marca la quinta como cargada para EINT1 y se responde ACK desde el loop.
- App:
  - Lógica sin GUI que arma la trama desde la tabla del editor y espera el ACK con timeout. Tests con pytest sobre los bytes y el checksum, y con un puerto falso para la respuesta.
  - En la ventana: selector de puerto, botón "Enviar a la placa" y un mensaje con el resultado.
  - Agregar pyserial a las dependencias.
- Evidencia en placa:
  - Con dos notas sonando, enviar varias ondas, incluidos los cuatro instrumentos, y leer `audioUnderruns` en 0 al final.
  - Comparar la tabla activa en la vista Memory con la exportada.
  - Rotar hasta la quinta y escucharla.
  - Reenviar con la quinta sonando.

## Fuera de alcance
- NACK y sus códigos, PING, recuperación de transferencias cortadas, validación de rango y avisos de error en la app (descartados por tiempo, Bloque 9).
- Seleccionar la onda desde la app: solo se selecciona con EINT1.
- Conservar la quinta al reiniciar: vive en RAM (§3.10).
- SET_EQ y el ecualizador (Bloque 10).
- Detectar el puerto o reconectar en forma automática.

## Notas
- Hardware:
  - Adaptador USB-TTL de 3,3 V: su TX va a P0.3 (RXD0), su RX a P0.2 (TXD0) y GND en común, sin unir VCC.
  - Etapa de audio y auriculares, teclas, pulsador de EINT1 y potes.
- Acá se juzga a oído si los instrumentos de la story 12 se parecen a lo que imitan.

## Sugerencias (tu decisión)
- **No crear el módulo `wavetable` de §5.** Con dos buffers y un intercambio de punteros alcanza con el parser y `audio_out`. §5 dice que los nombres son orientativos.
- **Corregir dos referencias desactualizadas.** `wave_select.h` y `sintetizador.c` dicen que la quinta llega en la "story 12". Ahora es la 13.
