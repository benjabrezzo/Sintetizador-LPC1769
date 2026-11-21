# Story 5: Notas afinadas por DDS

**Bloque:** 4 · **Spec:** §3.1, §6 Bloque 4 · **Estado:** terminada

## Story
Como evaluador, quiero seleccionar distintas frecuencias, compararlas con las teóricas y escucharlas, para comprobar que las notas van a estar afinadas.

## Criterios de aceptación
- [x] Dados la etapa de audio del Bloque 3 con los auriculares puestos y el firmware nuevo, cuando corre la secuencia de prueba, entonces se oye la escala de Do mayor de Do4 a Do5 repitiéndose en bucle, con cada nota estable durante su tramo y sin cortes ni silencios al cambiar de nota.
- [x] Dado el proyecto, cuando corro los tests de la PC, entonces pasan dos tests. El de afinación comprueba que el incremento de cada una de las 8 notas da su frecuencia de §3.11 con menos de 1 cent de error, con Fs = 25 MHz / 781. El de vuelta del acumulador comprueba que, al pasar por 2^24, el índice de la tabla sigue sin saltos. También siguen pasando los tests de alternancia y de nivel del DAC, y si rompo cualquiera de ellos el comando termina con error.

## Plan
- Registrar primero en la spec:
  - La tolerancia del Bloque 4: ±1 % (≈ 17 cents) en las notas y menos de 1 cent en los incrementos que se verifican en la PC.
  - La secuencia automática de prueba: Do4→Do5, unos 5 s por nota. Es temporal hasta que lleguen las teclas en el Bloque 6.
  - Que la frecuencia de las notas y la Fs fija se aprueban con evidencia indirecta, porque no hay instrumento para medirlas. Como f = ΔP·Fs/2^24, los incrementos se verifican en la PC y la Fs ya se midió en la story 2 (de 31 936 a 32 012 Hz, dentro de ±1 %). Esta story no cambia el contador del DAC, así que las notas quedan dentro de ±1 %.
  - Que la ausencia de interrupciones se aprueba con la escucha y los tests de la PC.
  - Las escalas de punto fijo: fase en Q16.16 con vuelta en 2^24, índice en los bits 23:16 y muestras en Q15.
- Que el script de tablas genere los incrementos de las 8 notas con la Fs real (25 MHz / 781), y escribir primero los tests de afinación y de vuelta del acumulador.
- Crear un módulo de lógica para el oscilador DDS, que avanza la fase y lee la tabla seno. La regla de mitades del doble buffer se mantiene igual. La posición de lectura pasa a ser la fase del DDS y se conserva entre bloques, así que hay que adaptar el test de la story 3 sin perder la verificación de la alternancia.
- La ISR del DMA genera cada bloque con el incremento de la nota actual. SysTick, que ya corre cada 1 ms, avanza la secuencia. La nota cambia entre bloques y la fase sigue continua.
- Escuchar la escala, guardar en la evidencia la salida de los tests y el cálculo indirecto, y marcar los criterios del Bloque 4 en la spec.
- Agregado durante la story: fijar desde `config.h` la prioridad del DMA (0) y la de SysTick (1), según §3.12 y el principio 9. El driver de SysTick no la fijaba y quedaba en 0, igual que el DMA.

## Fuera de alcance
- Leer el contador de underruns. Queda para la prueba prolongada de B14.
- Teclas, varias voces, mezcla y ganancia de 1/2 (Bloques 5 y 6). La nota sigue saliendo a escala completa, como hoy.
- Las ondas cuadrada, sierra y triangular, y la rotación con EINT1. Se prueba solo con la tabla seno.
- La envolvente (Bloque 7): el cambio de nota es abrupto, sin ataque ni liberación.
- Medir en placa la frecuencia de las notas, la Fs y la duración de la ISR, porque no hay instrumento (la ISR se mide en los Bloques 5 y 10, §3.12). Si después se mide y contradice la evidencia indirecta, se corrige antes de seguir (principio 3).
- La interpolación entre muestras (§3.1: sin interpolación).
- El arreglo del contacto intermitente que mete ruido (pendiente para B14).

## Sugerencias (a tu criterio)
- **Probar sin depurador.** Repetir una vuelta sin depurador, desenchufando y volviendo a enchufar el USB, y escuchar solamente. La story 2 mostró diferencias entre las dos situaciones.
- **Secuencia aislada.** Mantenerla separada y comentada como andamiaje del Bloque 4, para que sea fácil sacarla en el Bloque 6 y defenderla en la presentación.

## Preparación de hardware (laboratorio)
- LPCXpresso por USB con el firmware grabado.
- Etapa de audio del Bloque 3 armada, con el cargador de 5 V y la masa común con la placa.
- Antes de escuchar, revisar o sujetar el contacto intermitente entre la LPC y la protoboard: el ruido puede tapar las notas.
- Auriculares con el volumen al mínimo al conectar, y después a un nivel moderado.
