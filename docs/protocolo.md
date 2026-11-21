# Protocolo UART — Sintetizador LPC1769

Spec §3.14. Este documento define las tramas entre la aplicación (PC) y la placa. Se completa por stories:
la 13 define UPLOAD_WAVE y el ACK, y la 14 agrega SET_EQ. No hay NACK, PING ni recuperación de transferencias
cortadas: se descartaron por tiempo (spec, Bloque 9).

## Enlace

- UART0 de la placa (TXD0 P0.2, RXD0 P0.3), 115200 baudios, 8 bits, sin paridad, 1 bit de stop (8N1), sin control de flujo.
- Adaptador USB-TTL de 3,3 V. TX del adaptador a P0.3, RX del adaptador a P0.2 y GND en común. No se une VCC.
- A 115200 baudios cada byte tarda unos 87 µs (10 bits por byte), así que una trama UPLOAD_WAVE (517 B) tarda unos 45 ms.

## Formato de trama

```
 0xA5 | CMD | LEN_L | LEN_H | payload (LEN bytes) | CHK
```

| Campo | Bytes | Contenido |
|---|---|---|
| `0xA5` | 1 | Marca de inicio |
| `CMD` | 1 | Comando (tabla de abajo) |
| `LEN_L`, `LEN_H` | 2 | Largo del payload en bytes, little-endian |
| payload | `LEN` | Datos del comando |
| `CHK` | 1 | Checksum |

**Checksum.** `CHK` es el complemento a dos de la suma de 8 bits de `CMD`, `LEN_L`, `LEN_H` y todos los bytes del
payload (no incluye `0xA5`):

```
CHK = (0x100 - ((CMD + LEN_L + LEN_H + Σ payload) & 0xFF)) & 0xFF
```

Con eso, la suma de 8 bits de `CMD` a `CHK` (incluido) da 0. Como `LEN_L` y `LEN_H` están en la suma, un largo corrupto
también rompe el checksum.

Todos los enteros de más de un byte viajan en little-endian, como en la LPC1769 y en la PC.

## Comandos

| CMD | Mensaje | Dirección | Payload | Story |
|---|---|---|---|---|
| `0x01` | UPLOAD_WAVE | PC → LPC | 256 muestras `int16_t` little-endian (LEN = 512) | 13 |
| `0x06` | ACK | LPC → PC | vacío (LEN = 0) | 13 |
| `0x04` | SET_EQ | PC → LPC | 15 coeficientes Q2.30, `int32_t` little-endian (LEN = 60) | 14 |

### UPLOAD_WAVE (`0x01`)

Un período de la onda: 256 muestras `int16_t` en Q15, con la muestra 0 primero. Cada una ocupa dos bytes, el bajo primero.
`LEN` vale siempre 512 (`0x00 0x02`).

Ejemplo con la tabla en cero:

```
A5 01 00 02 | 00 × 512 | FD
```

`CHK` = (0x100 − (0x01 + 0x00 + 0x02)) & 0xFF = 0xFD.

Ejemplo con la muestra 0 en 0x1234 y las otras 255 en cero: el payload empieza con `34 12 00 00 …`, la suma es
0x01 + 0x00 + 0x02 + 0x34 + 0x12 = 0x49 y `CHK` = 0xB7.

### SET_EQ (`0x04`)

Los coeficientes de las tres bandas del ecualizador (spec §3.5): 15 `int32_t` en Q2.30, el byte bajo primero. `LEN` vale
siempre 60 (`0x3C 0x00`). Cada coeficiente es el valor real por 2^30: 1,0 es `0x40000000` y el rango va de −2 a 2 (sin
incluir el 2).

Orden: low-shelf (200 Hz), peaking (1 kHz) y high-shelf (4 kHz); en cada banda, `b0, b1, b2, a1, a2`. Van normalizados
con a0 = 1 (fórmulas RBJ del Audio EQ Cookbook, Fs = 32,01 kHz, Q = 1/√2 en los shelves y 1 en el peaking) y cada banda
calcula:

```
y[n] = b0·x[n] + b1·x[n−1] + b2·x[n−2] − a1·y[n−1] − a2·y[n−2]
```

Cada SET_EQ lleva las tres bandas y reemplaza la configuración completa.

Ejemplo con las tres bandas en 0 dB (identidad: `b0 = 0x40000000`, el resto en cero): el payload es
`00 00 00 40` seguido de 16 bytes en cero, tres veces. La suma es 0x04 + 0x3C + 0x00 + 3 × 0x40 = 0x100, que da 0 en
8 bits, así que `CHK` = 0x00:

```
A5 04 3C 00 | (00 00 00 40, 00 × 16) × 3 | 00
```

Ejemplo con `b0` del low-shelf en 0x12345678 y el resto en cero: el payload empieza con `78 56 34 12 00 00 …`, la suma es
0x04 + 0x3C + 0x78 + 0x56 + 0x34 + 0x12 = 0x154, que da 0x54 en 8 bits, y `CHK` = 0xAC.

### ACK (`0x06`)

La placa lo envía cuando aceptó la trama. Para UPLOAD_WAVE significa que la tabla se recibió completa, el checksum cerró y
la onda ya es la quinta de la rotación. Para SET_EQ, que el checksum cerró y los coeficientes se aplican al empezar un
bloque. Trama fija, de 5 bytes, igual para los dos comandos:

```
A5 06 00 00 FA
```

## Comportamiento de la placa con UPLOAD_WAVE

1. Recibe los bytes por interrupción hacia un buffer circular de 256 B. La trama (517 B) no entra en él: el loop principal
   la va consumiendo byte a byte.
2. El payload se escribe en el buffer de recepción, que no es la tabla que suena.
3. Solo si el checksum cierra, se cambia el puntero de la quinta onda entre bloques de audio, se la marca como cargada
   para la rotación de EINT1 y se responde ACK.
4. Una trama con `LEN` distinto de 512 o con el checksum incorrecto se descarta sin responder y no modifica la onda activa.

La onda vive en RAM: se pierde al reiniciar la placa (spec §3.10).

## Comportamiento de la placa con SET_EQ

1. El parser decide por CMD y exige el `LEN` de cada comando (512 para UPLOAD_WAVE, 60 para SET_EQ). Un CMD
   desconocido, un `LEN` que no corresponde o un checksum incorrecto descartan la trama sin responder.
2. Los 60 bytes se decodifican en un juego de coeficientes de recepción, que no es el que usa el audio.
3. Solo si el checksum cierra, ese juego pasa a ser el activo entre dos bloques de audio y se responde ACK. El estado de
   los filtros no se toca, así que el cambio no produce saltos.
4. Al arrancar, las tres bandas están en 0 dB. Los coeficientes viven en RAM: se pierden al reiniciar la placa.

## Comportamiento de la aplicación

1. Arma la trama desde la tabla del editor (256 `int16_t`).
2. La envía por el puerto serie y espera el ACK con un tiempo máximo.
3. Si llega exactamente el ACK, el envío salió bien. Cualquier otra cosa, o el silencio, se informa como que la placa no
   confirmó.
4. Para SET_EQ, calcula los coeficientes de las tres bandas, valida que entren en Q2.30 y envía la trama con el mismo
   camino y el mismo criterio de ACK. Lo hace al soltar la perilla, un envío por gesto.
