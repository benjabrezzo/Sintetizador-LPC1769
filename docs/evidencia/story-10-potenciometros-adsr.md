# Story 10 · Bloque 7 (parte 2) · Potenciómetros de la ADSR

**Estado:** CERRADA CON EVIDENCIA PARCIAL. El cierre lo decidió Benja, que asume la responsabilidad por fallas futuras. Lo que no se midió está en "Lo que no se midió".

## Procedimiento
- Spec registrada antes del código (§3.4 y Bloque 7): mapeo lineal con 50 cuentas de margen en cada punta; el Sustain sigue al pote; si A, D o R cambian a mitad de etapa, se conserva lo recorrido; se retiran los valores fijos de la story 9.
- Firmware de la story 10:
  - `envelope` (lógica pura): Attack, Decay y Release avanzan con `progress` en Q24, así que cambiar un tiempo a mitad de etapa conserva lo recorrido. `envMapTimeMs` y `envMapSustainLevel` usan un mismo helper `mapPot`. `EnvParams.sustainLevel` va en Q15.
  - `pot_filter` (lógica pura): EMA con α = 1/16 y 4 bits fraccionarios; arranca en la primera lectura.
  - `adc_input`: ADC en burst sobre AD0.0, AD0.1, AD0.2 y AD0.4 (sin AD0.3), a 200 kHz y sin interrupción. Espera la primera conversión al arrancar.
  - `sintetizador.c`: SysTick lee, filtra y mapea los potes antes de `audioOutEnvelopeStep`.
  - `config.h`: `POT_END_MARGIN` = 50, `POT_FILTER_SHIFT` = 4, canales del ADC y `ADC_RATE_HZ` = 200000.
- Hardware: lo de la story 9, más cuatro potes de 10 k entre 3,3 V y GND, con los cursores en P0.23 (A), P0.24 (D), P0.25 (S) y P1.30 (R). Ninguno a 5 V.
- Lectura en el depurador: las lecturas crudas se leen por dirección de registro, porque Expressions no evalúa macros: `(*(volatile unsigned int *)0x40034010 >> 4) & 0xFFF` para A, `…14` para D, `…18` para S y `…20` para R. `envParams` y `potFilters` se leen por nombre, y resolvieron en estas capturas.
- Compilación y tests: `arm-none-eabi-gcc -Wall` sin errores ni warnings en `sintetizador.c`, `adc_input.c`, `envelope.c` y `pot_filter.c` (sin enlazar). `make -C firmware/tests test`: 11 programas en verde al cerrar.

## Resultado 1: las puntas en el depurador (criterio 1)

Cinco capturas. En cada una, un pote está en el tope y los demás, en el mínimo, salvo la primera, con los cuatro en el mínimo.

| Captura (archivo en `docs/evidencia/`) | `envParams` {A, D, S, R} | Crudas {A, D, S, R} | Resultado |
|---|---|---|---|
| `story-10-4-al-minimo.jpg` (los cuatro al mínimo) | {2, 2, 0, 2} | {11, 5, 4, 4} | Cumple: las cuatro ≤ 50 |
| `story-10-soloA-al-minimo.jpg` (A al máximo; nombre con errata) | {2000, 2, 0, 2} | {4095, 1, 0, 1} | Cumple |
| `story-10-soloD-al-maxmio.jpg` (D al máximo; nombre con errata) | {2, 2000, 0, 2} | {1, 4095, 0, 0} | Cumple |
| `story-10-soloS-al-maximo.jpg` | {2, 2, 32767, 2} | {1, 1, 4095, 0} | Cumple |
| `story-10-soloR-al-maximo.jpg` | {2, 2, 0, 2000} | {1, 0, 1, 4095} | Cumple |

- Cada pote mueve solo su parámetro y solo su fila del ADC. Los canales son los acordados.
- Las etiquetas de las expresiones salen cortadas en estas capturas. Las crudas se identificaron por posición (A, D, S, R, en el orden en que se cargaron). Los valores encajan con el pote movido en cada caso.

## Resultado 2: potes quietos (criterio 1)

**Parte A (oído):** nota sostenida durante 30 s, sin tocar los potes. Suena estable, sin trémolo ni vibración de volumen.

**Parte B (lecturas):** cinco lecturas con la CPU suspendida. S quedó en ~2669 (≈65 %) y A, D y R, quietos. Capturas `story-10-parteB-iteracion1.jpg` a `story-10-parteB-iteracion5.jpg`.

| # | Cruda (ADDR2) | Filtrada (cuentas) | `envParams.sustainLevel` | Coherencia |
|---|---|---|---|---|
| 1 | 2669 | 2669 | 21481 | Cumple |
| 2 | 2669 | 2670 | 21489 | Cumple |
| 3 | 2669 | 2669 | 21481 | Cumple |
| 4 | 2669 | 2670 | 21489 | Cumple |
| 5 | 2668 | 2669 | 21481 | Cumple |
| Rango | 1 | 1 | 8 | |

Criterios, definidos antes de medir: rango de la filtrada de 2 cuentas o menos, y rango de `sustainLevel` menor que 20 unidades de Q15. Se cumplen (1 cuenta y 8 unidades).

Coherencia: en las cinco filas, `sustainLevel` = (filtrada − 50) · 32767 / 3995, el mismo cálculo que hace el firmware.

Notas:
- La filtrada oscila entre 2669 y 2670: la lectura queda en la frontera de redondeo. Un paso de `sustainLevel` son 8 unidades de Q15, unos 0,02 % del volumen.
- La cruda varió 1 cuenta. El ruido en reposo es chico, así que el filtro casi no tuvo que actuar. Su efecto sobre ruido grande lo cubre el test 6 de `test_pot_filter` (en la PC).
- El pote D (ADDR1) varió entre 2629 y 2631, y `decayMs` dio 1292 en las cinco lecturas.

## Resultado 3: casos de la prueba 3 en el depurador (criterio 2)

| Caso | Archivo | Esperado (A, D, S, R) | Observado `envParams` {A, D, S, R} | Crudas {A, D, S, R} | Resultado |
|---|---|---|---|---|---|
| 3a (S al máximo) | `story-10-caso-3a.jpg` | mín, mín, máx, mín | {2, 2, 32767, 2} | {0, 2, 4095, 0} | Cumple |
| 4a (R al mínimo) | `story-10-caso-4a.jpg` | mín, mín, máx, mín | {2, 2, 32767, 2} | {0, 0, 4087, 3} | Cumple |
| 4b (R al máximo) | `story-10-caso-4b.jpg` | mín, mín, máx, máx | {2, 2, 32767, 2000} | {8, 0, 4095, 4095} | Cumple |

Las capturas de los casos 1a, 1b, 2a, 2b y 3b no se usan como evidencia (ver "Observaciones").

## Resultado 4: oído (por reporte de Benja)

- **Parte A** (30 s con potes quietos): estable, ver resultado 2.
- **Prueba 3** (oído por caso, con depurador): Benja reporta que funciona en los casos. No quedó una línea por caso.
- **Prueba 7** (sin depurador): Benja reporta que funciona bien.
- **Uso:** unas 5 h de prueba, sin fallas audibles según Benja.

## Lo que no se midió

- **Prueba 4 (Audacity):** los cambios de A, D y R a mitad de etapa no se midieron en placa. Los respaldan los tests de la PC de cada caso (`testCambiarElAttack…`, `testCambiarElDecay…`, `testCambiarElRelease…`) y la prueba de uso.
- **Prueba 5 (S en varias voces con `envLevel`):** no se leyó en el depurador. El Sustain que sigue al pote lo respalda `testCambiarElSustainMueveElNivelDelSustain`, y el sonido, la prueba de uso.
- **Prueba 6 (underruns y rango en placa):** `audioUnderruns` no se leyó con el código de la story 10. El rango de `envLevel` lo cubre el barrido de la PC (`testConParametrosQueCambianElNivelNuncaSaleDeCeroAMaximo`).
- **Tiempos de A, D y R en placa:** sin osciloscopio ni Audacity. Los respalda el test de la PC (error de 1 ms).

## Observaciones

- **Nombres de archivo (sin renombrar):**
  - `story-10-soloA-al-minimo.jpg` contiene A al **máximo**. Es la captura de la prueba 1 que usa el resultado 1.
  - `story-10-A-al-maximo.jpg` contiene A al **mínimo** (caso 1a).
  - `story-10-soloD-al-maxmio.jpg`: errata.
  - `story-10-2b.jpg` y `story-10-caso3b.jpg` muestran el mismo estado con nombres de estilo distinto.
- **Capturas que no cumplen el protocolo** (`story-10-A-al-maximo.jpg`, `story-10-soloA-al-maximo.jpg`, `story-10-caso-2a.jpg`, `story-10-2b.jpg`, `story-10-caso3b.jpg`): algunas lecturas no convergieron con la cruda. Con la CPU suspendida, el filtro se congela y, si el pote se movió justo antes, la filtrada no coincide con la cruda. En 2b, el pote D está en el mínimo y no en el máximo. 2b y 3b tienen valores idénticos con seis minutos de diferencia. No son evidencia.
- **Underruns:** no se midieron con el código de la story 10. Las suspensiones pueden subirlos, así que en la prueba 2 no se leyeron.
- **Constitución, principio 3:** un bloque se aprueba con todos sus criterios cumplidos y evidencia. Este cierre tiene evidencia parcial por decisión de Benja.
- Las capturas no traen metadatos EXIF, así que no hace falta `jpegtran`.

## Conclusión

- **Criterio 1 de la story:** cumple. Puntas y potes quietos medidos en el depurador (resultados 1 y 2). El oído lo reporta Benja (resultado 4).
- **Criterio 2 de la story:** respaldado por los tests de la PC y por la prueba de uso. Sin medición en placa (prueba 4).
- **Criterio 3 de la story:** respaldado por los tests de la PC (rango y sin saltos) y por la prueba de uso (S sin clicks). Underruns en placa no medidos (prueba 6).
- **Bloque 7, criterios 3 y 4:** marcados en la spec con esta evidencia. El criterio 4 queda respaldado por los tests de la PC y la prueba de uso, sin medición en placa.
