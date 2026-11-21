# Story 4 · Bloque 3 · Auriculares con volumen controlable

**Estado:** COMPLETA. Respalda los tres criterios de la story 4 y los cinco del Bloque 3 (ver "Conclusión"). La
atenuación a 32 kHz se aprueba con evidencia indirecta y el criterio de los auriculares, con el ruido por
contacto pendiente.

## Procedimiento
- Circuito de §3.7 en protoboard, armado y medido por etapas: fuente, filtro, acople y volumen, LM386 y
  salida. Nombres: R1, R2, C1 y C2 (filtro), C3 (1 µF), R3 (150 kΩ), P1 (pote de 10 kΩ log), C4 y C5
  (100 nF y 100 µF en el pin 6 del LM386), R4 y C6 (10 Ω + 47 nF), C7 (220 µF), R5 y R6 (100 Ω por canal)
  y RL1 y RL2 (carga de prueba).
- Esquemático, lista de materiales y conexiones por pin: `hardware/etapa-audio.md`.
- Alimentación de audio: cargador de 5 V aparte, con el negro a GND de la LPCXpresso y el rojo solo a la
  etapa. La LPCXpresso, por USB con el firmware de la story 3 (senoide de 125 Hz).
- Instrumento: multímetro UNI-T UT39A+. En AC no marca la continua (los 5 V del cargador dieron 0,00 V en
  AC), así que se midió en AC directo, sin capacitor en serie. Con una senoide de 125 Hz, la lectura en Vrms
  vale sea o no true RMS.
- Sin osciloscopio.

## Componentes montados

| Componente | Nominal | Medido | Desvío |
|---|---|---|---|
| R1 | 8,2 kΩ | 8,04 kΩ | −2,0 % |
| R2 | 8,2 kΩ | 8,03 kΩ | −2,1 % |
| C1 | 2,2 nF ("222") | 2,34 nF | +6,4 % |
| C2 | 1 nF ("102") | 1,034 nF | +3,4 % |
| R3 | 150 kΩ | 143,9 kΩ | −4,1 % |
| P1 | 10 kΩ log | 10,5 kΩ | +5,0 % |
| R5 / R6 | 100 Ω | 99,4 / 99,4 Ω | −0,6 % |
| RL1 / RL2 | 16 Ω | 15,9 / 15,8 Ω | −0,6 / −1,2 % |

- C1 y C2 son cerámicos de disco, marcados solo con el código, sin letra de tolerancia ni dieléctrico.
  Benja cree que son C0G; no se verificó. En lugar de la tolerancia se usa el valor medido.
- P1 se midió entre las patas 1 y 3, fuera del circuito. Las primeras lecturas (8,24 y 10,48 kΩ) se
  tomaron dentro del circuito, con la entrada del LM386 en paralelo, y se descartaron.
- C3 a C7 y R4 no se midieron.

## Mediciones por etapa
Lo esperado sale del diseño de §3.7, salvo donde se indica.

**Fuente**

| Punto | DC | AC | Esperado |
|---|---|---|---|
| Cargador con 200 Ω (R5 + R6) | 4,92 V | 0,00 V | el LM358 llega a Vcc − 1,5 V = 3,42 V, más que el pico de 3,27 V |
| Pin 6 del LM386, con la etapa | 5,01 V | | |

**Filtro (LM358)**

| Punto | DC | AC |
|---|---|---|
| AOUT | 1,633 V | 1,151 Vrms |
| Pin 1 | 1,653 V | 1,154 Vrms |

**Acople y volumen, sin el LM386**

| Punto | DC | AC | Esperado |
|---|---|---|---|
| Y | 0,00 V | 1,15 Vrms | ≈ pin 1 |
| Cursor de P1 al máximo | 0,00 V | 73 mVrms | 72 mV (diseño), 78 mV (componentes medidos) |
| Cursor a la mitad | 0,00 V | 8 mVrms | 10 a 20 % del máximo si es log: da 11 % |
| Cursor al mínimo | 0,00 V | 0 mVrms | 0 |

**LM386, sin la salida**

| Punto | DC | AC | Esperado |
|---|---|---|---|
| Pin 5 | 2,52 V | | Vcc/2 = 2,505 V |
| Cursor al máximo, con el LM386 | | 59 mVrms | 60 mV (diseño), 65 mV (componentes medidos) |
| Pin 5, cursor al máximo | | 1,281 Vrms | 1,26 V (diseño); ganancia aparente 21,7 |
| Pin 5, cursor al mínimo | | 0,000 Vrms | 0 |

**Salida con RL1 y RL2 (prueba 2)**

| Punto | DC | AC |
|---|---|---|
| Punta (L), pote al mínimo | 0,00 V | 0,00 V |
| Anillo (R), pote al mínimo | 0,00 V | 0,00 V |
| Punta (L), pote al máximo | 3,6 mV | 0,168 Vrms |
| Anillo (R), pote al máximo | 5 mV | 0,169 Vrms |

## Resultado 1: filtro (criterio 1)
- Amplitud: pin 1 / AOUT = 1,154 / 1,151, **+0,02 dB** (límite ±1 dB).
- Atenuación a 32 kHz, calculada con `filtro.attenuation_db` y los R1, R2, C1 y C2 medidos: **−15,96 dB**,
  con fc = 12,7 kHz. Pide al menos 12 dB; el diseño da −15,5 dB y el peor caso de tolerancias, −12,86 dB.
  La comprueba `test_filtro_montado_atenua_al_menos_12_dB_a_32kHz` (`hardware/tests/test_filtro.py`).
- Es evidencia indirecta: el multímetro no responde a 32 kHz y no hay osciloscopio. Si C1 y C2 no fueran C0G
  y bajaran un 20 % con la temperatura, el cálculo daría −12,15 dB: seguiría cumpliendo, con poco margen.

## Resultado 2: carga de 16 Ω (criterio 2)
- L 0,168 Vrms y R 0,169 Vrms (rango 0,12 a 0,21). Continua con carga: 3,6 y 5 mV (límite 10 mV).
- `etapa.py`, con el DAC medido (3,26 Vpp) y los componentes nominales, da 0,1671 Vrms: lo medido está un
  0,5 a 1,1 % arriba. Quedó registrado en la tabla de §3.7.
- Con los pasivos medidos da 0,1805 (L) y 0,1796 (R): lo medido queda un 6 a 7 % abajo (ver "Observaciones").

## Resultado 3: auriculares (criterio 3)
- Con el pote al mínimo no se oye. Al girarlo de a poco, el tono sube cada vez más.
- Hubo bastante ruido, que cambiaba al mover la LPC. Con la LPC firme, el ruido se estabiliza y se oye solo
  el tono, bien.
- No se registró por separado si suena igual en los dos lados. Lo respaldan las mediciones de L y R
  (0,168 y 0,169 Vrms).
- Benja acepta el criterio así (ver "Decisiones").

## Resultado 4: tests
- `make -C firmware/tests test`: salida 0 (`block_cursor`, `dac_level` y el de ejemplo).
- `app/.venv/bin/pytest hardware/tests app/tests`: 5 tests pasan, 4 de hardware y el de ejemplo de la app.
  El test del filtro montado no pasó por rojo: no hay código nuevo, solo aplica `attenuation_db`, que los
  otros dos tests del filtro validan contra los números de diseño, a los valores medidos.

## Observaciones sin resolver
- **Amplitud del DAC.** El multímetro da 1,151 Vrms (3,26 Vpp). Concuerda con el DC de 1,633 V: VREFP ≈
  3,27 V, y una senoide de escala completa con esa referencia da 1,154 Vrms. Los 3,36 a 3,40 Vpp de la spec
  son del osciloscopio e incluyen los picos finos de la story 2. Los rangos de los criterios no cambian.
- **Lecturas de AC bajas contra el cálculo con componentes medidos:** cursor sin LM386 73 contra 78 mV,
  cursor con LM386 59 contra 65 mV, pin 5 1,281 contra 1,308 V y jack 0,168 contra 0,180 Vrms. Hipótesis
  sin probar: la entrada (50 kΩ) y la ganancia (20) del LM386 son típicas y no tienen tolerancia en la hoja
  de datos, y el multímetro puede perder precisión en lecturas de decenas de mV. Con los nominales el cálculo
  coincide (0,1671 Vrms), pero puede ser una compensación de errores. No cambia ningún criterio.
- **DC de AOUT contra el pin 1:** 20 mV de diferencia (1,633 y 1,653 V), más que el offset máximo del LM358
  (7 mV). Hipótesis sin probar: la lectura en DC con 1,15 Vrms de 125 Hz encima. C3 bloquea esa continua.
- **Ruido al mover la LPC.** Hipótesis: un contacto intermitente entre la LPC y la protoboard (el cable de
  GND o el de AOUT). No se localizó.
- **Dieléctrico de C1 y C2** sin verificar.

## Decisiones (09-11-2026)
- La atenuación a 32 kHz se aprueba con evidencia indirecta: el cálculo con los componentes medidos
  (−15,96 dB), por no contar con osciloscopio. Si después se mide con osciloscopio y contradice el cálculo,
  se corrige antes de seguir (§3.7).
- El nivel medido se compara con la fila de §3.7 calculada con el DAC medido (3,26 Vpp) y los componentes
  nominales.
- El criterio 3 se acepta con el ruido por contacto: con la LPC firme, el tono se oye limpio. La mejora del
  hardware queda para el final del proyecto.
- C1 y C2 se aceptan como C0G sin verificarlo, con el valor medido en lugar de la tolerancia marcada.

## Conclusión
- **Criterio 1:** cumple (+0,02 dB; −15,96 dB calculados, con evidencia indirecta).
- **Criterio 2:** cumple (0,168 y 0,169 Vrms; 3,6 y 5 mV de continua).
- **Criterio 3:** aceptado, con el ruido por contacto pendiente.
- **Bloque 3:** cinco criterios. El de atenuación, con evidencia indirecta; el de nivel sin distorsión, con
  la salvedad del ruido.
