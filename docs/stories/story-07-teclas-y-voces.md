# Story 7: Teclas y asignación de voces

**Bloque:** 6 · **Spec:** §3.2, §3.11, §6 Bloque 6 · **Estado:** terminada

## Story
Como músico, quiero tocar hasta cuatro de las ocho notas a la vez con las teclas, para elegir qué suena.

## Criterios de aceptación
- [x] Dados la etapa de audio del Bloque 3 con los auriculares y las ocho teclas armadas, cuando aprieto y suelto cada tecla de a una, entonces suena su nota de §3.11 (de Do4 a Do5) mientras la mantengo y se calla al soltarla, una sola vez por pulsación aunque la tecla rebote.
- [x] Dadas hasta cuatro teclas cualesquiera apretadas a la vez, cuando las mantengo, entonces suenan todas juntas sin cortes, y al soltar una se calla solo esa. Con el depurador, los underruns siguen en 0.
- [x] Dadas cuatro teclas apretadas, cuando aprieto una quinta, entonces reemplaza a la más antigua: suena su nota y se calla la que empezó primero, mientras las otras tres siguen. Al soltar la tecla que perdió su voz, la nota nueva no se corta.

## Plan
- Registrar primero en la spec, en el Bloque 6:
  - La división en dos stories.
  - Que sin ADSR las notas entran y salen de golpe, con la amplitud fija.
  - Que en este bloque la quinta tecla reemplaza a la voz más antigua. En el Bloque 7 (story 9), con la envolvente, se agregan antes el paso 2 de la asignación (la voz en Release con menor nivel) y la reutilización de la voz de una tecla que vuelve a sonar.
- Escribir primero los tests y después la lógica en la PC:
  - Antirrebote de las ocho teclas: 10 ms estables generan un único evento de pulsación o liberación, y los rebotes más cortos no generan nada.
  - Asignación: primero una voz libre; si no hay, la quinta tecla reemplaza a la más antigua. Cada voz recuerda su tecla y cuándo empezó, y una liberación apaga la voz solo si todavía es de esa tecla.
- SysTick lee las ocho teclas cada 1 ms, las pasa por el antirrebote y aplica los eventos a las voces. Se retira la secuencia de acordes del Bloque 5.
- Al reemplazar una voz que está sonando, la ISR del DMA no tiene que verla a medio escribir (ver sugerencias).
- Probar en placa con y sin depurador y leer los underruns. Después, guardar la evidencia y marcar en la spec los criterios 1 a 5 del Bloque 6.

## Fuera de alcance
- EINT1, las ondas cuadrada, sierra y triangular, y el identificador de tabla por voz: story 8. Todo suena con la senoide.
- ADSR (Bloque 7): al tocar o soltar puede oírse un click.
- El paso 2 de la asignación (la voz en Release con menor nivel) y la reutilización de la voz de una tecla que vuelve a sonar: story 9 (Bloque 7), porque sin envolvente ninguna voz queda en Release. Ahí la voz más antigua pasa a ser el último recurso.
- Retomar una nota reemplazada cuando se libera una voz y su tecla sigue apretada: la spec no lo pide.
- La prueba prolongada (B14) y el arreglo del contacto intermitente.

## Sugerencias (a tu criterio)
- **Deshabilitar la IRQ del DMA mientras se reemplaza una voz.** SysTick deshabilita la IRQ del DMA solo mientras escribe la voz. Es NVIC de CMSIS, no un periférico. Así la ISR ve la voz completa sin importar cuántos campos se escriban, y el esquema sigue sirviendo en el Bloque 7, cuando la voz sume la envolvente. Atrasa la ISR unos ciclos, contra 2 ms de margen. La alternativa es la de hoy: apagar, escribir y encender, con barreras. En el peor caso, esa voz pierde un bloque (2 ms).
- **Un contador de pulsaciones por tecla**, para leer en Expressions. Sin osciloscopio, cuesta oír un re-disparo por rebote con la senoide y sin envolvente. Con el contador, la evidencia queda así: apreté 20 veces y el contador marca 20.

## Preparación de hardware (laboratorio)
- LPCXpresso por USB con el firmware grabado.
- Ocho pulsadores entre P2.0–P2.7 y GND, sin resistencias (pull-up interno): la tecla 1 en P2.0 (Do4) y la tecla 8 en P2.7 (Do5). Confirmar que esos pines estén libres en la placa. P2.8 sigue siendo el pin de medición.
- Etapa de audio del Bloque 3 armada, con los 5 V y la masa común.
- Revisar o sujetar el contacto intermitente antes de escuchar.
- Auriculares al mínimo al conectar. El volumen se ajusta con cuatro teclas apretadas, porque ahí la mezcla recorta.
