# Story 11: resultados de los tests (2026-11-12)

**Entorno:** Python 3.14.9, pytest 9.2.0, PySide6 6.12.0, numpy 2.5.4, pyqtgraph 0.14.0.

## Resultado final

| Suite | Comando | Resultado |
|---|---|---|
| Aplicación | `app/.venv/bin/pytest -v` | 21 passed (16 tests de la lista, 4 smoke de la ventana y `test_ejemplo`) |
| Firmware (lógica en la PC) | `make -C firmware/tests test` | 11 suites OK, 0 `FALLA`, exit 0 |

Ningún test falló en la corrida final.

## Tests de la lista (ciclo rojo → verde)

"Rojo" es lo que vimos al correr el test antes de escribir el código. Cuando un test pasó en la primera corrida, no hubo rojo ni código nuevo, y se indica.

| # | Test | Criterio | Rojo observado | Verde |
|---|---|---|---|---|
| 1 | `test_senoide_es_la_fundamental_al_100_por_ciento` | Plan (senoide) | `No module named 'core'` | `build_table`: suma de senoides con fase cero, escala a Q15 |
| 2 | `test_fundamental_mas_segundo_da_picos_en_1_y_2_con_proporcion` | C2 | `ImportError` de `spectrum` | `spectrum` y normalización al pico (adelantada del test 9, ver abajo) |
| 3 | `test_varios_armonicos_dan_picos_solo_donde_hay_amplitud` | C2 | Pasó sin código nuevo | — |
| 4 | `test_sin_picos_fuera_de_los_armonicos_configurados` | C2 | Pasó sin código nuevo | — |
| 5 | `test_espectro_cae_en_bins_enteros_sin_fugas` | Plan (periodicidad) | Pasó sin código nuevo | — |
| 6 | `test_armonicos_son_2_a_30_ni_uno_mas` | C1 | `DID NOT RAISE ValueError` | Chequeo de longitud en `build_table` (máximo 30) |
| 7 | `test_frecuencia_de_cada_armonico_es_k_por_la_nota` | C1 | `ImportError` de `harmonic_frequencies` | `harmonic_frequencies` (k × f0) |
| 8 | `test_cambiar_nota_cambia_solo_los_hz` | C1 | `ImportError` de `Editor` | `Editor` con nota, amplitudes, `set_note`, `set_amplitude`, `frequencies`, `table` |
| 9 | `test_normalizacion_pone_el_pico_absoluto_en_32767` | C3 | Pasó sin código nuevo (la normalización ya existía) | — |
| 10 | `test_cuantizacion_queda_dentro_de_int16` | Plan (rango) | Pasó sin código nuevo | — |
| 11 | `test_senoide_y_cuadrada_coinciden_con_tables_c` | Plan (firmware) | Pasó sin código nuevo | — |
| 12 | `test_espectro_se_calcula_sobre_la_tabla_cuantizada` | Plan (espectro) | `AttributeError`: `Editor` sin `spectrum` | `Editor.spectrum` = `spectrum(self.table())` |
| 13 | `test_exporta_256_enteros_uno_por_linea_arrancando_en_cero` | C3 | `AttributeError`: `Editor` sin `export` | `Editor.export` (texto, un entero por línea) |
| 14 | `test_exporta_pico_32767_y_ninguna_muestra_fuera_de_rango` | C3 | Pasó sin código nuevo | — |
| 15 | `test_onda_nula_no_exporta_y_devuelve_el_motivo` | C3 | `DID NOT RAISE ValueError` | `build_table` lanza `ValueError` con todas las amplitudes en cero; `export` calcula la tabla antes de abrir el archivo |
| 16 | `test_estado_inicial_es_fundamental_100_resto_0` | Plan (estado inicial) | Pasó sin código nuevo (lo cubre el `__init__` de `Editor` del test 8) | — |

## Decisiones tomadas durante el ciclo

- **Normalización adelantada (test 2).** Sin ella, fundamental al 100 % + segundo al 50 % desborda `int16`.
- **`Editor` en `core/armonicos.py` (test 8).** Guarda la nota y las 30 amplitudes; la lógica de "cambiar de nota cambia solo los Hz" queda fuera de la interfaz.
- **`build_table` rechaza más de 30 amplitudes (test 6) y todas en cero (test 15).**
- **Periodicidad (test 5).** Se verifica repitiendo el período dos veces: la FFT de 512 muestras no tiene energía en los bins impares.
- **Cruce con firmware (test 11).** Lee `firmware/src/tables.c`, el archivo que va a Flash, con tolerancia de 1 LSB.
- **Corrección sobre el pico.** Como todas las componentes son senos con fase cero, la onda es impar: el máximo positivo y el negativo tienen el mismo módulo. La ambigüedad sobre "pico" que había planteado no existe; la spec (±32767 en la muestra de mayor valor absoluto) ya lo dice bien.

## Verificación manual (M1 a M5)

| Punto | Resultado | Evidencia |
|---|---|---|
| M1: selector de 8 notas, La4 al abrir, 30 controles con su Hz | Verificado | las tres capturas |
| M2: mover una amplitud redibuja onda y espectro | Verificado | capturas de fundamental + segundo armónico y de varios armónicos; smoke test |
| M3: onda nula muestra aviso y no exporta | Aviso verificado a mano ("Todas las amplitudes están en cero: no hay onda que generar"); gráficos vacíos, smoke test | — |
| M4: estilo clásico de Windows con paleta forzada | Verificado en Plasma con tema claro; el oscuro no se probó | `story-11-captura-de-la-interfaz.jpg` |
| M5: capturas de evidencia | Tres: senoide, fundamental + segundo armónico, varios armónicos | `story-11-senoide.jpg`, `story-11-fundamental-con-segundo-armonico.jpg`, `story-11-captura-de-la-interfaz.jpg` |

### Archivo exportado

`story-11-onda-exportada.txt`: 256 enteros, uno por línea. Primera muestra en 0. Pico absoluto de ±32767 en las muestras 71 y 185. Ninguna muestra fuera de ±32767. Impar: v[256-n] = -v[n].

## Pendiente

- `/code-review` antes de cerrar la story (lo corre Benja).
- Marcar en la spec los criterios 1 a 5 del Bloque 8 (el 6 queda para la story 12), después del review.
