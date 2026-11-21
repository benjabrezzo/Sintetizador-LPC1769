# Story 14: resultados de los tests (2026-11-12)

**Entorno:** Python 3.14.9, pytest 9.2.0, gcc 16.2.1 (`-std=c99 -Wall -Wextra -Werror`) y `arm-none-eabi-gcc` del IDE (`/usr/local/mcuxpressoide-25.6.136/ide/tools/bin`, dentro del distrobox `nxp-mcuxpresso`). Última corrida a las 17:27 sobre el commit `d6d1b5a` más cambios sin commitear. Todo lo de abajo es de la PC. La prueba en placa y lo que no se midió están en `docs/evidencia/story-14-ecualizador.md`.

## Corridas

| Hora | Momento | Aplicación (`QT_QPA_PLATFORM=offscreen app/.venv/bin/pytest -v`, desde `app/`) | Firmware (`make -C firmware/tests test`) |
|---|---|---|---|
| 16:32 | Línea base, antes del primer test | 60 passed | 13 suites OK |
| 16:43 | Tras el test 1 | 60 passed | 14 suites OK (se suma `test_equalizer`) |
| 17:00 | Tras los tests 2 a 6 | 60 passed | 14 suites OK (2,7 s) |
| 17:27 | Tras los tests 7 a 21 | **85 passed**, 0 failed (aborto intermitente al salir, ver abajo) | 14 suites OK, 0 `FALLA`, 0 warnings, exit 0 (6,8 s) |

Los 85 de la aplicación son: `test_armonicos.py` 16, `test_board_link.py` 6 (3 nuevos), `test_ejemplo.py` 1, `test_equalizer.py` 15 (archivo nuevo), `test_instrumentos.py` 25, `test_protocol.py` 6 (3 nuevos) y `test_ui_smoke.py` 16 (4 nuevos). El firmware suma 8 tests en `test_equalizer.c` (archivo nuevo) y 4 en `test_uart_protocol.c`.

**Compilación para la placa** (comando de verificación del proyecto, sin enlazar, `-Wall`): `uart_protocol.c`, `equalizer.c` y `sintetizador.c` dan exit 0 y 0 warnings.

**Duración:** `test_equalizer` tarda unos 5,8 s: el test 6 (grilla de 6859 combinaciones) unos 1,9 s y el 7 unos 2,9 s. La suite del firmware pasó de unos 2,7 s a unos 6,8 s.

## Incidentes de las corridas

1. **`clean test` en un solo `make`.** La primera corrida del firmware se lanzó con `make -C firmware/tests clean test` y falló al enlazar (`no se puede abrir el fichero de salida build/test_...`). No fue un test. Causa (verificada con `type make`): en este shell `make` es un alias de `make -j8`, así que `clean` y `test` corrieron en paralelo y `clean` borró `build/` mientras gcc escribía. Conviene `clean` y después `test`, en dos comandos.
2. **Aborto de pytest al salir (resuelto el 2026-11-13).** Se aplicó al repo la fixture `qapp` de abajo, en `app/tests/test_ui_smoke.py`. Con ella, la suite completa corrió 20 veces con exit 0 y 85 passed. En la corrida de las 17:27 pytest imprimió `85 passed` y después terminó con `pure virtual method called`, `terminate called without an active exception` y exit 134 (otras veces 139). Ningún test falló. Es intermitente y viene de la parte Qt, no de los tests nuevos de la lógica:

   | Corrida | Exit 0 |
   |---|---|
   | Base (HEAD `d6d1b5a`, 60 tests, sin cambios) | 17 de 20 |
   | Árbol actual (85 tests) | 13 de 20 |
   | Solo tests sin Qt (armónicos, enlace, ecualizador, instrumentos, protocolo) | 20 de 20 |
   | Solo `test_ui_smoke.py`, base (12 tests) | 19 de 20 |
   | Solo `test_ui_smoke.py`, actual (16 tests) | 16 de 20 |

   El problema ya existía (3 de 20 en la base) y con más ventanas en los tests parece más frecuente (7 de 20), aunque con 20 corridas la diferencia no es concluyente. En una copia fuera del repo, una fixture `qapp` que al terminar cierra y destruye las ventanas con la `QApplication` viva, hace `processEvents()` y `gc.collect()` dio 0 abortos en 30 corridas de `test_ui_smoke.py` y en 30 de la suite completa. Después se aplicó al repo (ver arriba).

## Tests de la lista (ciclo rojo → verde)

"Rojo" es lo que vimos al correr el test antes de escribir el código. Cuando un test pasó en la primera corrida, no hubo rojo ni código nuevo, y se indica: es un test guarda de código que ya existía. Criterios: C1, C2 y C3 son los de la story; "B10" es el criterio del Bloque 10 de la spec.

### Firmware en la PC: `firmware/tests/test_equalizer.c`

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 1 | `testTresBandasEn0dBSonIdentidadBitABit` | C1 | Compilaba y no enlazaba: `equalizerInit` y `equalizerProcess` sin definir | Módulo `equalizer`: Biquad en forma directa I, coeficientes Q2.30, acumulador de 64 bits. Las tres bandas arrancan en identidad (`b0 = 2^30`) |
| 2 | `testLowShelfSoloCambiaLosGravesYDejaLosAgudosEn0dB` | C1 | 4 `FALLA` (40 Hz y 200 Hz, con −12 y con +6 dB): con `equalizerSetCoefs` vacía el filtro queda en identidad | `equalizerSetCoefs` copia los coeficientes sin tocar el estado. Después se agregó el punto de flanco de 133 Hz, que fija el Q |
| 3 | `testPeakingSoloCambiaLos1kHzYDejaElRestoEn0dB` | C1 | Pasó sin código nuevo | — |
| 4 | `testHighShelfSoloCambiaLosAgudosYDejaLosGravesEn0dB` | C1 | Pasó sin código nuevo | — |
| 5 | `testCascadaDeLasTresBandasCoincideConLaCurvaTeorica` | C1 | Pasó sin código nuevo | — |
| 6 | `testSinWrapAroundConEntradaAEscalaMaximaEnTodaLaGrilla` | C2, B10 | Pasó sin código nuevo | — |
| 7 | `testLaSalidaVuelveAlSilencioExactoEnTodaLaGrilla` | C2 | 27 422 de 27 436 pruebas no vuelven a 0 (valores finales de −925 a 0), p. ej. con (−12, −12, −12) dB y el impulso la última muestra vale −29 | Realimentación del error: `EqBandState.residue` guarda la parte fraccionaria de 30 bits y se suma a la muestra siguiente (`equalizer.c`, `biquadStep`). 0 de 27 436 fuera de 0 |
| 8 | `testCambiarCoeficientesConservaElEstadoDelFiltro` | C2 | Pasó sin código nuevo | — |

### Firmware en la PC: `firmware/tests/test_uart_protocol.c`

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 9 | `testSetEqValidaAvisaYElBufferQuedaIgualALosCoeficientesEnviados` | Plan: parser | 6 `FALLA`: tres tramas sin aviso y sin coeficientes escritos | Despacho por CMD, `storeEqByte` (int32 little-endian) y `UART_PARSER_EQ_READY`. `uartParserFeed` recibe un cuarto parámetro `eqBuf` |
| 10 | `testSetEqConChecksumMaloNoAvisa` | Plan: parser | Pasó sin código nuevo | — |
| 11 | `testLenQueNoCorrespondeAlCmdSeDescartaSinEscribirLosBuffers` | Plan: parser | Pasó sin código nuevo | — |
| 12 | `testCmdDesconocidoSeDescartaAunqueLaTramaSeaValida` | Plan: parser | 12 `FALLA`: los 6 CMD desconocidos con LEN = 512 se aceptaban como onda y escribían el buffer | `expectedPayloadBytes`: solo 0x01 y 0x04 son comandos; el resto (también el ACK) se descarta |
| 13 | Tests de UPLOAD_WAVE de la story 13 (regresión) | story 13 | No aplica: siguen en verde en la misma suite | — |

### Aplicación (pytest)

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 14 | `test_0_db_da_coeficientes_de_identidad` (`test_equalizer.py`) | C1 | `ModuleNotFoundError: core.equalizer` | `eq_coefficients` devolviendo la identidad (todavía ignoraba las ganancias) |
| 15 | `test_cada_banda_cumple_su_ganancia_en_los_extremos` (6 casos) | C1 | 6 failed: con la identidad todas las bandas dan 0 dB | Fórmulas RBJ con cuantización a Q2.30; 0 dB sigue dando la identidad trivial |
| 16 | `test_toda_la_grilla_entra_en_q2_30_y_es_estable` | B10 | Pasó sin código nuevo | — |
| 17 | `test_el_caso_mas_justo_es_el_low_shelf_a_mas_6_db` | B10 | Pasó sin código nuevo | — |
| 18 | `test_una_ganancia_fuera_de_rango_se_rechaza` (5 casos) y `test_un_coeficiente_que_no_entra_en_q2_30_se_rechaza` | B10 | `ImportError: cannot import name 'to_q2_30'` | `to_q2_30` (rechaza lo que no entra) y rango de −12 a +6 dB en `eq_coefficients` |
| 19 | 3 tests de `build_set_eq_frame` (`test_protocol.py`) | Plan: app | `ImportError: cannot import name 'build_set_eq_frame'` | `build_set_eq_frame` en `core/protocol.py` |
| 20 | 3 tests de `send_eq` (`test_board_link.py`) | Plan: app | Error de importación al colectar (`send_eq`) | `send_eq` y `send_eq_to_port` en `core/board_link.py` |
| 21 | 3 tests de las perillas (`test_ui_smoke.py`) | Plan: app | 3 failed: `AttributeError` (`send_eq_to_port` y `eq_dials` no existían) | Tres `QDial` de −12 a +6 con `setTracking(False)`, etiqueta de valor, `_on_eq_dial_changed` que envía las tres bandas |
| extra | `test_cambiar_una_perilla_sin_arrastrar_la_muestra_y_la_envia` | Plan: app | Pasó sin código nuevo | Se agregó porque un mutante sobrevivía: ningún test cubría la etiqueta cuando el valor cambia sin arrastre (rueda o teclado) |

Notas: el test 7 se escribió contra el criterio `y == 0` exacto que se eligió antes. Los tests 6 y 7 se ajustaron después del arreglo: la tolerancia del 6 pasó de 2000 a 50 cuentas y el 7 de 4096 a 2048 muestras, con silencio exigido desde la 1536.

## Medidas

Los valores esperados salen de la curva teórica de cada banda (RBJ, Fs = 25 MHz / 781 = 32 010,2 Hz, Q = 1/√2 en los shelves y 1 en el peaking), calculada con numpy fuera del firmware. La tolerancia es la de la story, ±1 dB. Ganancia medida en dB por el firmware, en la PC:

| Test | Configuración | Medido |
|---|---|---|
| 2 (low-shelf) | −12 dB: 40 / 133 / 200 / 5000 Hz | −11,789 / −9,687 / −6,014 / +0,001 (teoría: −11,97 / −9,71 / −6,00 / 0) |
| 2 (low-shelf) | +6 dB: 40 / 133 / 200 / 5000 Hz | +5,968 / +4,975 / +3,000 / +0,005 (teoría: +5,99 / +4,98 / +3,00 / 0) |
| 3 (peaking) | −12 dB: 100 / 500 / 1000 / 10000 Hz | −0,161 / −3,946 / −12,000 / −0,070 |
| 3 (peaking) | +6 dB: 100 / 500 / 1000 / 10000 Hz | +0,066 / +1,874 / +6,000 / +0,028 |
| 4 (high-shelf) | −12 dB: 500 / 4000 / 6000 / 12000 Hz | −0,003 / −6,000 / −10,150 / −11,986 |
| 4 (high-shelf) | +6 dB: 500 / 4000 / 6000 / 12000 Hz | +0,001 / +3,000 / +5,188 / +5,994 |
| 5 (cascada) | 4 configuraciones × 5 frecuencias | desviación máxima contra la curva: 0,080 dB |
| 6 (grilla) | 6859 combinaciones × 4 entradas × 512 muestras | error máximo del entero contra `double`: 12,7 cuentas con la realimentación del error (932,6 con el truncado). Tolerancia 50. Pico de la referencia en `double`: 210 769, contra 2,1·10⁹ del límite de `int32` |
| 7 (silencio) | 6859 combinaciones × 4 entradas cortadas de golpe | 0 de 27 436 fuera de 0. El peor caso se asienta en la muestra 1185 (673 después del corte), en (+6, +2, −12) dB con el escalón |
| 8 (cambio de coeficientes) | 3 orígenes × 9 destinos × 2 frecuencias × 5 posiciones | error máximo contra la referencia que conserva sus estados: 11,05 cuentas (tolerancia 50) |
| 15 (app) | Mismos anclajes que los tests 2 a 4, sobre los coeficientes ya cuantizados | dentro de ±1 dB |
| 17 (app) | Máximo |coeficiente| de toda la grilla | 1,953 (a1 del low-shelf a +6 dB), contra el límite de 2 |

## El hallazgo del offset y su solución (test 7)

El `>> 30` por truncamiento dejaba un offset de continua cuando la entrada volvía a cero: con el low-shelf solo, la salida quedaba en −313 con −12 dB y en −42 con +6 dB, y con `dac_level` un `y` negativo da el código 511 del DAC en vez de 512 (pop al soltar la última tecla). La causa es que el denominador del low-shelf casi se anula en continua (1 + a1 + a2 = 0,0030 con −12 dB y 0,0011 con +6 dB), así que el error de truncado se amplifica entre 300 y 900 veces. También explicaba el desvío de 0,19 dB del test 2 (−11,789 contra −11,97; la simulación en `double` de los mismos coeficientes da −11,976) y el error de 932,6 cuentas del test 6.

Opciones probadas contra el test 7 en copias fuera del repo:

| Opción | Pruebas que no vuelven a 0 | Valores finales |
|---|---|---|
| Truncado (código original) | 27 422 de 27 436 (99,9 %) | −925 a 0 |
| Redondeo al más cercano (+2^29) | 27 375 (99,8 %) | −441 a +473 (queda una banda muerta) |
| Realimentación del error (residuo de 30 bits) | 0 | 0 |

Se aplicó la realimentación del error: una suma y una resta por banda y muestra. El costo en la ISR se mide en la verificación M3.

## Los tests guarda no son vacuosos (mutantes fuera del repo)

Para los tests que pasaron sin rojo se probaron mutantes en copias del código fuera del repo. Los conteos de los tests de C son del programa completo (un mutante puede romper más de un test). Para Python, las copias se probaron sin caché de bytecode (un `.pyc` viejo, por tamaño y segundo iguales, distorsionó una primera tanda y se repitió).

| Test | Mutante | Resultado |
|---|---|---|
| 2 | Q = 1,0 o Q = 0,5 en el low-shelf | falla el flanco de 133 Hz (2 y 1 `FALLA`) |
| 3 | Q = 1/√2 en el peaking; frecuencia central en 2 kHz; signo de `a2` | falla (el flanco de 500 Hz; 1 kHz y 500 Hz; 14 `FALLA`) |
| 4 | Q = 1,0 en el high-shelf; frecuencia 2 kHz; `x2 = x` | 5, 6 y 17 `FALLA` |
| 5 | Bandas con `state[0]` compartido; cascada de dos bandas; peaking en 2 kHz | 35, 14 y 8 `FALLA` |
| 6 | Acumulador de 32 bits; wrap a 16 bits; signo de `a1`; tolerancia de 10 cuentas con la realimentación | fallan, desde la primera o la segunda muestra; el de 10 cuentas falla porque el error real es 12,7 |
| 7 | Residuo calculado y no sumado; residuo con signo opuesto; silencio exigido desde la muestra 1100 | fallan (la última muestra vale −29 y −89; el último, porque el peor caso se asienta en la 1185) |
| 8 | `equalizerSetCoefs` reinicia todo el estado, solo `y1` e `y2`, o solo `x1` y `x2` | fallan en la muestra del cambio |
| 8 | `equalizerSetCoefs` reinicia solo el residuo | **pasa**: cambia menos de 1 cuenta por banda, y no hace falta que el test lo vea |
| 9 | Palabra en big-endian; el byte bajo no limpia la palabra; último coeficiente sin guardar | 3, 3 y 14 `FALLA` |
| 10 | SET_EQ sin verificar el checksum; checksum que solo compara el nibble bajo | 4 y 3 `FALLA` |
| 11 | Cualquier CMD acepta los dos LEN; SET_EQ sin validar el LEN; UPLOAD_WAVE acepta el LEN de SET_EQ | 2, 5 y 1 `FALLA` |
| 12 | El ACK se acepta como UPLOAD_WAVE; sin la guarda `expected != 0`; cualquier CMD como UPLOAD_WAVE | 2, 6 y 12 `FALLA` |
| 14 a 18 | 13 mutantes de `core/equalizer.py` (Q, frecuencias, signos, escala 2^31, rangos de ganancia, `to_q2_30`, 0 dB sin identidad) | los 13 fallan |
| 19 | Big-endian; CMD 0x05; int16; checksum sin el CMD | los 4 fallan |
| 20 | `send_eq` siempre True; acepta cualquier respuesta no vacía; manda la trama de la onda; no escribe | los 4 fallan |
| 21 | Seguimiento activo; manda solo la banda movida; rango a +12; arranque en +1 dB; no distingue éxito de fallo | los 5 fallan |
| 21 | La perilla no actualiza su etiqueta al soltar | **sobrevivía**: se agregó el test extra |
