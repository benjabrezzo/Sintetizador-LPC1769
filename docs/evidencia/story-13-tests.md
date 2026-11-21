# Story 13: resultados de los tests (2026-11-12)

**Entorno:** Python 3.14.9, pytest 9.2.0, PySide6 6.12.0, numpy 2.5.4, pyqtgraph 0.14.0, pyserial 3.5. Firmware de la lógica en la PC con gcc (`-std=c99 -Wall -Wextra -Werror`).

## Resultado final

| Suite | Comando | Resultado |
|---|---|---|
| Aplicación | `app/.venv/bin/pytest -v` | 60 passed (52 anteriores y 8 de la story 13), exit 0 |
| Firmware (lógica en la PC) | `make -C firmware/tests test` | 13 suites OK (11 anteriores, `test_ring_buffer` y `test_uart_protocol`), 0 `FALLA`, exit 0 |
| Firmware (hardware, solo compila) | `arm-none-eabi-gcc -c -Wall …` sobre `uart_link.c`, `audio_out.c`, `sintetizador.c`, `uart_protocol.c` y `ring_buffer.c` | 0 warnings. El enlace y la prueba en placa son de la verificación manual |

Ningún test de la lista falló en la corrida final. Los tests de la lista que pasaron en la primera corrida se indican abajo.

## Tests de la lista (ciclo rojo → verde)

"Rojo" es lo que vimos al correr el test antes de escribir el código. Cuando un test pasó en la primera corrida, no hubo rojo ni código nuevo, y se indica.

### Firmware en la PC

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 1 | `testDevuelveLosBytesEnElOrdenQueEntraron` (`test_ring_buffer.c`) | C1 | No existía `ring_buffer.c` (el build no encuentra la regla) | Buffer FIFO con dos contadores, sin vuelta ni control de lleno |
| 2 | `testDaLaVueltaSinPerderBytes` | C1 | `FALLA` en cada byte leído después del 256 | Índice módulo `UART_RX_BUFFER_SIZE` |
| 3 | `testLlenoDescartaElByteNuevoYLoCuenta` | C1 | No compilaba: sin el campo `dropped` | Control de lleno: descarta el byte nuevo, suma a `dropped` y devuelve 0 |
| 4 | `testTramaValidaAvisaYElBufferQuedaIgualALaTablaEnviada` (`test_uart_protocol.c`) | C1 | No existía `uart_protocol.c` | Máquina de estados que copia el payload (byte bajo primero) y avisa en el CHK |
| 5 | `testTramaConA5DentroDelPayloadNoSeReinicia` | C1 | Pasó sin código nuevo | — |
| 6 | `testTramaPartidaEnVariosLlamadosAvisaSoloAlFinal` | C1 | Pasó sin código nuevo | — |
| 7 | `testChecksumMaloNoAvisa` | C1 | 3 `FALLA`: CHK equivocado, un bit del payload y un bit de CMD | Suma corrida de CMD a payload y control en el CHK |
| 8 | `testLenDistintoDe512SeDescartaSinEscribirElBuffer` | C1 | 4 `FALLA`: LEN = 0, 511, 513 y 0xFFFF escribían en el buffer | Guarda LEN y descarta la trama si no vale 512 |
| 9 | `testDespuesDeUnaTramaInvalidaAceptaLaSiguienteValida` | C1 | Pasó sin código nuevo | — |

### Aplicación

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 10 | `test_trama_de_tabla_en_cero_tiene_cabecera_largo_y_checksum` | C1 | `ModuleNotFoundError`: sin `core.protocol` | `build_upload_wave_frame` y `checksum` (`core/protocol.py`) |
| 11 | `test_muestras_se_codifican_int16_little_endian` | C1 | Pasó sin código nuevo | — |
| 12 | `test_suma_de_cmd_a_chk_da_cero_modulo_256` | C1 | Pasó sin código nuevo | — |
| 13 | `test_envio_ok_cuando_llega_el_ack` | C1 | `ModuleNotFoundError`: sin `core.board_link` | `send_wave` (`core/board_link.py`) |
| 14 | `test_envio_falla_por_timeout_si_no_hay_respuesta` | C1 | Pasó sin código nuevo | — |
| 15 | `test_una_respuesta_que_no_es_ack_no_cuenta_como_exito` | C1 | Pasó sin código nuevo | — |
| 16 | `test_ventana_tiene_selector_de_puerto_y_boton_enviar` | C1 | `AttributeError`: sin `list_serial_ports` en la ventana | `list_serial_ports` y los widgets `port_selector`, `send_button` y `status_label` |
| 17 | `test_boton_enviar_manda_la_tabla_del_editor` | C1 | `AttributeError`: sin `send_to_port` en la ventana | `send_to_port` (abre el puerto a 115200 con timeout de 1 s) y el slot `_send_to_board` |

Los tests que pasaron en la primera corrida (5, 6, 9, 11, 12, 14 y 15) son guardas: comprueban comportamiento que el código mínimo del test anterior ya daba y que no debe romperse. No prueban código nuevo.

No se hizo el test opcional de `test_mixer.c` (decisión de Benja). Los criterios 2 y 3 de la story dependen del hardware y de la rotación ya probada en `test_wave_select.c` (con y sin onda de Python): se verifican a mano.

## Decisiones tomadas durante el ciclo

- **Checksum:** incluye CMD, LEN_L, LEN_H y el payload, para que un largo corrupto también lo rompa. Registrado en spec §3.14 y en `docs/protocolo.md`.
- **Módulos:** `ring_buffer` (lógica) con los contadores libres de 16 bits y política de descartar el byte nuevo cuando está lleno, contado en `dropped`. `uart_protocol` (lógica) y `uart_link` (hardware: driver e ISR), con los nombres de la spec §5. Sin módulo `wavetable`: los dos buffers viven en `audio_out`.
- **Parser:** acepta una sola trama (UPLOAD_WAVE, LEN = 512). Descarta sin responder la que tenga otro LEN o el checksum malo. **No verifica CMD**: el despacho por CMD entra con SET_EQ en la story 14 (ecualizador); PING y los NACK se descartaron.
- **Sin `audioIsrLock` en `audioOutPythonWaveActivate`** (se aparta del plan). Ese bloqueo lo toma solo SysTick y no se anida: si SysTick interrumpiera al loop principal en medio del bloqueo, su `audioIsrUnlock` reactivaría la IRQ del DMA antes de tiempo. Además no protegía nada: el puntero es una escritura atómica de 32 bits y el loop principal no puede interrumpir la ISR del DMA, así que el cambio siempre cae entre dos bloques.
- **FIFO de la UART:** nivel de disparo de 4 bytes. Quedan 12 bytes de margen (unos 1,04 ms) frente a la ISR del audio (~0,42 ms con cuatro voces) y a SysTick.
- **`UART_Init` deja el transmisor apagado y la FIFO deshabilitada:** hace falta `UART_FIFOConfig` y `UART_TxEnable` después.
- **Trama ACK:** `uartAckFrame` es una constante de `uart_protocol.c` sin test propio en la PC. La app prueba el mismo literal del documento (`A5 06 00 00 FA`), y la igualdad se comprueba de punta a punta en la verificación manual (la app recibe el ACK).
- **`pythonWaveLoaded`:** reemplaza a la macro `PYTHON_WAVE_LOADED`. Es `volatile` y global, y la escribe el loop principal después de activar la onda.
- **Referencias a "story 12":** actualizadas a "story 13" en `wave_select.h`, `audio_out.c` y `sintetizador.c`.
- **Dependencia:** `pyserial==3.5` en `app/requirements.txt` (ya está en la lista de la constitución, principio 7).
- **Nombres:** los identificadores nuevos van en inglés (constitución, principio 8).

## Fuera de esta story (queda para la 14)

- La app no maneja `serial.SerialException` (puerto que no abre): la excepción llega sin capturar al slot.
- El botón "Enviar" no maneja la tabla nula (todas las amplitudes en cero): `Editor.table()` lanza `ValueError`.
- Una transferencia cortada a mitad deja al parser esperando el resto de la trama; la siguiente se descarta hasta resincronizar.
- Los bytes perdidos por desborde de la FIFO del hardware no se cuentan (solo `uartRxRing.dropped`).

## Hallazgo ajeno a la story

Los tests de humo de la UI (`tests/test_ui_smoke.py`) abortan de forma intermitente al cerrar el intérprete: `pure virtual method called` / `terminate called without an active exception`, exit 134, con todos los tests ya en verde. Se midió en el HEAD sin ningún cambio de esta story (9 de 50 corridas) y con los cambios (4 de 25), así que ya existía. Es un problema del cierre de Qt con las ventanas que los tests no destruyen. No se corrigió acá. La corrida final de esta story salió con exit 0.

## Incidencia en la primera prueba en placa

La app mostraba "La placa no confirmó la recepción". La causa fue el cableado: TX y RX estaban invertidos en la LPC (el adaptador TX debe ir a P0.3 y su RX a P0.2). Se corrigió solo el cableado. No hubo cambios de código después de la última corrida de tests.

## Verificación manual

| Punto | Estado |
|---|---|
| M1: ACK con dos notas sonando y `audioUnderruns` en 0 tras varios envíos, incluidos los cuatro instrumentos | Verificado: la app confirma la recepción; `audioUnderruns` = 0 y `uartRxRing.dropped` = 0 (captura `docs/evidencia/story-13-criterio1.jpg`); audio sin cortes según Benja. Los cuatro instrumentos no quedaron registrados por separado |
| M2: tabla activa en la vista Memory igual a la exportada | Verificado según Benja: la tabla de Memory coincide con la exportada |
| M3: la rotación con EINT1 saltea la quinta sin envío y la incluye después de la senoide con envío | Verificado: sin envío, EINT1 saltea la quinta. Con envío, la quinta aparece después de la senoidal y suena |
| M4: reenvío con una nota sonando en la quinta: pasa a la onda nueva y las notas con otras ondas no cambian | Verificado: al reenviar con la quinta sonando, la nota pasa a la onda nueva sin cortarse. Las notas de las otras ondas no cambiaron |
| M5: los cuatro instrumentos de la story 12 se parecen a lo que imitan (a oído) | Escuchados a oído según Benja. Juicio de parecido: pendiente de registrar |
