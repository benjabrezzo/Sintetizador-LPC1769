# Story 18: Color de las barras según la onda seleccionada

**Bloque:** 13 · **Spec:** §3.6, §3.8, §6 Bloque 13 (criterio 6) · **Estado:** en curso, lista para el laboratorio (el código y los tests de la PC están hechos; los criterios de placa quedan sin marcar hasta la visita, según la excepción de la Regla de avance)

## Story
Como músico, quiero que las barras de la matriz tomen el color de la onda seleccionada (rojo la senoide, azul la cuadrada, verde la triangular, amarillo la sierra y morado la de Python), para saber de un vistazo con qué onda van a sonar las próximas notas.

## Criterios de aceptación
- [ ] Dadas las barras del espectro en la matriz (story 17) y una onda subida desde la app, cuando rotamos las cinco ondas con el pulsador mientras suena una nota, entonces las barras se ven rojas con la senoide, azules con la cuadrada, verdes con la triangular, amarillas con la sierra y moradas con la de Python, y el color coincide con `activeWave`.
- [ ] Dada una nota sostenida en Sustain con una onda, cuando pulsamos para elegir otra, entonces las barras cambian de color enseguida y todas juntas, aunque la nota siga sonando con la onda anterior y la altura de las barras no cambie.

## Plan
- Lógica pura con tests en la PC: `ledWaveColor(wave)` en `led_patterns`, con la tabla indexada por el identificador de onda (`WAVE_*` de `wave_select.h`, con inicializadores designados para no depender del orden del enum). Los tests comparan cada una de las cinco ondas con los valores de partida de las Notas, escritos a mano, y comprueban que un identificador fuera de rango devuelva rojo.
- Una sola tabla para las barras y para el patrón de colores de la story 16: `ledPatternColor` la recorre en el orden de la spec (senoide, cuadrada, triangular, sierra y Python: rojo, azul, verde, amarillo y morado), así que el patrón 4 conserva su orden. Los colores se ajustan en un solo lugar.
- Valores de partida en el código: amarillo (255, 160, 0) y morado (170, 0, 255), en lugar de los valores completos. Se actualizan los esperados de `test_led_patterns.c` y su regla del Makefile (pasa a depender de `wave_select.h`).
- Integración: `updateMatrix` pinta las barras con `ledWaveColor(activeWave)`, leída una vez por cuadro de pantalla, en lugar de `BARS_COLOR` (que se elimina). Se actualizan los comentarios de `MATRIX_PATTERN_SPECTRUM` (`config.h`) y de `ledPatternColor` (`led_patterns.h`). La placa sigue arrancando sin transmitir.
- Compilación de verificación sin warnings antes de la visita.
- En el laboratorio, después de validar la story 17: rotar las cinco ondas con una nota sonando y sacar una foto de cada color, con `activeWave` en Expressions.
- Si en el laboratorio se ajustan el amarillo o el morado, se pasan los valores finales a la tabla y a los esperados de los dos tests (`test_led_patterns.c` y el de `ledWaveColor`), y los tests pasan antes de cerrar la story (spec §7.6).

## Fuera de alcance
- Color o paleta por intensidad, y color por nota (cada voz con su onda).
- Cambiar el orden de rotación o el salto de la quinta onda sin onda cargada (stories 8 y 13).
- Otro brillo que el 25 %, y medir de nuevo el consumo: el peor caso sigue siendo el blanco (story 16).
- La prueba prolongada de todo el sistema (story 19).

## Notas
- Decisiones confirmadas el 2026-11-15:
  - Los colores se comparten con el patrón de colores de la story 16 en vez de repetirlos.
  - El amarillo y el morado se ajustan a ojo con el patrón 4 (`MATRIX_PATTERN_COLORS`) en la visita de la story 16, para que se distingan del verde y del rojo. Valores de partida: amarillo (255, 160, 0) y morado (170, 0, 255). Mientras los nombres de los colores no cambien, no hace falta tocar la spec; si alguna mezcla no se distingue con ningún valor, se cambia primero el color en la spec.
- Decisiones al revisar el plan (2026-11-15):
  - La tabla se indexa por identificador de onda y no por el orden de la spec: los identificadores siguen la rotación (0 cuadrada, 1 sierra, 2 triangular, 3 senoide, 4 Python), así que `colors[activeWave]` con la tabla del patrón 4 daría colores cruzados (la senoide saldría amarilla).
  - Un identificador fuera de rango devuelve rojo, el color de la onda con que arranca la placa. `waveSelect` no lo genera, pero `activeWave` se puede escribir desde Expressions.
  - El criterio 2 pide la nota en Sustain: durante el Attack o el Decay la altura de las barras cambia aunque no se toque el pulsador.
- Por qué el ajuste: a igual valor de PWM, en el WS2812B el verde se ve mucho más brillante que el rojo y el azul es el más tenue. Con valores completos, el amarillo tiende a verde lima (se confunde con la triangular) y el morado a fucsia (se confunde con la senoide si el azul flojea). A 4,5 V la tensión no debería ser el problema (las barras cargan mucho menos que el blanco de la story 16); sí lo sería si se baja a 4,0 V (spec §3.15).
- Sin una onda subida desde la app no hay morado: EINT1 saltea la quinta onda si no llegó ninguna.
- Fotos de evidencia: al suspender la CPU para leer `activeWave`, la matriz conserva el último cuadro, así que las barras quedan quietas en la foto. Es esperable, no una falla.
- Hardware para la visita: el de la story 17, más el USB-TTL de 3,3 V en P0.2/P0.3 y la app para subir la quinta onda.
- Depende de la story 17: se valida después de ella en la misma visita. Si algo de la 17 falla, se corrige antes.
