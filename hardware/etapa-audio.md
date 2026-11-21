# Etapa analógica de salida (Bloque 3)

Esquemático, lista de materiales y conexiones de la etapa de la spec §3.7, tal como quedó armada en la
protoboard y validada en la story 4. Las mediciones están en `docs/evidencia/story-04-auriculares.md`.

Cadena: AOUT → filtro Sallen-Key (LM358) → acople y volumen → LM386 → jack de 3,5 mm.

**Convenciones de los dibujos:**
- `+` une cables.
- `o` es un terminal, o un punto que sigue en otro dibujo (por ejemplo, `o F` y `F o`).
- En los electrolíticos, el `+` marca la pata positiva.
- Todas las GND son la misma línea de la protoboard.

**Nodos:**
- **X:** unión de R1, R2 y C1.
- **F:** salida del filtro (pin 1 del LM358) y + de C3.
- **Y:** − de C3 y R3.
- **P5:** salida del LM386 (pin 5), R4 y + de C7.
- **Z:** entre R4 y C6.
- **S:** − de C7, R5 y R6.

## Esquemático

### Alimentación

```text
 cargador rojo  o--+-------------------------> LM358 (8)
                   |
                   +--------+----------+-----> LM386 (6)
                            |          |
                         [ C4 ]     [ C5 ]
                            |          |
 cargador negro o--+--------+----------+-----> GND de la protoboard
                   |
                   +-------------------------> GND de la LPCXpresso
```

- El rojo del cargador va solo a la etapa de audio, nunca a la LPCXpresso, que se alimenta por USB.
- C5 es electrolítico: el + va hacia el pin 6. C4 y C5 van lo más cerca posible del pin 6.

### Filtro (LM358, mitad A)

```text
                 +------------------[ C1 ]-------------------+
                 |                                           |
                 |                      +-------------+      |
 AOUT o--[ R1 ]--X--[ R2 ]--+------(3)--| +   LM358   |      |
                            |           |   mitad A   |--(1)-+--o F
                            |     +(2)--| -           |      |
                            |     |     +-------------+      |
                         [ C2 ]   |                          |
                            |     +--------------------------+
                           GND
```

- AOUT es P0.26 de la LPCXpresso.
- Pin 8 a +5 V y pin 4 a GND (ver "Alimentación").
- La mitad B no se usa y queda atada: pin 5 a GND, pin 6 con pin 7.

### Acople, volumen y LM386

```text
 F o--[+ C3 -]--Y--[ R3 ]--+  pata 3                  +5V
                           |                           |
                          +-+                 +-------(6)-------+
             P1 10 kΩ log | |  pata 2         |                 |
                          | |<------------(3)-| +               |
                          | |                 |      LM386      |--(5)--o P5
                          +-+           +-(2)-| -               |
                    pata 1 |            |     +-------(4)-------+
                          GND          GND             |
                                                      GND
```

- Pines 1, 7 y 8 sin conectar: con 1 y 8 abiertos, la ganancia es 20.
- Girando en sentido horario, el cursor va hacia la pata 3: eso es el volumen máximo.

### Salida y jack

```text
 P5 o---+---[+ C7 -]---S---+--[ R5 ]--+------o punta (L)
        |                  |          |
     [ R4 ]                |       [ RL1 ]   solo en la prueba 2
        |                  |          |
        Z                  |         GND
        |                  |
     [ C6 ]                +--[ R6 ]--+------o anillo (R)
        |                             |
       GND                         [ RL2 ]   solo en la prueba 2
                                      |
                                     GND

                                 manga o---- GND
```

- C7 es electrolítico: el + va hacia el pin 5 (2,5 V de continua), y S queda en 0 V.
- R4 y C6 forman la red de Zobel de la aplicación típica del LM386.
- Con auriculares, RL1 y RL2 se sacan: la bobina de cada auricular queda entre la punta o el anillo y la
  manga.

## Lista de materiales

| Ref. | Componente | Nominal | Medido | Nota |
|---|---|---|---|---|
| U1 | LM358, DIP-8 | — | — | Amplificador operacional doble; se usa la mitad A |
| U2 | LM386, DIP-8 | — | — | Sufijo sin registrar; con 5 V conviene N-1 o N-3 (4 a 12 V) |
| R1 | Resistencia | 8,2 kΩ | 8,04 kΩ | Filtro |
| R2 | Resistencia | 8,2 kΩ | 8,03 kΩ | Filtro |
| C1 | Capacitor cerámico de disco ("222") | 2,2 nF | 2,34 nF | Filtro; C0G según Benja, sin verificar |
| C2 | Capacitor cerámico de disco ("102") | 1 nF | 1,034 nF | Filtro; C0G según Benja, sin verificar |
| C3 | Capacitor | 1 µF | no medido | Acople; si es electrolítico, el + va hacia el LM358 |
| R3 | Resistencia | 150 kΩ | 143,9 kΩ | Atenuación |
| P1 | Potenciómetro logarítmico | 10 kΩ | 10,5 kΩ | Volumen; medido entre las patas 1 y 3, fuera del circuito |
| C4 | Capacitor | 100 nF | no medido | Desacople del pin 6 del LM386 |
| C5 | Capacitor electrolítico | 100 µF | no medido | Desacople del pin 6 del LM386 |
| R4 | Resistencia | 10 Ω | no medido | Red de Zobel |
| C6 | Capacitor | 47 nF | no medido | Red de Zobel |
| C7 | Capacitor electrolítico | 220 µF | no medido | Bloquea la continua de la salida |
| R5 | Resistencia | 100 Ω | 99,4 Ω | Canal L |
| R6 | Resistencia | 100 Ω | 99,4 Ω | Canal R |
| J1 | Jack estéreo hembra de 3,5 mm | — | — | Punta L, anillo R, manga GND |
| RL1 | Resistencia | 16 Ω | 15,9 Ω | Carga de prueba del canal L; solo en la prueba 2 |
| RL2 | Resistencia | 16 Ω | 15,8 Ω | Carga de prueba del canal R; solo en la prueba 2 |
| — | Cargador de celular de 5 V | 5 V | 4,92 V con 200 Ω; 5,01 V con la etapa | Línea aparte de la de las matrices |

## Conexiones por pin

### U1, LM358

| Pin | Función | Va a |
|---|---|---|
| 1 | salida A | pin 2, C1 y el + de C3 (nodo F) |
| 2 | entrada − A | pin 1 |
| 3 | entrada + A | R2 y C2 |
| 4 | V− | GND |
| 5 | entrada + B | GND |
| 6 | entrada − B | pin 7 |
| 7 | salida B | pin 6 |
| 8 | V+ | +5 V del cargador |

### U2, LM386

| Pin | Función | Va a |
|---|---|---|
| 1 | ganancia | sin conectar |
| 2 | entrada − | GND |
| 3 | entrada + | cursor de P1 (pata 2) |
| 4 | GND | GND |
| 5 | salida | R4 y el + de C7 (nodo P5) |
| 6 | Vs | +5 V del cargador, con C4 y C5 a GND |
| 7 | bypass | sin conectar |
| 8 | ganancia | sin conectar |

### P1, potenciómetro

| Pata | Va a |
|---|---|
| 1 | GND |
| 2 (cursor) | pin 3 del LM386 |
| 3 | R3 |

Mirando el pote desde el eje, con las patas hacia abajo, la 1 queda a la izquierda, la 2 al centro y la 3 a
la derecha. Para comprobar la orientación, poné el eje a mitad de recorrido: entre las patas 1 y 2 tiene que
haber del 10 al 20 % del total. En la story 4 dio el 11 %, medido en tensión.

### J1, jack

| Contacto | Va a |
|---|---|
| Punta | R5 (canal L) |
| Anillo | R6 (canal R) |
| Manga | GND |

Si el jack tiene 5 patas, dos son contactos de conmutación (cerrados sin plug) y no se usan.

## Pendiente
- Ruido por un contacto intermitente entre la LPC y la protoboard (story 4). La mejora del hardware queda
  para el final del proyecto.
