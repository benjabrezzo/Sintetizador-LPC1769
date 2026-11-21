# Story 3: Audio continuo con doble buffer por DMA

**Bloque:** 2 · **Spec:** §7.1, §7.4, §6 Bloque 2 · **Estado:** terminada

## Story
Como evaluador, quiero que esa señal se mantenga durante minutos sin pausas ni discontinuidades, para confiar en que el audio no se corta.

## Criterios de aceptación
- [x] Dado el firmware corriendo en la placa con el depurador conectado, cuando mido AOUT (P0.26) durante 5 min, entonces la senoide mantiene 125,04 Hz ± 1 % (123,79 a 126,29 Hz) sin cortes ni reinicios, y el contador de underruns, leído con la vista Expressions al suspender la CPU al final (no hay lectura en vivo disponible en este IDE), sigue en 0.
- [x] Dada la senoide en pantalla junto con el pin de medición (P2.8), cuando hago zoom en los cambios de bloque (cada 2 ms), entonces no veo escalones ni saltos de fase, la forma es la misma que en la story 2 y el pulso de generación de cada bloque dura menos de 500 µs (25 % de los 2 ms).
- [x] Dado el proyecto, cuando corro los tests de la PC, entonces pasan el de continuidad entre bloques consecutivos (cada bloque empieza donde terminó el anterior) y el de alternancia (la CPU solo escribe en la mitad que el DMA no está leyendo), y si los rompo el comando termina con error.

## Plan
- Registrar primero en la spec la duración de la prueba prolongada del Bloque 2 (5 min), el umbral de 500 µs y el pin de medición P2.8 en la tabla de pines.
- Pasar de la LLI única a dos LLI circulares con interrupción de fin de transferencia, de modo que cada 2 ms el DMA avise que terminó una mitad y arranque la otra.
- En esa interrupción (prioridad máxima): subir el pin de medición, generar el bloque libre desde la tabla seno con la misma conversión a valores del DAC, comprobar si el canal ya terminó el buffer siguiente para sumar al contador de underruns y bajar el pin.
- Separar en un módulo de lógica, sin acceso a registros, la regla de qué mitad llenar y de dónde seguir leyendo la tabla, para probarla en la PC.
- Medir en el laboratorio, guardar las capturas en la evidencia y marcar los criterios del Bloque 2 en la spec.

## Fuera de alcance
- DDS, incrementos y selección de notas (Bloque 4): la frecuencia sigue saliendo de Fs / 256.
- Varias voces, ADSR, ecualizador y captura para la FFT en la cadena de niveles.
- Reporte del contador por UART (Bloque 9) y la etapa analógica (Bloque 3).
- Cambiar el LED de vida del Bloque 0.

## Sugerencias (a tu criterio)
- Cada período son exactamente 4 bloques (64 muestras sobre una tabla de 256), así que un bloque repetido o salteado casi no se ve a ojo. El test de la PC es el que realmente lo cubre: conviene que alterne bloques de contenido distinguible.
- La corrida de 5 min va con el depurador conectado (lo pide Live Expressions). Como la story 2 mostró diferencias con y sin depurador, conviene repetir un minuto sin depurador mirando solo el osciloscopio.
- El pin de medición queda fuera de P0.0–P0.7 a propósito: `FIOMASK0 = 0xFE` (matrices, Bloque 12) también enmascara FIOSET y FIOCLR en ese byte.

## Preparación de hardware (laboratorio)
- LPCXpresso con el firmware grabado y conectada por USB al depurador (MCUXpresso abierto en la notebook, para Live Expressions).
- Osciloscopio de dos canales: CH1 en AOUT (P0.26), CH2 en P2.8 y masa en GND de la placa.
- Pendrive para el osciloscopio, o el celular para fotografiar las capturas.
