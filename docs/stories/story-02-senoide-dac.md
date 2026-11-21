# Story 2: Senoide estable a la salida del DAC

**Bloque:** 1 · **Spec:** §3.1, §6 Bloque 1 · **Estado:** terminada

## Story
Como evaluador, quiero medir una senoide estable a la salida del DAC, para comprobar que su frecuencia es la esperada.

## Criterios de aceptación
- [x] Dado el firmware corriendo en la placa, cuando mido AOUT (P0.26) con el osciloscopio, entonces veo una senoide de 125,04 Hz ± 1 % (123,79 a 126,29 Hz), generada con Fs = 25 MHz / 781 = 32,01 kHz y 256 muestras por período.
- [x] Dada la señal en curso, cuando mido el período de la señal al comienzo, a los 5 min y a los 10 min (Fs = 256 / período; el ancho del escalón con cursores queda como evidencia cualitativa, porque su resolución de 1 µs no alcanza para ±1 %), entonces la Fs se mantiene en 32,01 kHz ± 1 % (31,24 µs por muestra) y en esos 10 minutos la senoide no muestra cortes ni reinicios.
- [x] Dado el proyecto, cuando corro los tests de la PC, entonces pasa el de conversión de muestras a valores del DAC (centro, extremos y saturación), y si lo rompo el comando termina con error.

## Plan
- Registrar en la spec la tolerancia (±1 %) y la duración de la prueba prolongada (10 min) del Bloque 1, antes de implementar.
- Generar la tabla seno de 256 muestras (Q15) con el generador de tablas. Es la primera onda que produce y será la definitiva del Bloque 4 en adelante.
- Pasar las muestras al formato del DAC (10 bits con offset de media escala) en una función de lógica sin dependencias del hardware, con test en la PC.
- Configurar el DAC con su contador en 781 y el GPDMA canal 0 en M2P con una sola LLI circular sobre un buffer en AHB banco 0. Esa es la ruta definitiva (contador del DAC + DMA) y no necesita ISR. Fs, DACCNTVAL y el canal van a la configuración centralizada.
- Medir en el laboratorio, guardar las capturas en la evidencia y marcar los criterios del Bloque 1 en la spec.

## Fuera de alcance
- Doble buffer, ISR del DMA que genera bloques y contador de underruns: story 3 (Bloque 2).
- DDS, incrementos y selección de notas (Bloque 4). La frecuencia sale solo de Fs / 256.
- Las otras tres ondas y las tablas de FFT del generador.
- Etapa analógica y escucha (Bloque 3).
- Separar los bancos AHB en la configuración de memoria: alcanza con que el buffer quede en `RAM2`.

## Sugerencias (a tu criterio)
- La función de conversión de muestra a valor del DAC es el paso 4 de la cadena de niveles (`(y >> 6) + 512` saturado). Si nace con esa forma, el Bloque 2 la reutiliza sin cambios.

## Preparación de hardware (laboratorio)
- LPCXpresso con el firmware ya grabado, alimentada por USB (arranca desde Flash, sin depurador).
- Punta del osciloscopio en AOUT (P0.26) y masa en GND de la placa.
- Pendrive para el osciloscopio, o el celular para fotografiar las capturas.
- La notebook, por si hay que recompilar.
