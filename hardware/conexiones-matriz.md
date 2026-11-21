# Conexiones de la matriz (story 16), con el riel de la protoboard

Hay tres conexiones distintas:

- **Potencia** (cables 1, 2 y 3): la fuente alimenta la matriz, y la masa pasa por el riel GND de la protoboard.
- **Referencia** (cable 4): el GND de la LPC va al mismo riel, así que la LPC y la matriz comparten la masa. Sin este cable, la tensión de DIN no tiene contra qué medirse.
- **Señal** (cable 5, con la resistencia): la LPC le manda los datos a la matriz.

La LPC sigue alimentada por el USB: la fuente no se conecta a la placa.

## Diagrama

```text
┌────────────────────────┐                      ┌──────────────────────────┐
│ FUENTE HY3005          │                      │ MATRIZ CJMCU-8*8         │
│ 4,5 V, límite 1,5 A    │  cable 1 (grueso)    │ pads del lado de ENTRADA │
│  (+) rojo ─────────────┼──────────────────────┼────────> +5V             │
│  verde: no se usa      │                      │                          │
│  (-) negro ─┐          │                      │                          │
└─────────────┼──────────┘                      │                          │
              │      cable 3 (grueso, corto)    │                          │
              │   ┌─────────────────────────────┼────────> GND             │
              │   │                             │                          │
     cable 2  │   │                             │   ┌───> DIN              │
    (grueso)  │   │                             │   │                      │
              │   │                             └───┼──────────────────────┘
              │   │                                 │  cable 5 (señal)
              │   │                              [330 Ω]  pegada a la matriz
              │   │                                 │
              │   │           ┌─────────────────────┼─────┐
              │   │           │ LPC1769             │     │
              │   │           │ (USB a la PC)       │     │
              │   │           │  P0.0 ──────────────┘     │
              │   │           │                           │
              │   │   ┌───────┼── GND                     │
              │   │   │       └───────────────────────────┘
              │   │   │  cable 4 (fino)
              │   │   │
        ══════╧═══╧═══╧══════  riel GND de la protoboard
              2   3   4  (cable en cada agujero)
```

## Cable por cable

| Cable | Desde | Hasta | Tipo |
|---|---|---|---|
| 1 | Fuente, borne **rojo (+)** | Matriz, pad **+5V** del lado de entrada | grueso y corto |
| 2 | Fuente, borne **negro (−)** | **Riel GND** de la protoboard | grueso y corto |
| 3 | **Riel GND**, agujero pegado al del cable 2 | Matriz, pad **GND** del lado de entrada | grueso y corto |
| 4 | LPC, **GND** | **Riel GND**, agujero pegado al del cable 3 | fino |
| 5 | LPC, **P0.0** | Resistencia de **330 Ω**, y de ahí al pad **DIN** de la matriz | fino, con la resistencia pegada a DIN |

## El riel GND

- Los cables 2, 3 y 4 van en la **misma mitad del riel**, en agujeros vecinos y en ese orden: fuente, matriz y LPC. Así el cable de la LPC queda pegado al de la matriz y fuera del camino del ≈ 1 A, que va del 3 al 2.
- Muchas protoboards cortan el riel por la mitad. Si hay dudas, medir continuidad entre los agujeros que se usan.
- Si el + de la fuente también pasa por el riel + de la protoboard, vale lo mismo: agujeros vecinos y continuidad comprobada.

## Lo que no se conecta

- El borne **verde** de la fuente (tierra del chasis).
- El rojo (+) de la fuente a la LPC.
- Los pads del lado de salida de la matriz: **DOUT**, +5V y GND.

## Por qué así

Por los cables 1, 2 y 3 circula ≈ 1 A y por el 4, casi nada. Lo que cae en el cable 3 se le resta al margen de DIN, que a 4,5 V es de unos 150 mV: por eso el cable 3 tiene que ser corto. Con un cable corto la caída queda en decenas de mV.

Si en la prueba de 5 minutos la tensión en los pads baja mucho respecto de los 4,5 V de la fuente, o si los contactos del riel se calientan, se saca la protoboard del camino de la potencia:

- un cable directo del negro (−) de la fuente al pad GND de la matriz, en lugar de los cables 2 y 3;
- el cable 4 al pad GND de la matriz.

## Orden para conectar

1. Fuente ajustada (4,5 V y límite en 1,5 A) y **apagada**.
2. Placa enchufada por USB y corriendo con `matrixPattern = 0`: no transmite y P0.0 queda en bajo.
3. Conectar los cables **1, 2, 3 y 4**.
4. Encender la fuente. Con el multímetro en V, medir entre los pads +5V y GND de la matriz: tiene que dar 4,5 V, con el LED **CV** de la fuente encendido.
5. Recién ahora conectar el cable **5** (P0.0 → 330 Ω → DIN).
6. Desde acá, todas las pruebas de las stories 16, 17 y 18 se hacen sin tocar las conexiones.

## Orden para desconectar

1. `matrixPattern = 0`.
2. Sacar el cable 5.
3. Apagar la fuente.
4. Sacar los cables 1, 2, 3 y 4.

Con la matriz apagada, DIN solo tolera 0,5 V. Por eso la matriz se enciende antes de conectar DIN y se apaga después de sacarlo.

## Mediciones con estas conexiones

- **Osciloscopio:** la punta en el pad DIN (del lado de la matriz, después de la resistencia) y el cocodrilo de masa en el pad GND de la matriz.
- **Prueba de 5 minutos:** la corriente se lee en el display de la fuente. La tensión se mide con el multímetro en V sobre los pads +5V y GND de la matriz, no sobre el riel.
