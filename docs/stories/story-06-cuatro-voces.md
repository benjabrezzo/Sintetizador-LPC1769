# Story 6: Cuatro voces mezcladas

**Bloque:** 5 · **Spec:** §3.2, §6 Bloque 5 · **Estado:** terminada

## Story
Como evaluador, quiero medir y escuchar cuatro notas simultáneas en una sola salida, para comprobar que se mezclan sin desborde.

## Criterios de aceptación
- [x] Dados la etapa de audio del Bloque 3 con los auriculares puestos y el firmware nuevo, cuando corre la secuencia de prueba, entonces se oye el acorde Do4, Mi4, Sol4, Do5 armándose de a una nota (1, 2, 3 y 4 a la vez, unos 5 s por paso) y repitiéndose en bucle. Cada nota que entra se distingue sin que cambien las que ya sonaban, y no hay cortes ni silencios, con y sin depurador.
- [x] Dado el proyecto, cuando corro los tests de la PC, entonces pasan los de la mezcla:
  - Cuatro voces con notas y amplitudes distintas dan, muestra por muestra, la suma esperada con la ganancia de 1/2.
  - Con las cuatro voces al máximo y en fase, los picos quedan en 0 y 1023 sin saltar al extremo opuesto, y el contador de saturaciones cuenta exactamente las muestras recortadas.
  - La fase de cada voz continúa entre bloques, sin depender de las otras voces.

  Siguen pasando los tests anteriores, y si rompo cualquiera el comando termina con error.
- [x] Dada la secuencia corriendo con el depurador durante al menos 5 vueltas, cuando leo los contadores, entonces los underruns están en 0 y el contador de saturaciones es mayor que 0, porque con 3 y 4 voces al máximo la suma recorta. También queda registrada la duración máxima de la ISR por bloque con cuatro voces.

## Plan
- Registrar primero en la spec, en el Bloque 5:
  - La secuencia de acordes, que reemplaza a la escala del Bloque 4 y se retira igual en el Bloque 6.
  - Las voces al máximo.
  - La aprobación con evidencia indirecta, porque no hay osciloscopio.
  - La medición de la ISR con el contador de SysTick.
  - La aclaración del paso 1 de la cadena: cada producto se desplaza antes de sumar. También va en `CLAUDE.md`.
- Escribir primero los tests y después la lógica en la PC:
  - Una voz: oscilador, estado activo y amplitud en Q15. La amplitud ocupa el lugar del nivel de la envolvente hasta el Bloque 7.
  - La mezcla de las voces activas con la ganancia de 1/2.
  - La saturación con su contador.
- La ISR del DMA genera cada bloque con la mezcla en lugar de una sola voz. Las voces cambian al empezar el bloque, como la nota en la story 5. Los contadores de saturaciones y underruns y el máximo de ciclos por bloque quedan como globales, para leerlos con el depurador.
- SysTick avanza la secuencia de acordes en lugar de la escala.
- Escuchar con y sin depurador y leer los contadores al final. Después, guardar la evidencia y marcar los criterios del Bloque 5 en la spec.

## Fuera de alcance
- Teclas y asignación de voces (voz libre → la que está en Release con menor nivel → la más antigua, y reutilización): Bloque 6.
- ADSR: la amplitud es fija y las notas entran y salen de golpe. Al reiniciar el bucle, cuando se apagan tres voces, puede oírse un click.
- Ondas cuadrada, sierra y triangular, identificador de tabla por voz y EINT1: se prueba solo con la tabla seno.
- Ecualizador (Bloque 10): por ahora, la ganancia de 1/2 pasa directo a la saturación.
- Ver en el osciloscopio la forma de la mezcla y el recorte en AOUT. Si una medición posterior contradice la evidencia, se corrige antes de seguir (principio 3).
- Un margen de tiempo para la ISR: la spec no lo fija, así que se registra el valor como base para el Bloque 10.
- Medir en placa la frecuencia de cada nota (ya se aprobó con evidencia indirecta en el Bloque 4).
- La prueba prolongada (B14) y el arreglo del contacto intermitente.

## Sugerencias (a tu criterio)
- **ISR con 1 y con 4 voces.** Registrar el máximo de ciclos con una voz y con cuatro da el costo por voz. Sirve para el presupuesto del Bloque 10 (§3.12 estima unos 20 000 ciclos para la cadena completa) y para defenderlo en la presentación.
- **Secuencia aislada.** Igual que en la story 5: separada y comentada como andamiaje, para sacarla sin dificultad en el Bloque 6.

## Preparación de hardware (laboratorio)
- LPCXpresso por USB con el firmware grabado.
- Etapa de audio del Bloque 3 armada, con el cargador de 5 V y la masa común con la placa.
- Revisar o sujetar el contacto intermitente entre la LPC y la protoboard antes de escuchar.
- Auriculares con el volumen al mínimo al conectar. Con la ganancia de 1/2, una nota sola suena unos 6 dB más baja que en la story 5, pero con 3 o 4 voces el DAC vuelve a usar todo el rango y recorta. No subas el volumen guiándote por una sola nota: ajustalo con el acorde completo.
