# Story 11: Editor de armónicos

**Bloque:** 8 · **Spec:** §3.3, §6 Bloque 8 · **Estado:** terminada (ver `docs/evidencia/story-11-tests.md`)

## Story
Como músico, quiero diseñar una onda por armónicos en la app y ver su forma y espectro, para crear un timbre propio.

## Criterios de aceptación
- [x] Dada la app abierta, cuando elijo la fundamental entre las ocho notas de las teclas, entonces veo la fundamental y los armónicos 2 a 30, ni uno más, cada uno con su frecuencia en Hz. Al cambiar de nota cambian solo los Hz: las amplitudes y la onda quedan iguales.
- [x] Dada una combinación de amplitudes, cuando muevo cualquiera de ellas, entonces se actualizan un período de la forma de onda y su espectro. El espectro muestra picos solo en los armónicos con amplitud distinta de cero, en su frecuencia y con la proporción configurada (senoide, fundamental + segundo, varios armónicos).
- [x] Dada una onda con al menos un armónico distinto de cero, cuando la exporto, entonces obtengo un archivo con un período de 256 muestras `int16_t` que arranca en cero, con el pico en exactamente 32767 y ninguna muestra fuera de ±32767. Con todas las amplitudes en cero, la app no exporta y avisa por qué.

## Plan
- Registrar primero en la spec (§3.3 y Bloque 8):
  - La fundamental se elige entre las 8 notas y arranca en La4.
  - Las amplitudes van de 0 a 100 %, sin signo. Al abrir, la fundamental está al 100 % y el resto en 0 (senoide).
  - El archivo exportado es texto con 256 enteros, uno por línea.
  - Una onda nula no se exporta.
  - La interfaz usa el estilo clásico de Windows que trae Qt.
- Entorno: agregar PySide6, numpy y pyqtgraph al venv y a los requirements. Antes, comprobar que haya wheels para Python 3.14; si no, rehacer el venv con 3.13.
- Tests primero y después la lógica pura en numpy, sin GUI:
  - Síntesis aditiva de un período de 256 muestras con fase cero.
  - Normalización al pico y cuantización, con el mismo criterio que las cuatro ondas del firmware.
  - Frecuencia de cada armónico según la nota.
  - Espectro calculado sobre la tabla cuantizada (lo que va a sonar), no sobre los controles.
  - Exportación.
  - Casos de prueba: senoide, fundamental + segundo, varios armónicos con sus picos, periodicidad, rango, cambio de nota y onda nula.
- Ventana con PySide6 y pyqtgraph: selector de nota, 30 controles de amplitud con su frecuencia, gráfico de un período, espectro en Hz y botón de exportar. Todo se recalcula en cada cambio. Usa el estilo clásico de Windows de Qt con su paleta, forzados para que no los pise el tema claro u oscuro de Plasma.
- Evidencia:
  - Capturas con senoide, fundamental + segundo y varios armónicos.
  - El archivo exportado y los tests pasando.
  - Marcar los criterios 1 a 5 del Bloque 8. El 6 queda para la story 12.

## Fuera de alcance
- Instrumentos y mensajes de ADSR (story 12).
- Envío por UART y PING (Bloque 9).
- Fase editable, más de 30 armónicos y tablas por rango de notas (§3.3, etapa posterior).
- Escuchar la onda en la PC.
- Abrir un archivo exportado en el editor, o guardar y cargar configuraciones de amplitudes.
- Ecualizador (Bloque 10).
- Cambios en el generador de tablas del firmware.
- Imitar el tema Luna de XP con hojas de estilo, y cambiar la barra de título (la dibuja KWin).

## Notas
- Sin hardware: todo corre en la PC.

## Sugerencias (tu decisión)
- **Test cruzado con el firmware:** `firmware/tools/gen_tables.py` ya hace la misma suma, normalización y cuantización. Un test que compare la senoide de la app con `sineTable`, y la de impares en 1/k con `squareTable`, garantiza que la quinta onda tiene el mismo formato que las cuatro precargadas. No hace falta tocar `gen_tables.py`.
