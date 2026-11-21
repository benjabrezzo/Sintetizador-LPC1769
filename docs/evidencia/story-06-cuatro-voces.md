# Story 6 · Bloque 5 · Cuatro voces mezcladas

**Estado:** COMPLETA. Respalda los tres criterios de la story 6 y los cinco del Bloque 5 (ver "Conclusión").
Se aprueba con evidencia indirecta, porque no hay osciloscopio: la forma de la mezcla y del recorte en AOUT
no se observó.

## Procedimiento
- Firmware de la story 6: cuatro `Voice` (oscilador, `active` y amplitud Q15) en `audio_out.c`. La ISR del
  DMA mezcla con `mixNextSample` (cada producto se desplaza 15 bits antes de sumar y después va la ganancia
  de 1/2) y convierte con `dacValueFromSampleCounted`. Todas las voces con la amplitud máxima (32767).
- Secuencia de prueba desde SysTick: acorde Do4, Mi4, Sol4, Do5 (`SEQUENCE_CHORD_NOTES`) que se arma de a una
  voz cada `SEQUENCE_STEP_MS` = 5000 ms. Con las cuatro sonando, el paso siguiente apaga las voces 1 a 3 y
  vuelve a empezar. Una vuelta son 4 pasos (≈ 20 s por diseño; no se cronometró).
- Compilado en MCUXpresso 25.6, configuración **Debug**, sin errores. Revisión aparte con
  `arm-none-eabi-gcc -Wall -Wextra -Os` sobre los .c de hardware: 0 diagnósticos.
- Etapa de audio del Bloque 3 con el cargador de 5 V y masa común, y auriculares. Sin osciloscopio.

## Resultado 1: escucha (criterio 1)
- Con depurador: 10 vueltas seguidas. Se escucha muy bien.
- Sin depurador (USB desenchufado y vuelto a enchufar): arranca igual de bien; otras 10 vueltas, igual.
- Con las cuatro notas no se percibió saturación y suena bien, aunque el contador registra recortes
  (Resultado 3). Es una percepción, no una medición.

## Resultado 2: contadores en placa (criterio 3)
Sesión con depurador en Debug, leída en la vista Expressions con la CPU suspendida.

| Variable | Valor |
|---|---|
| `audioUnderruns` | 0 |
| `audioSaturations` | 481 343 |
| `audioIsrMaxCycles[1]` a `[4]` | 23 182 · 29 326 · 35 628 · 41 573 |
| `msInStep` · `soundingVoices` | 11 · 1 (recién cerrada una vuelta) |

La duración de la sesión se estimó a ojo en 2 min. El contador indica 11 vueltas (≈ 220 s), como se ve en el
Resultado 3. En los dos casos son más de las 5 vueltas pedidas.

## Resultado 3: consistencia con la simulación de la PC
Simulación de la secuencia con la regla del firmware (voz 0 continua, voces 1 a 3 entrando en fase 0), con la
misma lógica que los tests:

| Vuelta | Saturaciones simuladas |
|---|---|
| 6 (120 s) | 263 096 |
| 11 (220 s) | 482 926 |
| medido | 481 343 (−0,33 % contra las 11 vueltas) |

Por paso: 1 y 2 voces, 0; 3 voces, ≈ 18 200 (≈ 11 % de las muestras); 4 voces, ≈ 25 400 (≈ 16 %). La
simulación es un programa descartable que no está en el repo; el mismo comportamiento lo cubre
`testElAcordeDeLaSecuenciaRecortaSoloConTresYCuatroVoces`.

## Resultado 4: duración de la ISR por bloque
Con el contador de SysTick (1 tick = 1 ciclo de CPU), medido entre la subida y la bajada de P2.8. Un bloque
dura 199 936 ciclos (64 muestras · 3 124).

| Voces | Ciclos máx. | % del bloque | Salto |
|---|---|---|---|
| 1 | 23 182 | 11,6 % | |
| 2 | 29 326 | 14,7 % | +6 144 |
| 3 | 35 628 | 17,8 % | +6 302 |
| 4 | 41 573 | 20,8 % | +5 945 |

- Costo lineal: ≈ 6 100 ciclos por voz y por bloque (≈ 95 por muestra), más una parte fija de ≈ 17 000
  (extrapolada a 0 voces).
- La spec (§3.12) estima ≈ 20 000 ciclos (10 %) para la cadena completa. Solo voces, mezcla y saturación con
  4 voces ya son el doble. Hay margen (79 % libre, underruns en 0), pero el presupuesto se revisa en el Bloque 10.
- Medido en Debug. El nivel de optimización no se verificó en el IDE (valor por defecto de MCUXpresso).

## Resultado 5: tests de la PC (criterio 2)
```
$ make -C firmware/tests test
== build/test_block_cursor
OK
== build/test_dac_level
OK
== build/test_ejemplo
OK
== build/test_mixer
OK
== build/test_oscillator
OK
(salida 0)
```
- Nuevos: 8 en `test_mixer` (una voz, voz inactiva, cuatro voces distintas, fase por voz, picos 0/1023 sin
  wrap, contador exacto, dos voces sin recorte, acorde de la secuencia) y 1 en `test_dac_level` (el contador no
  cuenta los valores exactos 0 y 1023). Los anteriores siguen pasando.
- Con rojo real (`make` salió con 2): una voz (módulo inexistente), voz inactiva (15 fallas), contador (no
  compilaba) y acorde (no compilaba). Los otros cinco pasaron de entrada porque el código ya cumplía.
- Para no fiarse de un verde sin rojo, se probaron mutaciones en copias fuera del repo. A todas las detectó
  algún test (salida 1): ignorar la cuarta voz (cuatro voces distintas), `continue`→`break` (fase por voz),
  wrap en 1023 o en 0 y suma en 16 bits (picos), contar el 1023 exacto (no lo ve el test del contador; sí el
  de `dac_level` y el de dos voces), sin ganancia de 1/2 y ganancia de 1/4 (acorde).

## Observaciones sin resolver
- **Presupuesto de la ISR:** medir también en Release y revisar el de §3.12 antes del ecualizador y la FFT (B10).
- **Voz activa:** `audioOutVoiceStart` solo es seguro sobre una voz apagada. Reutilizar una voz que suena
  (§3.2, Bloque 6) necesita otro esquema.
- **Fase de una voz apagada:** queda congelada; ningún test lo fija (la spec no lo define).
- **Pasos sin cronometrar:** los 5 s por paso son por diseño (SysTick a 1 ms), no se midieron con cronómetro.

## Decisiones (09-11-2026)
- El contador de saturaciones vive en `dac_level` (`dacValueFromSampleCounted`); `dacValueFromSample` es un envoltorio.
- Una voz nueva entra en fase 0, por el cruce por cero. `active` se escribe al final, con una barrera.
- La voz 0 arranca después de `audioOutInit()`: los dos primeros bloques salen en silencio (≈ 4 ms al encender).
- Los máximos de ciclos se llevan por cantidad de voces activas, para leer 1 y 4 voces en una sola lectura.

## Conclusión
- **Criterio 1:** cumple (20 vueltas, con y sin depurador).
- **Criterio 2:** cumple (tests en verde; salida distinta de 0 en los rojos y en las mutaciones).
- **Criterio 3:** cumple (underruns 0, saturaciones 481 343, ISR máx. 41 573 ciclos con 4 voces).
- **Bloque 5:**
  - Las cuatro voces funcionan independientemente: cumple (fase por voz y underruns en 0).
  - Cada voz puede tener otra frecuencia: cumple (suma con cuatro notas distintas y acorde).
  - La mezcla contiene las cuatro señales: cumple, con evidencia indirecta (suma muestra por muestra).
  - La suma no desborda y satura sin wrap-around: cumple (picos 0/1023 y contador exacto).
  - La reproducción continúa sin interrupciones: cumple (underruns 0 y escucha sin cortes).
