# Story 4: Auriculares con volumen controlable

**Bloque:** 3 · **Spec:** §3.7, §6 Bloque 3 · **Estado:** terminada

## Story
Como músico, quiero escuchar en auriculares la señal de la placa con volumen controlable, para oírla sin distorsión y con un nivel seguro.

## Criterios de aceptación
- [x] Dado el filtro alimentado con los 5 V de audio y la senoide del DAC en su entrada, cuando mido con el multímetro en AC la entrada y la salida, entonces la salida tiene la misma amplitud que la entrada (±1 dB), y el cálculo con los valores verificados de los componentes montados da al menos 12 dB de atenuación a 32 kHz (diseño: −15,5 dB; peor caso con resistencias al 5 % y capacitores al 10 %: ≈ −12,9 dB).
- [x] Dada la etapa completa con la senoide del DAC y una carga de 16 Ω en cada canal del jack, cuando llevo el volumen al máximo, entonces el multímetro mide en cada canal entre 0,12 y 0,21 Vrms (diseño: ≈ 0,17 Vrms, unos 0,47 Vpp) y menos de 10 mV de continua.
- [x] Dados los auriculares puestos con el volumen al mínimo, cuando giro el pote hasta el máximo, entonces el tono sube gradualmente desde el silencio hasta un nivel fuerte, sin saltos ni raspado, suena igual en los dos lados y no se oye distorsión.

## Plan
- Registrar primero en la spec los umbrales, la carga prevista de 16 Ω, la fuente de audio (un cargador aparte, con masa común con la placa) y la decisión de aprobar la atenuación del filtro con evidencia indirecta, por no tener osciloscopio.
- Conseguir los componentes de §3.7, verificar sus valores y dejar el esquemático y la lista de materiales en la carpeta de hardware.
- Armar y medir por etapas: los 5 V del cargador ya con carga, el filtro en continua y en alterna (entrada contra salida), después el acoplamiento, el volumen y el LM386 con la carga de prueba, y los auriculares al final.
- Si un valor no cumple, registrar el ajuste en §3.7 antes de cambiar el circuito.
- Guardar en la evidencia la tabla de valores medidos contra el diseño y el cálculo de la atenuación con la decisión fechada, correr los tests existentes y marcar los criterios del Bloque 3 en la spec.

## Fuera de alcance
- Cambios de firmware: la señal sigue siendo la senoide de 125 Hz. Las otras frecuencias son del Bloque 4.
- La medición directa de la atenuación y de la forma de onda. Cuando vuelva el osciloscopio se puede sumar a la evidencia como confirmación; si contradice el cálculo, se corrige antes de seguir (principio 3).
- El nivel con la mezcla real (una nota a media escala, cuatro voces), que se ve en el Bloque 5.
- El ruido de las matrices en el audio y la unión de masas en un punto (Bloque 12).
- Volumen digital, mute y estéreo real: L y R llevan la misma señal.

## Sugerencias (a tu criterio)
- **Capacitores del filtro.** Usar C0G/NP0 o de film para los de 2,2 nF y 1 nF, y anotar su tolerancia (J = 5 %, K = 10 %). Un cerámico Y5V puede variar −20/+80 %, y con eso el cálculo del peor caso deja de cumplir.
- **Margen del LM358.** La senoide de prueba es de escala completa (3,36 a 3,40 Vpp) y el LM358 llega a unos Vcc − 1,5 V. Si al máximo se oye áspero, subir la de 150 kΩ (por ejemplo, a 180 kΩ) baja el máximo sin tocar el filtro.
- **Continua y "pop".** Medir la continua con la carga puesta: sin carga, la salida del 220 µF queda flotante y el multímetro puede leer volts por la fuga. Si al enchufar los auriculares se oye un "pop", lo resuelve una resistencia de descarga de unos kΩ a GND detrás del capacitor.
- **Nivel máximo.** 1,7 mW en auriculares de 100 dB/mW dan unos 102 dB SPL. Arrancar siempre con el volumen al mínimo y conectar la alimentación sin los auriculares puestos.
- **Zumbido.** Un cargador conmutado mete más ruido que el USB. Si se oye a volumen bajo, lo primero a probar es el bypass del pin 7 del LM386 (10 µF, de la aplicación típica de la hoja de datos).

Cualquiera de estos ajustes cambia §3.7: se registra primero en la spec.

## Preparación de hardware (laboratorio)
- Fuente de audio: el cargador con pinzas, con el negro a GND de la LPCXpresso y el rojo solo a la etapa de audio.
- Componentes de §3.7: LM358, LM386, 2 × 8,2 kΩ, 2,2 nF y 1 nF (C0G o film), 1 µF, 150 kΩ, pote de 10 kΩ log, 100 nF + 100 µF, 10 Ω + 47 nF, 220 µF, 2 × 100 Ω y jack estéreo de 3,5 mm. Además, 2 × 16 Ω (o 15 Ω) para la carga de prueba.
- Multímetro en DC y en AC, y en capacidad si tiene ese modo. Antes de medir, comprobar que en AC no marca la continua: los 5 V del cargador medidos en AC tienen que dar ≈ 0. Si no, medir en AC a través de un capacitor en serie.
- Prueba 1: el filtro armado y la LPCXpresso por USB con el firmware actual, midiendo DC y AC en AOUT y en la salida del LM358.
- Prueba 2: la etapa completa con la carga de 16 Ω y el multímetro en el jack.
- Prueba 3: auriculares.
