# Story 8: Rotación de la forma de onda con EINT1

**Bloque:** 6 · **Spec:** §3.8, §3.3, §3.11, §3.12, §6 Bloque 6 · **Estado:** terminada

## Story
Como músico, quiero rotar la forma de onda con un pulsador, para cambiar el timbre de las notas que toque después.

## Criterios de aceptación
- [x] Dados los auriculares y el pulsador de EINT1 armados, con la placa recién encendida (suena senoide), cuando aprieto el pulsador una vez por onda y toco una nota después de cada pulsación, entonces suenan en orden cuadrada → diente de sierra → triangular → sinusoidal, y la pulsación siguiente vuelve a la cuadrada (la quinta posición se saltea porque no llegó ninguna onda desde Python).
- [x] Dada una nota que mantengo apretada, cuando aprieto el pulsador, entonces esa nota conserva su onda hasta que la suelto y la primera nota que toco después suena con la onda nueva. Mientras tanto, los underruns siguen en 0.
- [x] Dado el pulsador, cuando lo aprieto 20 veces, entonces la onda avanzó exactamente 20 posiciones (una por pulsación, aunque rebote), y una segunda pulsación a menos de 150 ms de la anterior se ignora.
- [x] Dado el pulsador, cuando lo mantengo apretado más de 150 ms (por ejemplo, un segundo), entonces cuenta una sola pulsación y la onda no vuelve a cambiar hasta que lo suelto; el rebote al soltar tampoco suma una pulsación.

## Plan
- Registrar primero en la spec (hecho, §3.8 y §3.11):
  - La placa arranca en senoide, así que la primera pulsación va a la cuadrada.
  - Cuadrada, sierra y triangular se generan limitadas en banda, con la misma regla de 30 armónicos de §3.3. Arrancan en cero y su pico es 32767.
  - La regla del pulsador: una pulsación cuenta si pasaron 150 ms desde la última aceptada (contador de milisegundos de SysTick) y el pin estuvo 10 ms seguidos en alto desde entonces (SysTick lo lee por GPIO). Así una pulsación mantenida cuenta una sola vez.
- Generar las tres ondas nuevas sumando armónicos hasta el 30, normalizadas al pico Q15 (el rizado de Gibbs no puede desbordar). Probar en la PC sus propiedades: simetría, que la tabla llegue al pico, las amplitudes de cada armónico y que no tenga armónicos por encima del 30.
- Escribir primero los tests y después la lógica pura de selección: el orden cíclico, el salto de la quinta cuando no hay onda de Python, la ventana de 150 ms y el rearmado al soltar (pulsación mantenida y rebote al soltar).
- Que cada voz guarde un identificador de su onda al arrancar la nota y la mezcla lea la tabla por voz, no una global. Los identificadores van en el orden de la rotación (0 cuadrada, 1 sierra, 2 triangular, 3 senoide, 4 Python) y el arreglo de punteros por identificador vive en `audio_out.c`, en RAM. Hasta la story 11 la posición 4 apunta a la senoide, y `audioOutVoiceStart` ignora un identificador fuera de rango. Test: dos voces con ondas distintas en el mismo bloque. Los 8 usos de `mixNextSample` en `test_mixer.c` se adaptan a la firma nueva sin cambiar lo que verifican.
- Atender el pulsador por EINT1 (flanco descendente, con pull-up y prioridad 4) y rearmarlo desde SysTick leyendo P2.11 por GPIO, con un contador de pulsaciones y la onda activa legibles en el depurador. Primer paso en placa: comprobar que `GPIO_ReadValue` lee el nivel de un pin con función EINT1. Probar en placa con y sin depurador, guardar la evidencia, marcar el criterio 6 y cerrar el Bloque 6.

## Fuera de alcance
- La quinta onda y su carga por UART: story 11 (Bloque 9). Acá la rotación tiene cuatro posiciones.
- ADSR: al tocar o soltar puede seguir oyéndose un click (Bloque 7).
- Igualar el volumen entre ondas (la cuadrada suena más fuerte que la senoide al mismo pico).
- Indicador visual de la onda activa (la spec no pide LED ni pantalla).
- Tablas distintas por rango de notas (§3.3 lo deja para una etapa posterior).
- La prueba prolongada (B14) y el arreglo del contacto intermitente.

## Sugerencias (a tu criterio)
- **Identificador de onda por voz, no puntero.** La story 7 dejó una sola tabla (`sineTable`) y difirió el identificador de tabla por voz a esta story, así que hay que agregarlo a `Voice`. Con un identificador, cuando llegue la quinta onda el cambio de puntero entre bloques de §3.8 alcanza a todas las voces que la usan sin tocar cada una.
- **Contador de pulsaciones de EINT1**, como `keyPressCount` de la story 7. Sin osciloscopio no se oye un rebote, y el contador deja la evidencia así: apreté 20 veces y marca 20. Cuenta las pulsaciones aceptadas, no los flancos. Veinte pulsaciones son cinco vueltas de las cuatro posiciones, así que la onda activa vuelve a senoide: la prueba de las 20 se lee en el contador, no en la onda.

## Preparación de hardware (laboratorio)
- Lo de la story 7, más un pulsador entre P2.11 y GND, sin resistencia (pull-up interno). Confirmar que P2.11 esté libre en la placa.
