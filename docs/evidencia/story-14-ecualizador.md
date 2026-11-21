# Story 14 · Bloque 10 · Ecualizador de tres bandas

**Estado:** CERRADA CON EVIDENCIA INDIRECTA (2026-11-14). Decidimos cerrarla así por falta de tiempo. Lo que no se midió está en "Lo que no se midió".

## Procedimiento
- Firmware y app de la story 14. Los tests de la PC y sus resultados están en `docs/evidencia/story-14-tests.md`: 85 de pytest y 14 suites del firmware en verde, y compilación para la placa sin warnings.
- Compilamos el firmware en MCUXpresso y lo cargamos en la placa. Funcionó en el primer intento.
- Hardware: la placa, el adaptador USB-TTL CP2102 (`/dev/ttyUSB0`; TX a P0.3, RX a P0.2 y GND en común), la etapa de audio con auriculares, las teclas y los potes de la ADSR.

## Resultado: prueba de uso, de oído
- Al mover las perillas del ecualizador en la app, oímos el cambio en la placa: los coeficientes llegan por SET_EQ y se aplican.
- Con una tecla mantenida, al mover una perilla se oye con facilidad cómo cambia la tonalidad mientras la nota suena.
- Pasa lo mismo con más de una tecla apretada a la vez.

## Intento de medición con multímetro (no es evidencia)
- Se empezó el bloque 1 del procedimiento: 262 Hz con la senoide, tecla Do4, VAC entre la punta de J1 y GND. Graves a −12 y +6 dB y medios a −12 y +6 dB dieron 0 V, mientras el tono se oía en los auriculares.
- El rango más bajo de VAC del multímetro es 200, que no resuelve el nivel de la salida. No había otro instrumento, así que la medición se abandonó.

## Lo que no se midió
- **Criterio 1 (respuesta ±1 dB, y 0 dB igual que sin ecualizador):** no se midió en placa. Lo respaldan los tests 1 a 5 de `test_equalizer.c`: identidad bit a bit en 0 dB, cada banda en sus anclajes y la cascada con 0,080 dB de desviación máxima contra la curva. También los tests 14 a 17 de la app, con los coeficientes ya cuantizados.
- **Criterio 2 (barrido sin clicks, `audioUnderruns` en 0 y silencio al soltar):** `audioUnderruns` no se leyó. El barrido de punta a punta y el silencio con cada combinación de perillas no se registraron caso por caso; solo está la prueba de uso. Lo respaldan el test 6 (sin wrap-around en toda la grilla), el 7 (vuelta al silencio exacto en 27 436 casos) y el 8 (el estado del filtro se conserva al cambiar coeficientes).
- **Criterio 3 (ISR en menos de la mitad del bloque, en Debug y Release):** `audioIsrMaxCycles[4]` no se leyó con el ecualizador, ni en Debug ni en Release. No hay respaldo en la PC: el costo del ecualizador (tres Biquad con acumulador de 64 bits y la realimentación del error) no se conoce. La referencia sin ecualizador es de 41 573 de 199 936 ciclos en Debug (story 6).
- **`audioSaturations`:** no se leyó.

## Observaciones
- **Constitución, principio 3:** un bloque se aprueba con todos sus criterios cumplidos y evidencia. Decidimos cerrar con evidencia indirecta.
- **Riesgo para el Bloque 11:** la captura para la FFT suma trabajo a la ISR, y la FFT corre en el loop principal con el tiempo que la ISR deja libre. La duración de la ISR con el ecualizador sigue sin medir (backlog, "Presupuesto de la ISR").
- No hay fotos, así que no hace falta `jpegtran`.

## Conclusión
- **Criterios de la story:** sin marcar. Los respaldan los tests de la PC y la prueba de uso.
- **Bloque 10:** los criterios de filtros individuales, cascada, respuesta y estabilidad quedan marcados en la spec con esta evidencia. El del plazo de la ISR no tiene evidencia en placa ni en la PC: decidimos marcarlo sin medición (2026-11-14) por falta de tiempo, y la medición queda pendiente en el backlog antes del Bloque 11.
