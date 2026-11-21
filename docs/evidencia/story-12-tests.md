# Story 12: resultados de los tests (2026-11-12)

**Entorno:** Python 3.14.9, pytest 9.2.0, PySide6 6.12.0 (Qt 6.12.0), numpy 2.5.4, pyqtgraph 0.14.0.

## Resultado final

| Suite | Comando | Resultado |
|---|---|---|
| Aplicación | `app/.venv/bin/pytest -v` | 52 passed (21 de la story 11 y 31 de la story 12) |
| Firmware (lógica en la PC) | `make -C firmware/tests test` | 11 suites OK, 0 `FALLA`, exit 0 (sin cambios de firmware) |

Ningún test falló en la corrida final.

## Tests de la lista (ciclo rojo → verde)

"Rojo" es lo que vimos al correr el test antes de escribir el código. Cuando un test pasó en la primera corrida, no hubo rojo ni código nuevo, y se indica.

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 1 | `test_cada_instrumento_carga_sus_30_porcentajes_de_la_spec` [×4] | C1 | `AttributeError`: `Editor` sin `load_instrument` | `Editor.load_instrument` con `AMPLITUDES` (`core/instrumentos.py`) |
| 2 | `test_espectro_de_cada_instrumento_respeta_las_proporciones` [×4] | C1 | Pasó sin código nuevo | — (error máximo 8 · 10⁻⁶ frente a la tolerancia 10⁻³) |
| 3 | `test_mensaje_tiene_una_linea_por_pote_en_orden` [×4] | C2 | `AttributeError`: sin `ADSR_MESSAGES` | `ADSR_MESSAGES` con el texto de la spec 3.3 |
| 4 | `test_cada_instrumento_tiene_un_mensaje_distinto` | C2 | Pasó sin código nuevo | — |
| 5 | `test_cargar_un_instrumento_no_cambia_la_nota` [×4] | C3 | Pasó sin código nuevo | — |
| 6 | `test_mover_un_control_despues_de_cargar_cambia_la_onda` [×4] | C3 | Pasó sin código nuevo | — |
| 7 | `test_despues_de_cargar_se_exporta_256_enteros_con_pico_32767` [×4] | C3 | Pasó sin código nuevo | — |
| 8 | `test_elegir_un_instrumento_mueve_los_30_controles_y_redibuja` [×4] | C1 | `AttributeError`: sin `instrument_buttons` | Un botón por instrumento y `_load_instrument` (señales bloqueadas, una sola redibujada) |
| 9 | `test_elegir_un_instrumento_muestra_su_mensaje_y_otro_lo_cambia` | C2 | `AttributeError`: sin `adsr_message_label` | Etiqueta fija con ajuste de línea, actualizada en `_load_instrument` |
| 10 | `test_cargar_un_instrumento_en_la_ventana_no_cambia_la_nota` | C3 | Pasó sin código nuevo | — |

Los tests que pasaron en la primera corrida (2, 4, 5, 6, 7 y 10) son guardas: comprueban comportamiento que ya daba la story 11 y que este bloque no debe romper. No prueban código nuevo.

## Decisiones tomadas durante el ciclo

- **Datos:** amplitudes y mensajes viven en `app/core/instrumentos.py`. `Editor.load_instrument` carga las amplitudes y no toca la nota.
- **Tabla de la spec:** se copió a mano en `tests/test_instrumentos.py` (test 1). Además se verificó que las 30 filas de §3.3 coinciden con `AMPLITUDES` en los cuatro instrumentos.
- **Test 8:** toma los valores esperados de `instrumentos.AMPLITUDES`, que el test 1 ya comparó con la spec.
- **Redondeo:** piano en k = 8 da 12,5 % exacto y queda en 13 % (empate hacia arriba, con fracciones exactas).
- **Botones** en vez de lista desplegable: cargar es una acción.
- **Redibujada única:** no tiene test. Se verificó leyendo el código: `_load_instrument` bloquea las señales de cada slider y llama a `_refrescar` una vez al final.
- **Nombres:** los identificadores nuevos van en inglés (constitución, principio 8). Los de la story 11 quedan como están.

## Verificación manual

| Punto | Estado | Evidencia |
|---|---|---|
| M1: una captura por instrumento con onda, espectro y mensaje (escritorio) | Verificado | `story-12-guitarra.jpg`, `story-12-piano.jpg`, `story-12-organo.jpg` y `story-12-flauta.jpg`, en esta carpeta. Cada captura muestra el instrumento elegido, sus 30 controles, la onda, el espectro y el mensaje de la ADSR. |
| M2: la zona del mensaje se lee bien y no se corta en el tamaño de la ventana | Verificado | Las mismas capturas, ventana de 986 × 909 px. Los cuatro mensajes se leen completos. El de Órgano ocupa dos líneas por el ajuste de texto. |

Las capturas no traen EXIF ni GPS (se comprobó), así que no hizo falta limpiarlas con `jpegtran`.

## Pendiente

- `/code-review` sobre los cambios antes del commit.
- Commit: lo hace Benja, con los mensajes propuestos en la respuesta.

Hecho: spec §6, Bloque 8, criterio 6 marcado. La story figura como terminada.
