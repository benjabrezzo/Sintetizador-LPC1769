# Sintetizador digital polifónico con LPC1769

**Especificación funcional, decisiones de diseño y plan de
implementación incremental**

  -----------------------------------------------------------------------
  Campo                               Definición
  ----------------------------------- -----------------------------------
  Proyecto                            Sintetizador digital polifónico
                                      basado en LPC1769

  Plataforma de desarrollo            LPC1769 / Cortex-M3

  Aplicación auxiliar                 Editor y configurador en Python

  Salida de audio                     DAC del LPC1769, filtro de
                                      reconstrucción, amplificador y jack
                                      de 3,5 mm

  Visualización                       Una matriz LED CJMCU-64 de 8×8

  Estado del documento                Especificación inicial acordada

  Principios del proyecto             `docs/constitution.md`
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## 1. Descripción del proyecto

El proyecto consiste en desarrollar un sintetizador digital polifónico
de hasta cuatro voces, implementado sobre un microcontrolador NXP
LPC1769.

El sintetizador permitirá tocar ocho notas musicales mediante ocho
pulsadores. Cada voz tendrá un oscilador digital independiente, una
frecuencia, una fase y una envolvente ADSR propias. Las señales
generadas por las voces se combinarán mediante un mezclador digital y se
procesarán con un ecualizador de tres bandas antes de enviarse al DAC.

Una aplicación de escritorio desarrollada en Python permitirá diseñar
formas de onda mediante síntesis aditiva, visualizar sus características
y transferir las tablas de muestras al microcontrolador mediante UART.
También ofrecerá configuraciones precargadas que imitan instrumentos y
perillas para configurar el ecualizador, cuyo procesamiento se realiza en
el LPC1769.

El sistema también contará con un visualizador de espectro de audio.
Este analizará mediante FFT la señal generada por el sintetizador y
mostrará sus componentes frecuenciales en una matriz LED CJMCU-64 de
8×8: ocho columnas, una por banda, de 8 píxeles de alto.

El desarrollo se realizará de manera incremental. Cada bloque deberá
implementarse, probarse y aprobarse de forma independiente antes de
comenzar el siguiente. No se avanzará si el bloque actual no cumple sus
criterios de aceptación.

## 2. Objetivos funcionales

El sistema deberá permitir:

-   Reproducir hasta cuatro notas simultáneamente.
-   Detectar ocho pulsadores, correspondientes a ocho notas musicales.
-   Generar señales periódicas mediante tablas de onda.
-   Cambiar entre cinco formas de onda (cuadrada, diente de sierra,
    triangular, sinusoidal y la generada en Python) mediante un pulsador
    conectado a EINT1.
-   Controlar la frecuencia de cada voz de manera independiente.
-   Controlar la amplitud temporal de cada voz mediante una envolvente
    ADSR.
-   Mezclar las voces en una única señal digital.
-   Aplicar ecualización digital de graves, medios y agudos.
-   Convertir la señal digital a analógica mediante el DAC del LPC1769.
-   Entregar una señal apta para auriculares mediante una etapa
    analógica externa.
-   Diseñar formas de onda desde una aplicación Python.
-   Transferir tablas de onda y parámetros al LPC1769 mediante UART.
-   Visualizar el espectro de la señal de audio en una matriz LED.
-   Mantener la reproducción de audio estable mientras se actualiza la
    matriz y se realizan otras tareas.

## 3. Decisiones de diseño acordadas

Las siguientes decisiones se consideran requisitos de diseño. Cualquier
modificación posterior deberá registrarse y justificarse.

  -----------------------------------------------------------------------
  Área                                Decisión
  ----------------------------------- -----------------------------------
  Arquitectura de síntesis            DDS (Direct Digital Synthesis) con
                                      acumulador de fase

  Frecuencia de muestreo              32 kHz para empezar, marcados por
                                      `DACCNTVAL`; fija durante la
                                      reproducción, se podrán probar
                                      otras

  Polifonía                           Cuatro osciladores independientes y
                                      mezclador digital

  Entradas musicales                  Ocho pulsadores, para ocho notas,
                                      leídos por GPIO desde SysTick

  Formas de onda                      Cuatro precargadas (cuadrada, diente
                                      de sierra, triangular y sinusoidal)
                                      y una quinta generada en Python

  Tablas de onda                      256 muestras por ciclo; las cuatro
                                      precargadas en Flash, la quinta en
                                      RAM

  Selección de forma de onda          Un pulsador en EINT1; cada pulsación
                                      avanza a la onda siguiente, que se
                                      aplica desde la próxima nota

  Grabación de voz                    No se implementa

  Programación de periféricos         Drivers de
                                      `CMSISv2p00_LPC17xx/Drivers`,
                                      incluida la UART

  Periféricos utilizados              SysTick, Timer, ADC, DAC, GPDMA,
                                      UART, GPIO y EINT; no se usa SPI

  Envolventes                         ADSR independiente por voz,
                                      actualizada cada 1 ms por SysTick

  Controles ADSR                      Cuatro potenciómetros leídos
                                      mediante ADC

  Diseño de formas de onda            Síntesis aditiva en Python

  Aplicación de escritorio            Python con PySide6, numpy, pyserial
                                      y pyqtgraph

  Parámetros armónicos                Solo la amplitud de cada armónico;
                                      la cantidad se carga sola al elegir
                                      la frecuencia

  Instrumentos                        Guitarra, piano, órgano y flauta:
                                      configuraciones precargadas de
                                      armónicos, con indicación de cómo
                                      ajustar la ADSR

  Frecuencias armónicas               Múltiplos enteros de la frecuencia
                                      fundamental

  Transferencia de formas de onda     Python genera y cuantiza las
                                      muestras; el LPC1769 recibe la
                                      tabla mediante UART

  Evolución del timbre                Wavetable variable en el tiempo,
                                      como ampliación posterior

  Ecualizador                         Digital, ejecutado en el LPC1769;
                                      la aplicación solo lo configura

  Tipo de ecualizador                 Tres bandas: low-shelf, peaking EQ
                                      y high-shelf

  Implementación de filtros           Filtros IIR Biquad

  Aritmética DSP                      Punto fijo: muestras y envolventes
                                      en Q15; coeficientes Biquad en
                                      Q2.30, con acumulador de 64 bits

  Transferencia de audio              DMA con doble buffering

  Generación de muestras              En la ISR del DMA

  Organización del firmware           Loop principal simple, sin RTOS

  Salida digital-analógica            DAC del LPC1769

  Salida física                       Filtro Sallen-Key con LM358,
                                      amplificador LM386 y jack de 3,5 mm

  Visualizador                        Espectro de frecuencias calculado
                                      mediante FFT propia de 512 puntos

  Matriz LED                          Una CJMCU-64 de 8×8, con fuente
                                      externa

  Transmisión a las matrices          Timer + DMA

  Nivel de la línea de datos          Matriz a 4,5 V y GPIO directo a DIN
                                      con 330 Ω en serie

  Disposición física                  8 columnas por 8 filas, una
                                      columna por banda

  Actualización visual                Objetivo inicial de 20--30 FPS,
                                      sujeto a pruebas

  Comunicación PC-microcontrolador    UART

  Tests                               Lógica del firmware en la PC (gcc),
                                      app con pytest y hardware con
                                      mediciones en placa

  Forma de trabajo                    De a una story por vez, dentro de
                                      cada bloque (excepción: Regla de avance)
  -----------------------------------------------------------------------

### 3.1. Arquitectura DDS

Cada voz tendrá su propio acumulador de fase. La frecuencia se
determinará mediante el incremento aplicado al acumulador en cada
muestra.

Para un acumulador de fase de (M) bits:

\[ P\[n+1\] = (P\[n\] + `\Delta `{=tex}P) `\bmod 2`{=tex}\^M \]

\[ f = `\frac{\Delta P \cdot f_s}{2^M}`{=tex} \]

Donde:

-   \(P\) es la fase acumulada.
-   (`\Delta `{=tex}P) es el incremento de fase.
-   (f_s) es la frecuencia de muestreo.
-   \(f\) es la frecuencia de salida.

La fase de cada voz será independiente. La frecuencia de muestreo del
sistema, en cambio, permanecerá fija mientras se reproduce audio.

Se comenzará con (f_s) = 32 kHz, marcados por el contador del DAC:
`DACCNTVAL` = 25 MHz / 32 kHz ≈ 781. Se podrán probar otras frecuencias.

La posición de cada voz se guardará en formato Q16.16. Como las tablas
tienen 256 muestras, la posición da la vuelta en 2\^24 (M = 24) y el índice
de tabla son los bits 23:16. Los incrementos de las ocho notas se
precalculan en una tabla `const`.

### 3.2. Polifonía y mezcla

El sistema tendrá cuatro voces simultáneas. Cada voz mantendrá, como
mínimo:

-   Estado de activación.
-   Nota asignada.
-   Frecuencia o incremento de fase.
-   Acumulador de fase.
-   Identificador de tabla de onda.
-   Estado de la envolvente ADSR.
-   Nivel de amplitud.

El estado de activación y el nivel de amplitud de una voz salen de su
envolvente: la voz está libre cuando la envolvente terminó el Release
(sección 3.4).

Las cuatro señales se combinarán digitalmente antes de ingresar al
ecualizador. El control de niveles será el siguiente:

1.  La suma de voces se calculará en 32 bits, de modo que nunca desborde.
2.  La mezcla se escalará con una ganancia fija de 1/2.
3.  Después del ecualizador, la señal se saturará al rango de 10 bits del
    DAC (0 a 1023). Nunca habrá wrap-around.
4.  Un contador registrará las saturaciones para medirlas en las pruebas.

Con cuatro voces fuertes o un refuerzo alto del ecualizador se podrá
recortar la señal de vez en cuando. Esto se acepta a cambio de que una
nota sola use la mitad del rango del DAC, en lugar de una cuarta parte.

Cuando se presiona una nota, se le asigna una voz con este criterio:

1.  Una voz libre.
2.  Si no hay, la voz en Release con menor nivel.
3.  Si tampoco hay, la voz más antigua.

Si la tecla vuelve a sonar mientras su voz sigue activa (apretada o en
Release), se reutiliza esa misma voz.

Si la voz que toma la nota no estaba libre (la que se reutiliza, la que se
estaba apagando o la más antigua), la nota nueva sigue desde la fase y el
nivel que la voz tenía, para evitar clicks. Si estaba libre, arranca en
fase 0 y nivel 0. La nota toma la onda activa al arrancar (sección 3.8): si
la onda cambió, puede oírse un click.

### 3.3. Editor de formas de onda

La aplicación se desarrollará en Python con PySide6 (interfaz), numpy
(cálculo), pyserial (UART) y pyqtgraph (gráficos). Permitirá configurar
la amplitud de la fundamental y de sus armónicos:

\[ x(t) = `\sum`{=tex}\_{k=1}\^{N} A_k `\sin`{=tex}(2`\pi `{=tex}k f_0
t) \]

Donde (f_0) es la frecuencia fundamental y (A_k) la amplitud del
armónico (k).

La aplicación generará una tabla periódica de muestras, la normalizará y
la cuantizará al formato acordado con el firmware. La tabla se
transferirá al LPC1769 mediante UART.

El usuario solo editará amplitudes; la fase de los armónicos no es
configurable. Al seleccionar la frecuencia fundamental, la aplicación
cargará automáticamente los armónicos y mostrará la frecuencia de cada uno.

La misma tabla suena en las ocho teclas, de modo que el límite lo fija la
nota más aguda: para evitar aliasing en Do5 (523,25 Hz) a 32 kHz, se usarán
como máximo 30 armónicos (30 × 523,25 Hz = 15,7 kHz \< 16 kHz). Si cambia la
frecuencia de muestreo, el máximo se recalcula con la misma regla.

La cantidad de armónicos utilizables deberá respetar las limitaciones de
muestreo y aliasing. En una etapa posterior se podrá incorporar un
conjunto de tablas limitadas en banda para diferentes rangos de notas.

La tabla generada será la quinta forma de onda del sintetizador. No
reemplaza a las cuatro precargadas.

**Editor (story 11).** La fundamental se elige entre las ocho notas de las
teclas (Do4 a Do5, tabla de la sección 3.11) y arranca en La4. Los armónicos
2 a 30 se muestran siempre, cada uno con su frecuencia en Hz: al cambiar de
nota cambian solo las frecuencias; las amplitudes y la tabla no cambian.
Las amplitudes van de 0 a 100 %, sin signo. Al abrir, la fundamental está al
100 % y el resto en 0 (senoide).

El pico de la tabla es el de la muestra de mayor valor absoluto: esa muestra
vale ±32767, igual que en las ondas precargadas. El espectro se calcula sobre
la tabla cuantizada (lo que va a sonar) y se muestra en porcentaje del pico
de escala. Un armónico con amplitud cero queda por debajo de 1 LSB (≈ 0,003 %
del pico de escala); el piso de cuantización medido es de 0,31 LSB.

La senoide y la cuadrada (armónicos impares con 1/k) de la aplicación
coinciden con `sineTable` y `squareTable` de `firmware/src/tables.c`, con
tolerancia de 1 LSB.

El archivo exportado es texto con 256 enteros, uno por línea, que arrancan en
cero y están entre −32767 y 32767. Una onda con todas las amplitudes en cero
no se exporta, y la aplicación avisa por qué.

La interfaz usa el estilo clásico de Windows que trae Qt, con su paleta
forzada para que no la cambie el tema claro u oscuro del escritorio.

**Instrumentos (story 12).** La aplicación incluye cuatro configuraciones
precargadas: guitarra, piano, órgano y flauta. Cada una carga en el editor
las 30 amplitudes de la tabla de abajo y muestra un mensaje que dice cómo
dejar los cuatro potenciómetros de la ADSR para acercarse a su sonido. Se
eligen con botones, que son una acción: el editor no recuerda cuál fue el
último instrumento. El mensaje queda visible hasta elegir otro. Cargar un
instrumento no cambia la nota, y la app solo da indicaciones: no cambia la
ADSR de la placa.

Las amplitudes salen de un modelo físico simple, normalizado a la
fundamental (100 %) y sin fase, y van en % enteros:

-   **Guitarra:** cuerda pulsada a 1/5 del largo, A_k = |sen(kπ/5)| / k².
-   **Piano:** cuerda percutida a 1/7 del largo, A_k = |sen(kπ/7)| / k.
-   **Órgano:** tiradores 8', 4', 2⅔' y 2' al máximo: armónicos 1 a 4 al
    100 % y el resto en 0.
-   **Flauta:** casi senoidal: armónicos 1, 2 y 3 al 100 %, 20 % y 5 %; el
    resto en 0.

Redondeo: al entero más cercano y, en un empate, hacia arriba. Se calcula
con fracciones exactas. El único empate es el piano en k = 8 (12,5 % → 13 %).

| Armónico | Guitarra | Piano | Órgano | Flauta |
|---:|---:|---:|---:|---:|
| 1 | 100 | 100 | 100 | 100 |
| 2 | 40 | 90 | 100 | 20 |
| 3 | 18 | 75 | 100 | 5 |
| 4 | 6 | 56 | 100 | 0 |
| 5 | 0 | 36 | 0 | 0 |
| 6 | 3 | 17 | 0 | 0 |
| 7 | 3 | 0 | 0 | 0 |
| 8 | 3 | 13 | 0 | 0 |
| 9 | 1 | 20 | 0 | 0 |
| 10 | 0 | 22 | 0 | 0 |
| 11 | 1 | 20 | 0 | 0 |
| 12 | 1 | 15 | 0 | 0 |
| 13 | 1 | 8 | 0 | 0 |
| 14 | 1 | 0 | 0 | 0 |
| 15 | 0 | 7 | 0 | 0 |
| 16 | 0 | 11 | 0 | 0 |
| 17 | 1 | 13 | 0 | 0 |
| 18 | 0 | 12 | 0 | 0 |
| 19 | 0 | 9 | 0 | 0 |
| 20 | 0 | 5 | 0 | 0 |
| 21 | 0 | 0 | 0 | 0 |
| 22 | 0 | 5 | 0 | 0 |
| 23 | 0 | 8 | 0 | 0 |
| 24 | 0 | 9 | 0 | 0 |
| 25 | 0 | 9 | 0 | 0 |
| 26 | 0 | 7 | 0 | 0 |
| 27 | 0 | 4 | 0 | 0 |
| 28 | 0 | 0 | 0 | 0 |
| 29 | 0 | 3 | 0 | 0 |
| 30 | 0 | 6 | 0 | 0 |

Las posiciones de los potenciómetros se dan con palabras, aproximadas. Para
un tiempo de t ms, la fracción del recorrido sale de `envelope.c`:
(50 + (t − 2) · 3995 / 1998) / 4095, que es ≈ (2t + 46) / 4095. Escala de
palabras: al mínimo (≈ 0 %), un poco más del mínimo (≈ 5 %), un décimo
(≈ 10 %), un sexto (≈ 17 %), tres cuartos (≈ 75 %), cuatro quintos (≈ 80 %)
y al máximo (≈ 100 %).

Mensajes (cada línea nombra un pote y su valor):

-   **Guitarra:**
    - Attack: al mínimo (≈ 2 ms).
    - Decay: tres cuartos del recorrido (≈ 1500 ms).
    - Sustain: al mínimo (nivel 0 %).
    - Release: un décimo del recorrido (≈ 200 ms).
-   **Piano:**
    - Attack: al mínimo (≈ 2 ms).
    - Decay: al máximo (2000 ms).
    - Sustain: al mínimo (nivel 0 %).
    - Release: un sexto del recorrido (≈ 300 ms).
-   **Órgano:**
    - Attack: al mínimo (≈ 2 ms).
    - Decay: en cualquier posición (con Sustain al 100 % no suena distinto).
    - Sustain: al máximo (nivel 100 %).
    - Release: al mínimo (≈ 2 ms).
-   **Flauta:**
    - Attack: un poco más del mínimo (≈ 100 ms).
    - Decay: un décimo del recorrido (≈ 200 ms).
    - Sustain: cuatro quintos del recorrido (nivel 80 %).
    - Release: un décimo del recorrido (≈ 150 ms).

### 3.4. Envolvente ADSR

Cada voz tendrá una envolvente propia con cuatro etapas:

-   **Attack:** tiempo de crecimiento desde el nivel inicial hasta el
    máximo.
-   **Decay:** transición desde el máximo hasta el nivel de sustain.
-   **Sustain:** nivel mantenido mientras la tecla continúe presionada.
-   **Release:** caída desde el nivel actual hasta cero al soltar la
    tecla.

Los cuatro potenciómetros controlarán los parámetros globales de Attack,
Decay, Sustain y Release. Cada voz mantendrá su propio estado temporal
de envolvente.

SysTick actualizará las envolventes cada 1 ms. Attack, Decay y Release se
mapearán linealmente a un rango de 2 a 2000 ms, y Sustain a un nivel de 0 a
100 %. Las puntas del recorrido de cada potenciómetro tienen un margen, para
que el giro real alcance los extremos.

El comportamiento ante las teclas será el siguiente:

-   Al soltar la tecla, en cualquier etapa, se pasa a Release desde el
    nivel actual.
-   Al volver a tocarla, se pasa a Attack desde el nivel actual, para evitar
    clicks.

Las rampas son lineales y cada etapa dura su tiempo completo desde el
nivel en que arranca: soltar la tecla a mitad del Attack igual tarda lo que
dura el Release en apagarse, y volver a tocarla desde un nivel bajo igual
tarda lo que dura el Attack en llegar al máximo. Con Sustain en 0 % y la
tecla apretada, la voz sigue ocupada (en silencio) hasta que se suelta; queda
libre recién cuando termina el Release.

Los potenciómetros pueden moverse mientras una nota suena. Si cambia el
tiempo de Attack, Decay o Release a mitad de una etapa, se conserva lo ya
recorrido: el nivel sigue desde donde estaba y la etapa termina lo que le
falta a la velocidad nueva. Por ejemplo, un Attack de 1000 ms que va por el
40 % y pasa a 2000 ms tarda 1200 ms más en llegar al máximo. Un cambio de
Sustain lleva hacia el nivel nuevo tanto a una nota sostenida como a una que
está en Decay.

El ADC funcionará en modo burst sobre los cuatro canales de los
potenciómetros, y SysTick leerá sus resultados.

### 3.5. Ecualizador

El ecualizador se ejecutará en el LPC1769, después de la mezcla de voces
y antes del DAC.

Tendrá tres bandas:

-   Low-shelf: graves, a 200 Hz.
-   Peaking EQ: medios, a 1 kHz.
-   High-shelf: agudos, a 4 kHz.

La ganancia de cada banda irá de −12 a +6 dB, porque por encima de +6 dB una
nota sola ya saturaría con la ganancia de mezcla de 1/2. Al iniciar, las
tres bandas estarán en 0 dB.

Cada banda se implementará mediante un filtro IIR Biquad. La aplicación
Python solo configurará los parámetros, mediante perillas de frecuencias
bajas, medias y altas. La aplicación calculará los coeficientes de los
Biquad y los enviará por UART, de modo que el LPC1769 no necesite cálculo en
punto flotante; el procesamiento de las muestras se realizará en el
microcontrolador. Los coeficientes nuevos se aplicarán entre bloques de
audio.

El factor Q es 1/√2 en los dos shelves (equivale a una pendiente de shelf
S = 1) y 1 en el peaking. Con esos valores y Fs = 32,01 kHz, los
coeficientes de las tres bandas entran en Q2.30 en todo el rango de
−12 a +6 dB: el más justo es el del low-shelf a +6 dB, con |a1| = 1,953
contra el límite de 2. La aplicación valida que cada coeficiente entre en
Q2.30 antes de enviarlo.

La aplicación no analizará sonidos existentes.

### 3.6. Visualizador LED

Una matriz CJMCU-64 formará una superficie de 8 columnas por 8 filas, con
una columna por banda. Al principio eran dos matrices en cascada (16×8 y
16 bandas); el 2026-11-14 decidimos usar una sola.

El visualizador analizará la señal digital posterior al ecualizador y
anterior al DAC:

1.  Obtener un bloque de muestras.
2.  Aplicar una ventana Hann.
3.  Calcular la FFT.
4.  Agrupar los bins en 8 bandas de frecuencia.
5.  Calcular la magnitud de cada banda.
6.  Aplicar escala, normalización y suavizado.
7.  Convertir la magnitud en una altura de entre 0 y 8 píxeles.
8.  Actualizar las matrices.

La actualización del visualizador será una tarea secundaria. Nunca
deberá bloquear la generación de audio ni retrasar la transferencia DMA
al DAC.

La FFT será de implementación propia: radix-2 de 512 puntos en Q15, con
tablas de factores de giro y de ventana precalculadas en Flash. Con 32 kHz,
cada bin mide 62,5 Hz.

Las 8 bandas son octavas desde 62,5 Hz hasta 16 kHz: como 16 kHz / 62,5 Hz
= 2^8, son exactamente ocho y ninguna queda sin bins. El bin 0 (continua)
no entra en ninguna.

  Banda   Rango             Bins
  ------- ----------------- ---------
  1       62,5--125 Hz      1
  2       125--250 Hz       2--3
  3       250--500 Hz       4--7
  4       500 Hz--1 kHz     8--15
  5       1--2 kHz          16--31
  6       2--4 kHz          32--63
  7       4--8 kHz          64--127
  8       8--16 kHz         128--255

Con 256 puntos, el bin mediría 125 Hz y la banda 1 quedaría vacía. Las
fundamentales de Do4 a Si4 caen en la banda 3 y Do5, en la 4: las teclas
no se separan por columna, y lo que se ve es el timbre. La ventana Hann
reparte la energía de un tono en unos ±2 bins, así que una nota cerca de
un borde también sube la banda vecina; la banda más alta es la del bin más
cercano a la frecuencia (por ejemplo, Si4, a 493,9 Hz, cae en el bin 8 y
sube sobre todo la banda 4).

Los LED encendidos toman el color de la onda seleccionada con el pulsador
(sección 3.8), la que van a usar las próximas notas: rojo la senoide, azul
la cuadrada, verde la triangular, amarillo la sierra (rojo + verde) y
morado la de Python (rojo + azul). Todas las barras van del mismo color y
la altura muestra el nivel; no hay paleta por intensidad. Si siguen
sonando notas con la onda anterior, el color igual cambia al pulsar. El
límite de brillo del 25 % vale también para el amarillo y el morado, que
encienden dos canales: el peor caso sigue siendo el blanco.

La ISR de audio copiará las muestras a un buffer de captura separado. Cuando
ese buffer se llene, dejará de escribirlo hasta que el loop principal
termine de procesarlo.

Decidimos para el Bloque 11 (2026-11-14):

- **Muestras capturadas:** las que van al DAC, ya saturadas a 10 bits (el
  valor sin el desplazamiento de 512), llevadas a Q15. Así el análisis ve lo
  que se oye, recorte incluido.
- **Cuadros:** 512 muestras consecutivas, sin solapamiento. Llenar el buffer
  lleva 8 bloques (16 ms) y el análisis tiene que caber en el resto de los
  50 ms de un cuadro: mínimo de 20 cuadros por segundo.
- **Magnitud de banda:** el máximo de las magnitudes de sus bins. La magnitud
  de un bin se normaliza para que una senoide centrada en él dé su amplitud,
  con 1,0 a fondo de escala del DAC.
- **Escala:** logarítmica, con un nivel por cada 6 dB (cada vez que la
  magnitud se duplica). El nivel 8 es una magnitud de al menos 1/2 del fondo de
  escala (−6 dB) y el nivel 1, de al menos 1/256 (−48 dB); por debajo, 0.
- **Suavizado:** la barra sube de inmediato al nivel nuevo y, si el nivel
  baja, cae un solo nivel por cuadro. Con el audio cortado, las barras bajan
  hasta 0 en como máximo 8 cuadros.

Decidimos para el Bloque 13 (2026-11-14):

- **Caída en pantalla:** el análisis entrega niveles a unos 55 cuadros por
  segundo (story 15) y la matriz se actualiza a 25, así que la caída del
  análisis (un nivel por cuadro, cada ≈ 18 ms) se vería como unos 2 niveles
  por cuadro de pantalla. Para que se vea fluida, la barra que se muestra es
  el mayor entre el nivel nuevo y la barra mostrada menos `BARS_FALL`, con
  `BARS_FALL` = 1 nivel por cuadro de pantalla (40 ms a 25 FPS): una barra
  tarda como máximo 320 ms en bajar de 8 a 0. Esta etapa se suma a la del
  análisis, no la reemplaza, y su constante se puede ajustar en el
  laboratorio. La subida sigue siendo inmediata.
- **Dibujo:** una columna por banda (la columna 0 es la banda 1, de 62,5 a
  125 Hz) y una barra de altura h enciende h píxeles desde la fila de abajo
  hacia arriba.
- **Color en dos pasos:** la story 17 pinta las barras de rojo fijo y la
  story 18 les da el color de la onda seleccionada.

La transmisión a las matrices se hará con Timer + DMA, sin SPI. El Timer
genera las peticiones de DMA, y el DMA copia un buffer ya codificado al
registro del pin de datos (P0.0). Las matrices se alimentarán con una fuente
externa regulable, ajustada a 4,5 V (§3.15), con masa común con el LPC1769.

La fuente no define el límite de consumo: 64 LEDs en blanco al máximo
consumirían unos 3,8 A (a 60 mA por LED), una corriente excesiva para el
cableado y las pistas de la matriz. El firmware limitará el brillo al
25 %, lo que deja el peor caso en alrededor de 1 A. Los límites de seguridad,
tras 5 minutos con los 64 LED en blanco al 25 %, son: corriente ≤ 1,1 A,
tensión en la matriz ≥ 4,0 V bajo carga, y dorso de la matriz y cables que se
puedan tocar 10 s sin quemar. La tensión de la fuente se ajusta y se comprueba
(con su pantalla o con un multímetro) antes de conectar la matriz, y su
límite de corriente se fija en ≈ 1,5 A para que no oculte la medición de
1,1 A.

### 3.7. Salida de audio

La salida se organizará de la siguiente manera:

\[ `\text{DAC}`{=tex}
`\rightarrow `{=tex}`\text{Filtro de reconstrucción}`{=tex}
`\rightarrow `{=tex}`\text{Amplificador de auriculares}`{=tex}
`\rightarrow `{=tex}`\text{Jack 3,5 mm}`{=tex} \]

El DAC no se conectará directamente a los auriculares. La etapa
analógica deberá contemplar el nivel de continua, el acoplamiento, la
impedancia de carga y los niveles seguros de salida.

El diseño inicial, a validar en el Bloque 3, es el siguiente:

``` text
AOUT --- 8,2 kΩ --+-- 8,2 kΩ --+-- LM358 (+)   salida ---+--- (-) LM358
                  |            |                         |
                2,2 nF        1 nF                       |
          (a salida LM358)   (a GND)                     |
                                                         |
  salida LM358 --- 1 µF --- 150 kΩ --- pote 10 kΩ log --- LM386 (pin 3)

  LM386 (pin 5) --- 220 µF --+-- 100 Ω --- L
                             +-- 100 Ω --- R
```

  Etapa            Componentes                Función
  ---------------- -------------------------- --------------------------------
  Filtro           LM358, 2 × 8,2 kΩ, 2,2 nF  Sallen-Key de 2.º orden, con
                   y 1 nF                     fc ≈ 13 kHz y ganancia unitaria
  Acoplamiento     1 µF                       Quita los 1,65 V de continua
  Atenuación y     150 kΩ y potenciómetro de  Baja los 3,3 Vpp del DAC a unos
  volumen          10 kΩ logarítmico          170 mVpp como máximo
  Amplificador     LM386 con ganancia 20,     Maneja la carga de los
                   100 nF + 100 µF en         auriculares, con unos 3,4 Vpp
                   alimentación               como máximo
  Red de salida    10 Ω + 47 nF de pin 5 a    Estabilidad, según la aplicación
                   GND                        típica del LM386
  Salida           220 µF y 100 Ω por canal   Bloquea la continua; deja unos
                                              0,8 Vpp en auriculares de 32 Ω

**Por qué se hace así:**

-   **LM358 a 5 V:** su entrada y su salida funcionan desde 0 V hasta unos
    3,5 V, lo que cubre el rango del DAC sin una fuente negativa.
-   **Carga del DAC:** la primera resistencia de 8,2 kΩ respeta el mínimo de
    1 kΩ de la hoja de datos.
-   **LM386:** a diferencia del LM358, está pensado para manejar cargas de
    baja impedancia y se consigue con facilidad.
-   **100 Ω por canal:** los auriculares son muy sensibles. Estas resistencias
    limitan el nivel a unos 2,6 mW y separan los dos canales, que reciben la
    misma señal.

**Alimentación.** El LM358 y el LM386 usarán 5 V de una línea distinta de la
de las matrices, porque los LED generan ruido en su alimentación. En el
Bloque 3 esa línea es un cargador de 5 V aparte, con su negro unido a GND de
la LPCXpresso y su rojo conectado solo a la etapa de audio. El retorno de
corriente de las matrices irá
directamente a la fuente, y no por la masa de la etapa de audio; ambas se
unirán en un solo punto.

**Validación (Bloque 3).** Se prueba con la senoide de 125 Hz de escala
completa del DAC (3,36 a 3,40 Vpp) y con el multímetro, sin osciloscopio.

-   **Filtro:** con los 5 V de audio, la salida tiene la misma amplitud que la
    entrada (±1 dB, medida en AC). La atenuación a 32 kHz no se mide: se
    calcula con los valores verificados de los componentes montados y debe dar
    al menos 12 dB (diseño: −15,5 dB; peor caso con resistencias al 5 % y
    capacitores al 10 %: ≈ −12,9 dB). Se aprueba con esa evidencia indirecta
    por no contar con osciloscopio; la decisión se fecha en
    `docs/evidencia/`. Si el osciloscopio mide después y contradice el
    cálculo, se corrige antes de seguir.
-   **Carga de prueba:** 16 Ω en cada canal del jack. Con el volumen al máximo,
    cada canal mide entre 0,12 y 0,21 Vrms (diseño: ≈ 0,17 Vrms, unos
    0,47 Vpp) y menos de 10 mV de continua, medidos con la carga puesta.
-   **Auriculares:** del mínimo al máximo el tono sube gradualmente, sin saltos
    ni raspado, parejo en los dos lados y sin distorsión audible.

Los capacitores de 2,2 nF y 1 nF deben ser C0G/NP0 o de film, con su
tolerancia anotada. Cualquier ajuste de valores se registra aquí antes de
cambiar el circuito.

**Scripts de cálculo (Bloque 3).** En `hardware/tools/` hay dos scripts de
Python, con sus tests en `hardware/tests/`. No son una simulación del
circuito: aplican las fórmulas del circuito ideal con los valores de los
componentes. Los tests comprueban que cada script reproduce los números de
diseño de este documento antes de usarlo con otros valores. Cumplen dos
funciones distintas:

-   **`etapa.py` (`output_vrms`): predice lo que debería medirse.** Calcula
    los Vrms sobre la carga de un canal con el volumen al máximo, con el
    divisor del pote, el LM386 de ganancia 20 y las 100 Ω en serie, sin
    recorte. Sirve de referencia para las mediciones con el multímetro:
    si la medición se aleja mucho del cálculo, algo está mal en el armado.
    No modela el recorte ni el ancho de banda de los chips, así que decide
    la medición y no el cálculo.

    | Caso (carga de 16 Ω, volumen al máximo) | Calculado | Medido |
    |---|---|---|
    | DAC de 3,36 Vpp | 0,1725 Vrms | — |
    | DAC de 3,40 Vpp | 0,1745 Vrms | — |
    | DAC medido con multímetro, 3,26 Vpp | 0,1671 Vrms | 0,168 Vrms (L) y 0,169 Vrms (R) |

    Las dos primeras filas usan el Vpp del osciloscopio, que incluye picos
    finos (story 2). El multímetro mide la senoide en 1,151 Vrms (≈ 3,26 Vpp),
    así que la medición se compara con la tercera fila, calculada con ese
    valor y los componentes nominales. Las mediciones cumplen lo esperado:
    0,12 a 0,21 Vrms y menos de 10 mV de continua (3,6 y 5 mV).

-   **`filtro.py` (`attenuation_db`): reemplaza una medición que no se
    puede hacer.** El criterio del filtro pide al menos 12 dB de
    atenuación a 32 kHz, que es donde cae la primera imagen de muestreo.
    No hay forma de medirla: el multímetro no responde a esa frecuencia y no
    se dispone de osciloscopio. Por eso el cálculo, con los valores
    verificados de los componentes montados, es la evidencia del criterio
    (evidencia indirecta). Resultados del script con valores de diseño:

    | Caso | Atenuación a 32 kHz |
    |---|---|
    | Nominal | −15,52 dB |
    | Peor caso (R −5 %, C −10 %) | −12,86 dB |

    El cálculo con los valores medidos de los componentes montados y la
    decisión fechada se guardan en `docs/evidencia/`. Si más adelante se mide
    con osciloscopio y contradice el cálculo, se corrige antes de seguir.

### 3.8. Selección de forma de onda

El sintetizador contará con cinco formas de onda. Para cambiar entre
ellas se utilizará un pulsador conectado a EINT1. Cada pulsación avanzará a
la onda siguiente, en este orden cíclico:

1.  Cuadrada.
2.  Diente de sierra.
3.  Triangular.
4.  Sinusoidal.
5.  Onda generada en Python.

Después de la última se vuelve a la cuadrada. Mientras no se haya recibido
una onda desde Python, la rotación saltea la quinta posición. La quinta
entra en la rotación con la primera onda válida (una trama completa con el
checksum correcto, sección 3.14) y queda ahí hasta reiniciar la placa; una
onda nueva reemplaza a la anterior sin cambiar el orden. La placa arranca en
la onda sinusoidal, así que la primera pulsación lleva a la cuadrada.

Una pulsación mantenida cuenta una sola vez: la onda no vuelve a cambiar
hasta que se suelta el pulsador (sección 3.11).

El cambio se aplicará desde la próxima nota: las voces que ya están
sonando conservan su onda hasta terminar.

La cuadrada, el diente de sierra y la triangular se generan limitadas en
banda, sumando senoides hasta el armónico 30, con la misma regla de la
sección 3.3 (30 × 523,25 Hz = 15,7 kHz \< 16 kHz):

  Onda               Armónicos   Amplitud del armónico k
  ------------------ ----------- ------------------------------------
  Cuadrada           impares     1/k
  Diente de sierra   todos       (−1)^(k+1)/k: la onda sube desde cero
  Triangular         impares     (−1)^((k−1)/2)/k²

Las tres arrancan en cero, como la senoide, y se normalizan para que su pico
valga exactamente el máximo de Q15 (32767): el rizado de Gibbs de la suma
no puede desbordar.

Hay una excepción: cuando llega una tabla nueva desde Python, se valida en un
buffer separado y el puntero de la quinta onda cambia entre bloques de audio.
Las voces que están usando esa onda pasan a la tabla nueva, y se acepta que
pueda oírse un click. Así, nunca se escribe sobre la tabla que está sonando.

No se implementará la grabación de voz.

### 3.9. Uso de drivers

Los periféricos del LPC1769 se programarán mediante los drivers ubicados
en `CMSISv2p00_LPC17xx/Drivers`. Esto incluye la UART, que cuenta con su
driver en `lpc17xx_uart.c`.

Solo se usarán SysTick, Timer, ADC, DAC, GPDMA, UART, GPIO y EINT (PINSEL,
NVIC y CLKPWR, como configuración). Se accederá directamente a un registro
únicamente donde el driver no cubra lo necesario, con un comentario que lo
justifique.

### 3.10. Distribución de memoria

  Región                  Contenido                     Motivo
  ----------------------- ----------------------------- -----------------------
  Flash                   Las cuatro tablas precargadas Son `const`: no ocupan
                          (2 KB, `int16_t`), tablas de  RAM y no se pierden al
                          incrementos, ventana Hann     reiniciar
  `RamLoc32` (32 KB)      Pila, voces, filtros, FFT,    Acceso directo de la
                          buffer UART y quinta onda     CPU, sin competir con
                          (activa + recepción, 1 KB)    el DMA
  AHB SRAM banco 0        Buffers de audio al DAC       Lo lee el canal DMA del
  (16 KB)                 (512 B)                       audio
  AHB SRAM banco 1        Buffer de la matriz           Puerto separado del
  (16 KB)                 (\~4,6 KB)                    banco 0: los dos canales
                                                        DMA no compiten

La quinta onda está en RAM porque llega en funcionamiento y escribir la
Flash detendría la ejecución; por eso se pierde al reiniciar. El proyecto
ve los dos bancos como una sola región (`RAM2`), de modo que ubicar cada
buffer en su banco requiere ajustar la configuración de memoria.

La FFT usará unos 3 KB de `RamLoc32`: 1 KB de captura y 2 KB de trabajo
(parte real e imaginaria). Sus tablas de factores de giro y de ventana
ocuparán unos 2 KB de Flash.

Las tablas de Flash (ondas, incrementos, ventana y factores de giro) se
generarán con `firmware/tools/gen_tables.py` y no se editarán a mano.

### 3.11. Lectura de pulsadores

Las ocho teclas se leerán por GPIO desde SysTick, cada 1 ms, sin
interrupciones de GPIO. Así, los rebotes no generan ráfagas de
interrupciones que compitan con el audio. Una tecla cambia de estado tras
10 ms estable, y ese cambio genera la pulsación o la liberación.

Se descartan las interrupciones de GPIO (los puertos 0 y 2 comparten el
vector de `EINT3`) por cuatro motivos:

-   El antirrebote necesita medir 10 ms igual, así que haría falta un Timer o
    SysTick: la interrupción solo avisaría del flanco, y el rebote seguiría
    llegando como una ráfaga que hay que filtrar.
-   Cada flanco del rebote costaría una entrada y salida de ISR que compite
    con el audio. Por sondeo el costo es fijo: una lectura por milisegundo,
    haya o no rebote.
-   No ahorra nada: SysTick ya corre cada 1 ms para los potenciómetros y las
    envolventes, y el audio interrumpe cada 2 ms, así que la CPU no duerme.
-   La latencia que suma el sondeo es de hasta 1 ms, contra los 10 ms del
    antirrebote.

EINT1 (la selección de onda, en P2.11) tiene su propio vector, así que
`EINT3` queda sin usar.

Las notas son la escala de Do mayor, de Do4 a Do5:

  Tecla   1        2        3        4        5        6        7        8
  ------- -------- -------- -------- -------- -------- -------- -------- --------
  Nota    Do4      Re4      Mi4      Fa4      Sol4     La4      Si4      Do5
  Hz      261,63   293,66   329,63   349,23   392,00   440,00   493,88   523,25

**Pulsador de EINT1.** Un flanco descendente de P2.11 interrumpe y cuenta
como pulsación solo si se cumplen las dos condiciones:

-   Pasaron al menos 150 ms desde la última pulsación aceptada. Se miden con
    un contador de milisegundos que incrementa SysTick (resolución de 1 ms)
    y siempre desde la última aceptada: los flancos ignorados no extienden la
    ventana.
-   El pulsador se soltó desde esa pulsación. SysTick lee P2.11 por GPIO cada
    1 ms y lo da por soltado cuando lleva 10 ms seguidos en alto, como el
    antirrebote de las teclas.

Así, una pulsación mantenida cuenta una sola vez, dure lo que dure, y el
rebote al soltar no genera una pulsación nueva. Al arrancar el pulsador está
listo: la primera pulsación se acepta. Un pulsador ya apretado al encender
no genera flanco y no cuenta. Los flancos que no cumplen las condiciones se
ignoran, y la ISR limpia el flag de EINT1 antes de salir de todos modos.

### 3.12. Prioridades de interrupción

Un número menor indica mayor prioridad.

  Interrupción        Prioridad   Motivo
  ------------------- ----------- --------------------------------------------
  `DMA_IRQn`          0           Si se atrasa, se produce un corte audible
  `SysTick_IRQn`      1           Envolventes, potenciómetros y teclas cada 1 ms
  `UART0_IRQn`        2           La FIFO es de 16 B
  `EINT1_IRQn`        4           Evento humano, sin urgencia

El Timer de las matrices no usa interrupción: solo genera peticiones de
DMA. El canal DMA 0 es del audio y el canal 1, de las matrices, ya que el
canal de menor número tiene prioridad.

La ISR del DMA generará cada bloque de audio: voces, mezcla, ecualizador y
copia de muestras para la FFT. Es la ISR más larga del sistema, pero su
duración está acotada, y así el audio no depende de las tareas del loop
principal. Se estimaban unos 20 mil ciclos cada 2 ms (cerca del 10 % de la
CPU), y la medición dio más. Con cuatro voces, ecualizador y captura la ISR
dura 40 560 ciclos (el 20,3 % del bloque de 199 936) en Debug con `-O1`, y
unos 127 000 (el 64 %) en Debug sin optimizar: por eso el Debug del proyecto
compila con `-O1`. La medición está en
`docs/evidencia/story-15-linea-base-isr.md`.

El loop principal, sin RTOS, atenderá en este orden el parser de UART, la FFT
y la codificación de las matrices.

### 3.13. Asignación de pines

  Función                      Pin
  ---------------------------- --------------------------------------------
  Teclas Do4 a Do5             P2.0 a P2.7, con pull-up, activas en bajo
  Selección de onda (EINT1)    P2.11, flanco descendente, con pull-up
  Attack, Decay, Sustain       AD0.0 P0.23, AD0.1 P0.24, AD0.2 P0.25
  Release                      AD0.4 P1.30 (AD0.3 comparte pin con el DAC)
  Salida del DAC               AOUT P0.26
  UART0                        TXD0 P0.2, RXD0 P0.3, adaptador USB-TTL de 3,3 V
  Datos de las matrices        P0.0, con 330 Ω en serie (`FIOPIN0` y `FIOMASK`)
  LED de la placa              P0.22, salida (prueba de vida del Bloque 0)
  Medición de la ISR de audio  P2.8, salida; sube mientras se genera un bloque (Bloque 2)

### 3.14. Protocolo UART

La comunicación usará UART0 a 115200 8N1, con recepción por interrupción
hacia un buffer circular de 256 B. Los paquetes se procesarán fuera de la
ISR. El detalle se documentará en `docs/protocolo.md`.

Formato de trama: `0xA5 | CMD | LEN_L | LEN_H | payload | CHK`, donde CHK
es la suma de 8 bits de CMD a payload, en complemento a dos. La suma incluye
CMD, LEN_L, LEN_H y el payload, de modo que un largo corrupto también rompe
el checksum: la suma de todos esos bytes más CHK da 0 módulo 256.

Una trama UPLOAD_WAVE con LEN distinto de 512 o con el checksum incorrecto se
descarta sin responder y no modifica la onda activa. No hay NACK: la
aplicación espera el ACK y trata cualquier otra respuesta, o el silencio,
como una falla (Bloque 9).

El parser decide por CMD. Una trama SET_EQ con LEN distinto de 60 o con el
checksum incorrecto se descarta igual, sin responder y sin modificar el
ecualizador activo, y lo mismo vale para un CMD desconocido. Al aceptar
una SET_EQ la placa responde ACK y aplica los coeficientes al empezar un
bloque. Al arrancar, las tres bandas están en 0 dB.

  CMD      Mensaje         Dirección   Contenido
  -------- --------------- ----------- -------------------------------------
  `0x01`   UPLOAD_WAVE     PC a LPC    256 muestras `int16_t` (512 B)
  `0x04`   SET_EQ          PC a LPC    5 coeficientes Q2.30 por banda (60 B)
  `0x06`   ACK             LPC a PC    Trama aceptada

### 3.15. Nivel de la línea de datos

El WS2812B toma como alto en DIN una tensión de al menos 0,7 × VDD (VIH) y
como bajo, hasta 0,3 × VDD (VIL), con 0,35 V de histéresis típica. La hoja de
datos garantiza esos umbrales con VDD entre 4,5 y 5,5 V. Con 5 V el alto
exige 3,5 V y un GPIO del LPC1769 entrega 3,3 V como máximo, de modo que
quedaría por debajo del umbral garantizado. Con VDD = 4,5 V el umbral es de
3,15 V y los 3,3 V del GPIO lo superan por unos 0,15 V. Por eso la matriz se
alimenta a 4,5 V y el GPIO se conecta a DIN por una resistencia de 330 Ω, sin
etapa adaptadora.

``` text
GPIO LPC1769 (P0.0) --- 330 Ω --- DIN de la matriz
Fuente regulable (4,5 V) ------- +5V de la matriz   (masa común: GND)
```

**Conexiones.** P0.0 va a DIN por los 330 Ω, con la resistencia lo más cerca
posible de la matriz. La fuente va a los pads +5V y GND del lado de entrada
(DIN, +5V y GND); DOUT no se usa. La masa es común entre la fuente, el
LPC1769 y la matriz.

**Tensión de la fuente.** Se ajusta a 4,5 V, el mínimo de las condiciones en
que la hoja de datos garantiza los umbrales. Si en la prueba DIN no
registra, se baja la fuente de a pasos hasta un mínimo de 4,0 V. Ese valor
está dentro del rango de alimentación de la hoja de datos (+3,5 a +5,3 V),
pero fuera de las condiciones en que garantiza los umbrales (4,5 a 5,5 V):
ahí el umbral de 0,7 × VDD es una extrapolación, y la evidencia lo anota.
Con VDD bajo, el verde y el azul
(caída directa de 3,0 a 3,4 V) pueden salir más tenues y correr el tono de
los colores mezclados, algo que se revisa en la prueba de colores.

**Por qué se hace así:**

-   **330 Ω en serie:** la entrada admite entre −0,5 V y VDD + 0,5 V. Con la
    matriz apagada, DIN solo tolera 0,5 V y un GPIO en alto lo excede. Por eso
    las matrices se alimentan antes de transmitir y el pin queda en bajo
    cuando no hay tramas. La resistencia limita la corriente por el diodo de
    protección si eso falla, y con los 15 pF de entrada de la hoja de datos su
    constante de tiempo es de unos 5 ns, despreciable frente a 400 ns.
-   **Sin etapa adaptadora:** no hace falta cuando VDD baja lo suficiente
    para que el GPIO supere el umbral.
-   **Etapa adaptadora con MOSFET (BSS138) (descartada):** no se consigue el
    componente, y no queda plan B si bajar VDD no alcanza: en ese caso la
    evidencia lo registra.
-   **NPN en emisor común (descartado):** invierte la señal y, al salir de
    saturación, puede demorar hasta unos 200 ns. Eso deforma pulsos de 400 ns.

**Temporización.** La hoja de datos fija T0H = 0,4 µs, T1H = 0,8 µs,
T0L = 0,85 µs y T1L = 0,45 µs, cada uno con ±150 ns; el bit dura
1,25 µs ±600 ns y el reinicio pide más de 50 µs en bajo. Con tres
transferencias de 400 ns por bit, el "0" queda en 400 ns altos y 800 ns bajos
y el "1", en 800 ns altos y 400 ns bajos: los cuatro tiempos caen dentro de
la tolerancia y el bit dura 1,2 µs. Sin pull-up, el flanco de subida lo
genera el propio GPIO y no hay atraso de unos 80 ns. Los valores se miden con
osciloscopio en la prueba del Bloque 12.

**Orden de los datos.** Cada LED recibe 24 bits en orden GRB, con el bit más
significativo primero.

**Estado de reposo.** Los WS2812B toman el nivel bajo prolongado como señal
de reinicio. Tras un reset, el GPIO queda como entrada con pull-up y DIN se
mantiene en alto; el firmware debe poner el pin como salida en bajo antes de
la primera transmisión y mantenerlo así entre tramas.

------------------------------------------------------------------------

## 4. Arquitectura general del sistema

``` text
                         COMPUTADORA
                 +--------------------------+
                 | Aplicación Python        |
                 |                          |
                 | - Editor de armónicos    |
                 | - Generador de tablas    |
                 | - Visualización de onda  |
                 | - Instrumentos           |
                 | - Configuración del EQ   |
                 +------------+-------------+
                              |
                         UART bidireccional
                              |
                              v
+-----------------------------------------------------------+
|                         LPC1769                           |
|                                                           |
|  Entradas                                                  |
|  +----------------+  +----------------+                    |
|  | 8 pulsadores   |  | 4 potenciómetros|                    |
|  | GPIO + EINT1   |  | ADC             |                    |
|  +--------+-------+  +--------+-------+                    |
|           |                   |                            |
|           v                   v                            |
|  +-----------------------------------------------------+  |
|  | Control de voces y parámetros                       |  |
|  +---------------------------+-------------------------+  |
|                              |                            |
|  +---------------------------v-------------------------+  |
|  | Motor de síntesis                                    |  |
|  | 4 DDS -> 4 ADSR -> Mezclador                         |  |
|  +---------------------------+-------------------------+  |
|                              |                            |
|  +---------------------------v-------------------------+  |
|  | Ecualizador digital: Low-shelf / Peaking / High-shelf|  |
|  +---------------------------+-------------------------+  |
|                              |                            |
|               +--------------+--------------+             |
|               |                             |             |
|               v                             v             |
|  +-------------------------+  +-------------------------+ |
|  | Doble buffer de audio   |  | Análisis espectral      | |
|  | DMA -> DAC              |  | Ventana -> FFT -> bandas | |
|  +------------+------------+  +------------+------------+ |
|               |                            |              |
+---------------|----------------------------|--------------+
                |                            |
                v                            v
       Filtro + amplificador          Control de matrices
                |                     LED (Timer + DMA)
                v                            |
          Jack 3,5 mm                        v
             Auriculares              1 matriz 8x8
                                      (8x8 píxeles)
```

## 5. Organización propuesta del firmware

La implementación se dividirá en módulos. Los nombres son orientativos y
podrán ajustarse al organizar el código.

  Módulo            Capa       Responsabilidad
  ----------------- ---------- -------------------------------------------
  `main`            Hardware   Inicialización y coordinación general
  `board`           Hardware   Configuración de pines y recursos de la placa
  `audio_engine`    Hardware   Coordinación del flujo de audio (ISR del DMA)
  `audio_dma`       Hardware   Buffers, DMA y DAC
  `adc_input`       Hardware   Lectura de potenciómetros
  `button_input`    Hardware   Lectura de pulsadores y EINT1
  `uart_link`       Hardware   UART y buffer de recepción
  `led_matrix`      Hardware   Timer + DMA hacia los WS2812B
  `oscillator`      Lógica     DDS y lectura de tablas
  `voice`           Lógica     Asignación y estado de las cuatro voces
  `envelope`        Lógica     Máquinas de estado ADSR y mapeo de parámetros
  `mixer`           Lógica     Mezcla y control de niveles
  `equalizer`       Lógica     Filtros Biquad de tres bandas
  `debounce`        Lógica     Antirrebote de las teclas
  `uart_protocol`   Lógica     Validación y respuesta de paquetes
  `wavetable`       Lógica     Almacenamiento y administración de tablas
  `spectrum`        Lógica     Ventana, FFT y agrupación de bandas
  `led_encoder`     Lógica     Mapeo de píxeles y codificación del buffer

Los módulos de lógica no acceden a registros ni a drivers, de modo que se
pueden compilar y probar en la PC. Reciben sus datos por parámetro, y los
módulos de hardware son los únicos que tocan periféricos. En la aplicación
rige la misma separación: la lógica no depende de PySide6, que solo se usa en
`app/ui/`.

La lógica de audio deberá tener prioridad sobre las tareas de
visualización, comunicación y controles. Las interrupciones deberán ser
breves: se evitará realizar una FFT, transmitir una trama LED o procesar
paquetes UART completos dentro de una ISR.

------------------------------------------------------------------------

# 6. Plan de implementación incremental

## Regla de avance

**No se comienza un bloque nuevo hasta que el bloque actual esté
implementado, probado y aprobado.**

Cada bloque deberá terminar con:

1.  Implementación funcional.
2.  Pruebas reproducibles.
3.  Evidencia del resultado (medición, captura, registro o
    demostración).
4.  Criterios de aceptación cumplidos.
5.  Código integrado y estable en el repositorio.

Si una prueba falla, se corrige el bloque actual. No se continúa
agregando funcionalidades sobre una base que todavía no funciona.

Los bloques se dividen internamente en stories, pero el criterio de
aprobación se aplica al bloque completo.

**Excepción por el acceso al laboratorio (2026-11-14).** La fuente
regulable y el osciloscopio solo están en el laboratorio. Para resolver
todo en una visita, el firmware y la lógica de las stories 16, 17 y 18
(Bloques 12 y 13) se escriben y prueban en la PC antes de esa visita, y
quedan sin marcar los criterios que necesitan la placa. En la visita se
validan en orden: 16, 17 y 18. Cada story se cierra con sus criterios y su
evidencia, y el Bloque 12 se aprueba antes que el 13. Si algo de la 16
falla, se corrige antes de validar las otras dos y se rehace lo que
dependa de eso.

## Stories

Se trabaja en una sola story por vez, salvo la excepción de la Regla de
avance. Cada story es un archivo en
`docs/stories/` que indica:

-   Bloque al que pertenece y sección de esta especificación que implementa.
-   Estado: pendiente, en curso o terminada.
-   Criterios de aceptación propios.

Una story se cierra cuando sus criterios se cumplen y sus tests pasan.

## Evidencia

La evidencia de cada bloque (mediciones, capturas, registros) se guarda en
`docs/evidencia/`. Un bloque queda aprobado cuando sus criterios están marcados
en esta especificación y su evidencia está guardada.

------------------------------------------------------------------------

## Bloque 0 --- Preparación del entorno y estructura del proyecto

**Objetivo:** disponer de un proyecto base reproducible y una estructura
de trabajo organizada.

### Implementación

-   Crear el proyecto para LPC1769 en MCUXpresso.
-   Verificar la configuración del reloj y la frecuencia del
    microcontrolador.
-   Verificar la compilación y descarga del firmware.
-   Crear la estructura inicial de carpetas; los módulos de la sección 5 nacen con su bloque.
-   Configurar el repositorio Git.
-   Crear `docs/stories/` y `docs/evidencia/`.
-   Preparar los tests de lógica en la PC (`firmware/tests/`, con gcc y make) y
    los de la aplicación (`app/tests/`, con pytest).
-   Agregar al proyecto el include path de `Drivers/inc`.
-   Documentar la placa, herramientas y versiones utilizadas.
-   Crear un mecanismo básico de diagnóstico, por ejemplo mediante UART
    o semihosting.

### Pruebas

-   Compilar el proyecto sin errores.
-   Programar el LPC1769.
-   Ejecutar una prueba de vida, como alternar un LED GPIO.
-   Verificar que el diagnóstico funcione.
-   Comprobar que el proyecto puede reconstruirse desde un estado
    limpio.
-   Ejecutar un test de ejemplo en la PC y otro con pytest.

### Criterios de aceptación

-   [x] El proyecto compila correctamente.
-   [x] El firmware se descarga y ejecuta.
-   [x] La prueba de vida funciona.
-   [x] El repositorio contiene el proyecto base.
-   [x] El entorno de desarrollo queda documentado.
-   [x] Los tests de ejemplo se ejecutan en la PC y con pytest.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 1 --- Temporización, DAC y generación de audio de prueba

**Objetivo:** producir una señal analógica estable utilizando el DAC del
LPC1769.

### Implementación

-   Configurar el DAC y su contador (`DACCNTVAL`) para establecer la
    frecuencia de muestreo de 32 kHz.
-   Generar una tabla de prueba sencilla, inicialmente una senoide.
-   Reproducir la tabla mediante el mecanismo de actualización elegido.
-   Medir la frecuencia de muestreo y la frecuencia de la señal de
    salida.

En este bloque se puede comenzar sin DMA si facilita la puesta en
marcha. Sin embargo, antes de aprobarlo debe quedar definida y probada
la ruta de reproducción que se utilizará en el sistema final.

### Pruebas

-   Medir la salida del DAC con osciloscopio o instrumento equivalente.
-   Comprobar la frecuencia de la señal.
-   Verificar que la frecuencia de muestreo sea estable.
-   Observar la forma de onda y el nivel de salida.
-   Comprobar que no existan pausas o reinicios inesperados durante
    10 minutos.

La tolerancia de frecuencia de este bloque es ±1 %, tanto para la señal
como para la frecuencia de muestreo. Con la tabla de 256 muestras y
`DACCNTVAL` = 781, lo esperado es Fs = 32,01 kHz y f = Fs / 256 = 125,04 Hz.

### Criterios de aceptación

-   [x] El DAC produce una señal periódica.
-   [x] La frecuencia medida coincide con la esperada dentro de la
    tolerancia definida (±1 %).
-   [x] La frecuencia de muestreo es estable (±1 % durante la prueba
    prolongada).
-   [x] La señal se mantiene durante un período prolongado sin
    interrupciones (10 minutos).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 2 --- DMA y doble buffering de audio

**Objetivo:** reproducir audio de manera continua sin que la CPU tenga
que atender cada muestra individualmente.

### Implementación

-   Configurar el DMA para alimentar el DAC.
-   Reservar dos buffers de audio.
-   Configurar la alternancia entre buffers.
-   Generar el contenido de un buffer, en la ISR del DMA, mientras el DMA
    reproduce el otro.
-   Implementar la sincronización entre finalización de transferencia y
    generación del siguiente bloque.
-   Implementar un contador de underruns (sección 7.1).

### Pruebas

-   Reproducir una senoide durante varios minutos.
-   Alternar el contenido de los buffers.
-   Comprobar que no existan discontinuidades entre bloques.
-   Medir el tiempo de generación disponible por bloque.
-   Verificar que la CPU no modifique un buffer mientras el DMA lo
    utiliza.

La prueba prolongada de este bloque dura 5 minutos. La frecuencia de la
senoide se mantiene en 125,04 Hz ± 1 %, como en el Bloque 1. El pulso de
generación de cada bloque (pin de medición P2.8, sección 3.13) debe durar
menos de 500 µs, el 25 % de los 2 ms disponibles.

### Criterios de aceptación

-   [x] El DAC recibe muestras mediante DMA.
-   [x] Los dos buffers se alternan correctamente.
-   [x] No aparecen pausas ni discontinuidades atribuibles al cambio de
    buffer.
-   [x] No se sobrescribe un buffer en uso (evidencia indirecta: test de
    alternancia, senoide sin discontinuidades y contador en 0; ver
    `docs/evidencia/story-03-doble-buffer-dma.md`).
-   [x] El sistema mantiene la reproducción durante una prueba
    prolongada, con el contador de underruns en 0.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 3 --- Etapa analógica de salida

**Objetivo:** obtener una salida adecuada para auriculares desde el DAC.

Este bloque se adelantó respecto del plan inicial, donde estaba después del
ecualizador, para poder escuchar el resultado de los bloques siguientes y no
validarlos solo con osciloscopio. Se prueba con la senoide de los Bloques 1 y 2.

### Implementación

-   Conseguir los componentes de la sección 3.7 (LM358, LM386 y pasivos).
-   Construir el filtro de reconstrucción y comprobar su respuesta.
-   Construir la etapa de acoplamiento, volumen y amplificación.
-   Conectar el jack de 3,5 mm.
-   Validar los niveles de salida y ajustar los valores si hace falta.

### Pruebas

-   Medir la señal a la salida del filtro.
-   Verificar atenuación de componentes no deseadas.
-   Medir offset de continua.
-   Probar con una carga de prueba apropiada.
-   Comprobar ausencia de clipping analógico.
-   Probar con auriculares y verificar que el nivel sea controlable.

### Criterios de aceptación

-   [x] El circuito reproduce correctamente el audio del DAC
    (filtro con la misma amplitud entrada/salida, ±1 dB).
-   [x] El filtro atenúa las imágenes de muestreo de manera adecuada
    (cálculo ≥ 12 dB a 32 kHz, §3.7; evidencia indirecta: −15,96 dB con
    los componentes medidos, ver `docs/evidencia/story-04-auriculares.md`).
-   [x] El amplificador puede manejar la carga prevista
    (16 Ω por canal, 0,12 a 0,21 Vrms al máximo).
-   [x] No existe una componente continua peligrosa en la salida
    (menos de 10 mV con carga).
-   [x] El nivel de audio es utilizable y no presenta distorsión
    evidente, con el volumen controlable de silencio a nivel fuerte
    (aceptado con ruido por un contacto intermitente, a mejorar al final
    del proyecto; ver `docs/evidencia/story-04-auriculares.md`).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 4 --- DDS y reproducción de una voz

**Objetivo:** generar distintas frecuencias mediante un acumulador de
fase sin modificar la frecuencia de muestreo.

### Implementación

-   Implementar el acumulador de fase en Q16.16 (sección 3.1).
-   Implementar el incremento de fase.
-   Implementar la conversión de fase a índice de tabla.
-   Reproducir una tabla de onda mediante DDS.
-   Implementar el cálculo del incremento de fase a partir de la
    frecuencia deseada.
-   Usar muestras en Q15 y documentar las escalas de punto fijo.

### Pruebas

-   Generar varias frecuencias con la misma tabla.
-   Medir las frecuencias obtenidas.
-   Comparar valores teóricos y medidos.
-   Comprobar cambios de frecuencia sin alterar la frecuencia de
    muestreo.
-   Verificar el comportamiento al pasar por el límite del acumulador.

La tolerancia de este bloque es ±1 % (≈ 17 cents) en la frecuencia de las
notas, y menos de 1 cent en los incrementos de fase, que se verifican en la
PC con Fs = 25 MHz / 781.

Mientras no estén las teclas (Bloque 6), la prueba usa una secuencia
automática: la escala de Do mayor de Do4 a Do5 (sección 3.11), unos 5 s por
nota y en bucle. Es andamiaje de este bloque y se retira en el Bloque 6.

No hay instrumento para medir la frecuencia de las notas, así que se aprueba
con evidencia indirecta. Como f = ΔP · Fs / 2^24, los incrementos se
verifican en la PC, y la Fs ya se midió en el Bloque 1: de 31 936 a 32 012 Hz,
dentro de ±1 % (`docs/evidencia/story-02-senoide-dac.md`). Este bloque no
cambia el contador del DAC, así que la Fs sigue fija y las notas quedan
dentro de ±1 %. La ausencia de interrupciones se aprueba escuchando la
secuencia y con los tests de la PC. Si una medición posterior contradice
esta evidencia, se corrige antes de continuar.

Escalas de punto fijo: la fase de cada voz está en Q16.16, con vuelta en
2^24, y el índice de tabla son los bits 23:16 (sección 3.1). Las muestras de
las tablas están en Q15 (sección 3, aritmética DSP).

### Criterios de aceptación

-   [x] Una voz reproduce una tabla mediante DDS.
-   [x] Se pueden seleccionar distintas frecuencias (las 8 notas de la
    sección 3.11, con la secuencia de prueba).
-   [x] La frecuencia de muestreo permanece fija (evidencia indirecta:
    el contador del DAC no cambia y la nota se elige con el incremento).
-   [x] La frecuencia generada coincide con la esperada dentro de la
    tolerancia definida (evidencia indirecta: incrementos con menos de
    0,01 cents de error y −4,0 cents en el peor caso con la Fs medida;
    ver `docs/evidencia/story-05-notas-afinadas.md`).
-   [x] No aparecen errores numéricos ni interrupciones (test de vuelta
    del acumulador y escucha sin cortes, con y sin depurador).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 5 --- Cuatro voces y mezclador digital

**Objetivo:** reproducir hasta cuatro notas simultáneamente.

### Implementación

-   Crear la estructura de datos de una voz.
-   Mantener un acumulador de fase independiente por voz.
-   Permitir asignar una frecuencia y una amplitud a cada voz.
-   Implementar la suma de las cuatro voces.
-   Aplicar el escalado y la saturación de la sección 3.2, con su contador.
-   Mantener la fase de las voces activas entre bloques de audio.

### Pruebas

-   Reproducir una voz.
-   Reproducir dos, tres y cuatro voces simultáneas.
-   Verificar las frecuencias individuales.
-   Comparar la señal mezclada con la suma esperada.
-   Probar combinaciones de amplitudes máximas.
-   Comprobar que no haya desborde ni wrap-around, y medir las saturaciones
    con el contador.

Mientras no estén las teclas (Bloque 6), la prueba usa una secuencia
automática de acordes: Do4, Mi4, Sol4 y Do5 se van sumando de a una nota
(1, 2, 3 y 4 voces a la vez), unos 5 s por paso y en bucle. Reemplaza a la
escala del Bloque 4 y se retira igual en el Bloque 6. Las voces suenan con
la amplitud máxima, que hasta el Bloque 7 ocupa el lugar del nivel de la
envolvente: con 3 y 4 voces la suma recorta, y esas saturaciones son las que
mide el contador.

No hay osciloscopio, así que la forma de la mezcla y del recorte en AOUT no
se observa. El bloque se aprueba con evidencia indirecta: la suma, la
saturación y la continuidad de fase se verifican con los tests de la PC; la
ausencia de cortes se aprueba escuchando el acorde, con y sin depurador; y
los contadores de underruns y de saturaciones se leen con el depurador. Si
una medición posterior contradice esta evidencia, se corrige antes de
continuar.

La duración de la ISR del DMA se mide con el contador de SysTick, leído al
entrar y al salir: con una ISR de menos de 1 ms, la diferencia módulo el
valor de recarga + 1 es válida aunque el contador dé la vuelta en el medio.
Se registra el máximo por bloque con cuatro voces (y con una, para estimar
el costo por voz). La spec no fija un margen, así que el valor queda como
base para el Bloque 10 (sección 3.12).

Aclaración del paso 1 de la sección 3.2: cada producto muestra · amplitud
se desplaza 15 bits a la derecha antes de sumar, no después de sumar. Las
muestras y la amplitud están en Q15 y la suma, en 32 bits.

### Criterios de aceptación

-   [x] Las cuatro voces funcionan independientemente (test de fase por voz;
    underruns en 0).
-   [x] Cada voz puede tener una frecuencia distinta (suma con cuatro notas
    distintas y acorde).
-   [x] La mezcla contiene las cuatro señales (evidencia indirecta: suma
    muestra por muestra).
-   [x] La suma no produce overflow y la salida se satura sin wrap-around
    (picos 0/1023 y contador exacto; ver
    `docs/evidencia/story-06-cuatro-voces.md`).
-   [x] La reproducción continúa sin interrupciones (underruns en 0 y
    escucha con y sin depurador).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 6 --- Pulsadores y asignación de notas

**Objetivo:** controlar el sintetizador mediante ocho pulsadores
físicos.

### Implementación

-   Configurar ocho entradas GPIO.
-   Implementar antirrebote.
-   Implementar la correspondencia entre pulsadores y notas (sección 3.11).
-   Detectar eventos de pulsación y liberación.
-   Implementar asignación de notas a las cuatro voces, con la política de
    la sección 3.2.
-   Configurar el pulsador de EINT1 para rotar entre las formas de onda
    (sección 3.8).

Las teclas se leerán por GPIO desde SysTick, cada 1 ms. El pulsador de
selección de onda es el único que usa una interrupción externa.

El bloque se divide en dos stories: la story 7 (teclas y asignación de voces)
cubre los criterios 1 a 5, y la story 8 (EINT1 y formas de onda), el 6.

Hasta el Bloque 7 no hay ADSR: las notas entran y salen de golpe, con la
amplitud fija, y al tocar o soltar puede oírse un click.

En este bloque, si se aprieta una tecla con las cuatro voces ocupadas, la
quinta reemplaza a la voz más antigua. En el Bloque 7 (story 9), con la
envolvente, se agregan antes el paso 2 de la asignación de la sección 3.2
(la voz en Release con menor nivel) y la reutilización de la voz de una tecla
que vuelve a sonar; la voz más antigua pasa a ser el último recurso.

### Pruebas

-   Pulsar cada tecla individualmente.
-   Mantener varias teclas presionadas.
-   Probar combinaciones de hasta cuatro notas.
-   Presionar más de cuatro teclas.
-   Verificar el antirrebote.
-   Comprobar que las liberaciones se detecten correctamente.
-   Rotar por todas las formas de onda con EINT1.
-   Mantener apretado el pulsador de EINT1 más de 150 ms y comprobar que
    cuenta una sola pulsación.
-   Cambiar de onda mientras suena una nota y comprobar que el cambio se
    aplique recién en la siguiente.

### Criterios de aceptación

-   [x] Los ocho pulsadores se detectan correctamente.
-   [x] Cada pulsador corresponde a la nota definida.
-   [x] Se pueden tocar cuatro notas simultáneas.
-   [x] La asignación de voces funciona cuando se supera la polifonía.
-   [x] No se producen activaciones falsas por rebote.
-   [x] EINT1 rota las ondas y el cambio se aplica desde la próxima nota.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 7 --- Envolventes ADSR y potenciómetros

**Objetivo:** controlar la evolución de amplitud de cada nota.

### Implementación

-   Implementar una máquina de estados ADSR por voz, actualizada cada
    1 ms por SysTick.
-   Implementar las etapas Attack, Decay, Sustain y Release.
-   Configurar cuatro canales ADC en modo burst para los potenciómetros, y
    leerlos desde SysTick.
-   Asignar un potenciómetro a cada parámetro ADSR.
-   Filtrar o suavizar las lecturas ADC para evitar variaciones bruscas.
-   Convertir los valores ADC en tiempos y niveles utilizables.
-   Implementar el comportamiento ante las teclas de la sección 3.4.

El bloque se divide en dos stories: la story 9 (envolvente por voz) cubre
los criterios 1, 2 y 5, y la story 10 (potenciómetros), el 3 y el 4.

En la story 9 no había ADC y los parámetros de la envolvente eran fijos
(Attack 300 ms, Decay 300 ms, Sustain 50 % y Release 1000 ms). Desde la
story 10 salen de los potenciómetros y esos valores fijos se retiran.

### Pruebas

-   Probar cada etapa por separado.
-   Medir tiempos de Attack, Decay y Release.
-   Verificar el nivel de Sustain.
-   Cambiar los potenciómetros durante la reproducción.
-   Probar varias voces con envolventes simultáneas.
-   Comprobar que una voz liberada llegue a amplitud cero y quede
    disponible.

### Criterios de aceptación

-   [x] Cada voz mantiene su propia envolvente (niveles distintos por voz
    en placa; ver `docs/evidencia/story-09-envolvente-adsr.md`).
-   [x] Las cuatro etapas ADSR funcionan (capturas en Decay, Sustain y
    Release; el Attack, de oído y por tests).
-   [x] Los potenciómetros modifican los parámetros correspondientes (ver `docs/evidencia/story-10-potenciometros-adsr.md`).
-   [x] Las envolventes no generan saltos o valores fuera de rango (evidencia parcial: la medición en placa de saltos y underruns queda pendiente; ver `docs/evidencia/story-10-potenciometros-adsr.md`).
-   [x] Las voces quedan disponibles al finalizar Release (captura con las
    cuatro voces libres y en nivel 0).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 8 --- Aplicación Python y editor de armónicos

**Objetivo:** diseñar formas de onda desde una computadora.

### Implementación

-   Crear la aplicación de escritorio.
-   Crearla con PySide6, numpy y pyqtgraph.
-   Permitir seleccionar la frecuencia fundamental de referencia.
-   Cargar automáticamente los armónicos, hasta el máximo de la sección 3.3.
-   Permitir configurar la amplitud de la fundamental.
-   Permitir configurar la amplitud de cada armónico.
-   Definir e incorporar las configuraciones de guitarra, piano, órgano y
    flauta, con su mensaje de ajuste de la ADSR.
-   Generar la señal mediante síntesis aditiva.
-   Mostrar la forma de onda en el dominio temporal.
-   Mostrar el espectro de amplitud.
-   Normalizar la señal.
-   Cuantizar las muestras al formato definido para el LPC1769.
-   Exportar la tabla a un archivo para facilitar las pruebas.

En esta etapa todavía no es necesario transferir la tabla por UART.
Primero debe verificarse que el generador produzca resultados correctos.

### Pruebas

-   Generar una senoide.
-   Generar una señal con fundamental y segundo armónico.
-   Generar una señal con varios armónicos.
-   Comprobar las frecuencias presentes en el espectro.
-   Verificar que la señal sea periódica.
-   Comprobar que la cuantización respete el rango permitido.
-   Cambiar la frecuencia y comprobar que se carguen los armónicos
    correctos.
-   Cargar cada instrumento y comprobar sus amplitudes y su mensaje de ADSR.

### Criterios de aceptación

-   [x] Se pueden configurar las amplitudes armónicas.
-   [x] La aplicación genera una tabla de muestras.
-   [x] La forma de onda y el espectro se visualizan correctamente.
-   [x] La tabla está normalizada y cuantizada.
-   [x] La cantidad de armónicos respeta los límites definidos.
-   [x] Los instrumentos cargan sus armónicos y muestran cómo ajustar la
    ADSR.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 9 --- Protocolo UART y transferencia de tablas

**Objetivo:** transferir formas de onda desde Python al LPC1769 de
manera confiable.

### Implementación

-   Configurar UART en el LPC1769 mediante su driver.
-   Implementar recepción sin bloquear el motor de audio.
-   Implementar el formato de paquete de la sección 3.14 y documentarlo en
    `docs/protocolo.md`.
-   Implementar mensajes de confirmación y error.
-   Transferir una tabla completa.
-   Validar longitud, formato y rango de muestras.
-   Almacenar la tabla recibida sin modificar una tabla que esté en uso.
-   Activar una tabla nueva solamente después de validar su recepción,
    cambiando el puntero de la quinta onda entre bloques de audio.

### Pruebas

-   Enviar una tabla de 256 muestras y tramas de longitud incorrecta.
-   Comprobar que los datos recibidos coincidan con los enviados.
-   Interrumpir una transferencia intencionalmente.
-   Enviar paquetes inválidos.
-   Verificar que el firmware rechace datos corruptos.
-   Reproducir una tabla recibida.
-   Probar la recepción mientras el audio está funcionando.

### Criterios de aceptación

-   [x] Python y el LPC1769 intercambian paquetes.
-   [x] Las tablas se reciben completas.
-   [x] Los errores de transferencia se detectan.
-   [x] Una tabla inválida no reemplaza una tabla funcional.
-   [x] La recepción no interrumpe la reproducción de audio.
-   [x] Una tabla recibida queda disponible como quinta onda.

El bloque se cerró sin la story de errores de envío (que iba a ser la 14 y
se descartó por falta de tiempo; el número 14 pasó al ecualizador). Quedaron fuera los NACK con sus códigos, PING y la
recuperación de una transferencia cortada. La placa descarta sin
responder una trama con LEN distinto de 512 o con el checksum
incorrecto, y la aplicación informa cualquier falla como que la placa no
confirmó la recepción. El primero, el segundo, el quinto y el sexto
criterio se verificaron en la story 13
(`docs/evidencia/story-13-tests.md`). El tercero y el cuarto se dan por
cumplidos con los tests del parser en la PC (checksum y LEN) y con el
uso real de la aplicación con la placa.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 10 --- Ecualizador digital de tres bandas

**Objetivo:** modificar el balance frecuencial del audio antes de
enviarlo al DAC.

### Implementación

-   Implementar un filtro Biquad en punto fijo, con coeficientes Q2.30 y
    acumulador de 64 bits.
-   Verificar su estabilidad y respuesta.
-   Implementar low-shelf.
-   Implementar peaking EQ.
-   Implementar high-shelf.
-   Conectar los tres filtros en cascada.
-   Usar las frecuencias y ganancias de la sección 3.5.
-   Permitir modificar los coeficientes entre bloques, sin producir saltos
    audibles.
-   Calcular los coeficientes en Python y validar que entren en Q2.30.
-   Incorporar configuración desde Python, inicialmente mediante valores
    de prueba y luego por UART, con perillas para frecuencias bajas, medias
    y altas.

### Pruebas

-   Aplicar señales de frecuencia conocida.
-   Medir la respuesta de cada filtro.
-   Verificar el comportamiento de graves, medios y agudos.
-   Comprobar estabilidad para los rangos de parámetros permitidos.
-   Probar cambios de parámetros durante la reproducción.
-   Medir el costo de procesamiento por bloque.

### Criterios de aceptación

-   [x] Los tres filtros funcionan individualmente (evidencia indirecta:
    respuesta de cada banda en la PC y prueba de uso de oído en placa; ver
    `docs/evidencia/story-14-ecualizador.md`).
-   [x] Los tres filtros funcionan en cascada (evidencia indirecta: cascada
    en la PC con 0,08 dB de desviación máxima y prueba de uso con varias
    teclas).
-   [x] La respuesta frecuencial coincide con la configuración (evidencia
    indirecta: solo en la PC; la respuesta en placa no se midió).
-   [x] No aparecen inestabilidades ni overflow (evidencia indirecta: toda
    la grilla de ganancias en la PC, sin wrap-around y con vuelta al silencio
    exacto; en placa, solo la prueba de uso).
-   [x] El procesamiento cumple el plazo de generación de audio (sin
    medición al cerrar el bloque: decidimos marcarlo por falta de tiempo.
    Se midió después, en la story 15: con cuatro voces y el ecualizador
    la ISR dura 127 912 ciclos en Debug sin optimizar, por encima de la mitad
    del bloque, y 39 016 con `-O1`, que cumple.
    `docs/evidencia/story-15-linea-base-isr.md`).

Decidimos cerrar el bloque con evidencia indirecta por falta de tiempo
(story 14). El multímetro disponible no resuelve el nivel de la salida, así
que la respuesta se respalda con los tests de la PC y con la prueba de uso.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 11 --- FFT y cálculo de las 8 bandas

**Objetivo:** analizar el espectro de la señal de audio en tiempo real.

### Implementación

-   Implementar una FFT propia radix-2 de 512 puntos en Q15, con
    escalado por etapa para evitar desbordes.
-   Medir su costo computacional.
-   Generar las tablas de factores de giro y de ventana en Flash.
-   Implementar una ventana Hann.
-   Calcular la FFT de bloques de audio.
-   Calcular magnitudes espectrales.
-   Agrupar los bins en 8 bandas de una octava (sección 3.6).
-   Aplicar escala y normalización.
-   Convertir las magnitudes en niveles de 0 a 8.
-   Incorporar suavizado temporal y caída gradual de las barras.

El análisis utilizará la señal posterior al ecualizador y anterior al
DAC, tomada del buffer de captura de la sección 3.6, sin interferir con los
buffers que están siendo reproducidos o generados.

### Pruebas

-   Analizar una senoide de frecuencia conocida.
-   Verificar que el pico aparezca en la banda esperada.
-   Analizar señales con varios armónicos.
-   Probar distintas frecuencias y amplitudes.
-   Verificar la agrupación de los bins.
-   Medir el tiempo de ejecución de la FFT.
-   Comprobar que el análisis no provoque underruns de audio.

### Criterios de aceptación

-   [x] La FFT identifica correctamente las componentes de prueba (tests
    de la PC y en placa: las notas Do4 a Do5, cinco armónicos sueltos del
    editor hasta el 30.º y una onda de varios armónicos; el pico cae en el bin
    más cercano. `docs/evidencia/story-15-fft-bandas.md`).
-   [x] Las 8 bandas se calculan correctamente (la banda más alta es la que
    contiene el bin del pico, y las vecinas suben con la diferencia en dB que
    da la ventana Hann, a ±0,1 dB).
-   [x] La conversión a niveles de 0 a 8 es estable (cinco lecturas con una
    nota sostenida: ninguna barra varía más de 1 nivel, y al soltar la tecla
    todas vuelven a 0).
-   [x] El procesamiento cumple los plazos establecidos (con cuatro voces y el
    ecualizador, la ISR dura 40 560 ciclos, el 20,3 % del bloque, y el
    análisis completa 55 cuadros por segundo; en Debug con `-O1`. Con `-O0`
    la ISR duraba ≈ 127 000 y no cumplía.
    `docs/evidencia/story-15-linea-base-isr.md`).
-   [x] La reproducción de audio no se interrumpe (`audioUnderruns` en 0
    con cuatro voces, ecualizador y análisis corriendo, con una sola
    suspensión del depurador).

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 12 --- Control de matrices CJMCU-64

**Objetivo:** controlar la matriz LED.

### Implementación

-   Verificar el modelo exacto de controlador y la disposición de
    píxeles de la matriz.
-   Conectar la matriz y comprobar su funcionamiento.
-   Conectar DIN a P0.0 por una resistencia de 330 Ω y alimentar la matriz
    a 4,5 V (sección 3.15).
-   Comprobar en la placa que P0.0 esté libre y accesible.
-   Configurar P0.0 como salida en bajo antes de la primera transmisión y
    mantenerlo así entre tramas.
-   Implementar el protocolo de comunicación para los WS2812B con Timer +
    DMA, según el esquema inicial siguiente:
    -   Timer2 con MR0 y reinicio al coincidir, para pedir una transferencia
        cada 400 ns (MAT2.0, con PCLK de 25 MHz y MR0 = 9).
    -   Tres transferencias de 1 byte por bit: alto, dato y bajo. Da un bit
        de 1,2 µs.
    -   Canal DMA 1, desde el buffer al registro `FIOPIN0`, con `FIOMASK`
        para que solo cambie P0.0.
    -   Dos elementos LLI, porque 4608 transferencias (64 píxeles × 24 bits
        × 3 bytes) superan el máximo de 4095 por elemento.
-   Fijar a mano la dirección de destino del DMA: en M2P con conexión
    `GPDMA_MAT2_0`, `GPDMA_SetupChannel` la apunta a `MR0`, no al GPIO.
-   Limpiar la petición de DMA pendiente del Timer antes de habilitar el
    canal.
-   Implementar el envío de los 64 píxeles.
-   Crear una función de representación de una matriz lógica de 8×8.
-   Implementar la limitación de brillo al 25 %.
-   Evitar transmisiones bloqueantes y conflictos con el audio.
-   Incorporar alimentación externa regulable a 4,5 V y masa común con el
    LPC1769, comprobando la tensión de la fuente y fijando su límite de
    corriente antes de conectar la matriz.

### Pruebas

-   Medir en DIN los niveles alto y bajo con osciloscopio.
-   Medir en DIN la duración de los pulsos "0" y "1", y compararla con la
    hoja de datos del WS2812B.
-   Encender un píxel individual.
-   Recorrer filas y columnas.
-   Comprobar la orientación de la matriz.
-   Mostrar patrones de prueba.
-   Verificar la transmisión de 64 píxeles.
-   Medir el tiempo de actualización.
-   Comprobar que el envío no interrumpa el audio.
-   Verificar alimentación y temperatura (al tacto) en condiciones de uso,
    con los límites de §3.6.

### Criterios de aceptación

-   [ ] DIN cumple los niveles (alto ≥ 0,7 × VDD) y los tiempos del
    WS2812B (§3.15), medidos con osciloscopio.
-   [ ] La matriz funciona.
-   [ ] Se controla como una superficie lógica de 8×8.
-   [ ] El mapeo físico de píxeles es correcto.
-   [ ] Se pueden mostrar patrones y colores.
-   [ ] La actualización no bloquea el audio.
-   [ ] El consumo y la alimentación se mantienen dentro de los límites
    seguros de §3.6 (corriente ≤ 1,1 A, tensión en la matriz ≥ 4,0 V y
    temperatura al tacto), tras 5 minutos con los 64 LED en blanco al 25 %.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 13 --- Integración del visualizador de espectro

**Objetivo:** mostrar en la matriz el espectro calculado en el bloque
11.

### Implementación

-   Conectar la salida de las 8 bandas con el controlador LED.
-   Asignar una columna a cada banda.
-   Convertir cada nivel en una altura de 0 a 8 píxeles.
-   Pintar los LED encendidos con el color de la onda seleccionada
    (sección 3.6; la story 17 usa rojo fijo y la 18 agrega el color por
    onda).
-   Implementar suavizado y caída de las barras (la caída en pantalla de
    la sección 3.6).
-   Limitar la frecuencia de actualización a un objetivo inicial de
    20--30 FPS.
-   Coordinar la actualización LED con la reproducción de audio.

### Pruebas

-   Reproducir una senoide y observar la banda correspondiente.
-   Reproducir señales con varios armónicos.
-   Tocar varias notas simultáneamente.
-   Cambiar el ecualizador y observar el cambio del espectro.
-   Rotar las cinco ondas con el pulsador y observar el color de cada una.
-   Verificar la estabilidad de las barras.
-   Ejecutar el sintetizador durante varios minutos con el visualizador
    activo.
-   Comprobar que no aparezcan underruns de audio.

### Criterios de aceptación

-   [ ] Las barras representan las 8 bandas calculadas.
-   [ ] La altura refleja la magnitud de cada banda.
-   [ ] Los cambios de timbre y ecualización se reflejan visualmente.
-   [ ] La actualización es fluida.
-   [ ] El audio permanece estable durante la visualización.
-   [ ] Las barras toman el color de la onda seleccionada.

El bloque se divide en dos stories: la 17 (barras del espectro) cubre los
criterios 1 a 5, y la 18 (color por onda), el 6.

**No avanzar hasta cumplir todos los criterios.**

------------------------------------------------------------------------

## Bloque 14 --- Integración completa y pruebas de sistema

**Objetivo:** verificar el funcionamiento conjunto del sintetizador.

### Implementación

-   Integrar entradas, voces, ADSR, tablas, ecualizador, DAC, FFT y
    matrices.
-   Integrar la aplicación Python y la comunicación UART.
-   Revisar prioridades de interrupciones y tareas.
-   Medir uso de CPU y memoria.
-   Revisar todos los buffers y sus propietarios.
-   Incorporar manejo de errores.
-   Documentar el procedimiento de puesta en marcha.

### Pruebas

-   Tocar una nota y variar sus parámetros.
-   Tocar cuatro notas simultáneamente.
-   Subir una onda nueva desde Python y seleccionarla rotando con EINT1.
-   Cambiar parámetros del ecualizador.
-   Observar el visualizador durante la reproducción.
-   Transferir una tabla mientras el sistema reproduce audio.
-   Ejecutar pruebas prolongadas.
-   Comprobar recuperación ante paquetes UART inválidos.
-   Medir carga de CPU, uso de SRAM y estabilidad de audio.
-   Demostrar en placa la función de cada periférico: SysTick (ADSR y
    teclas), Timer (matrices), ADC (potenciómetros), DAC y GPDMA (audio), UART
    (aplicación), GPIO (teclas y datos) y EINT1 (onda).

### Criterios de aceptación

-   [ ] Todas las funciones principales operan simultáneamente.
-   [ ] No existen interrupciones audibles durante el uso normal.
-   [ ] El contador de underruns permanece en 0 durante la prueba
    prolongada.
-   [ ] Cada periférico cumple su función, demostrada en placa.
-   [ ] No se producen errores de memoria ni overflow.
-   [ ] La comunicación UART no afecta la reproducción.
-   [ ] El visualizador no afecta la reproducción.
-   [ ] Los controles responden correctamente.
-   [ ] El sistema supera una prueba prolongada.

**El proyecto principal se considera terminado cuando se cumplen todos
los criterios.**

------------------------------------------------------------------------

## Bloque 15 --- Ampliación: wavetable variable en el tiempo

**Objetivo:** enriquecer el timbre de los instrumentos mediante varias
tablas que evolucionan durante una nota.

### Implementación

-   Definir un instrumento como un conjunto ordenado de tablas.
-   Definir en qué etapa o intervalo temporal se utiliza cada tabla.
-   Implementar transición entre tablas.
-   Evaluar interpolación entre tablas para evitar saltos.
-   Ampliar el editor Python para crear y organizar las tablas.
-   Ampliar el protocolo UART para transferir instrumentos completos.
-   Gestionar memoria y activación segura de los nuevos datos.

### Pruebas

-   Comparar una nota con tabla fija y con tablas variables.
-   Verificar que las transiciones no produzcan clicks.
-   Medir memoria utilizada.
-   Medir costo de procesamiento.
-   Probar varias voces con instrumentos variables.

### Criterios de aceptación

-   [ ] Un instrumento puede contener varias tablas.
-   [ ] Las tablas evolucionan durante la nota.
-   [ ] Las transiciones son audiblemente correctas.
-   [ ] No se exceden los límites de memoria.
-   [ ] La reproducción mantiene sus plazos.

Este bloque es una ampliación y no condiciona la aprobación del
sintetizador principal.

------------------------------------------------------------------------

# 7. Reglas técnicas transversales

Estas reglas se aplican durante todos los bloques.

## 7.1. Tiempo real de audio

-   La generación de audio tiene prioridad sobre la visualización, UART
    y controles.
-   Las ISR deben ser breves. La ISR del DMA es la excepción, porque genera
    el bloque de audio, pero su duración debe estar acotada y medida.
-   No ejecutar FFT dentro de la interrupción de audio.
-   No transmitir matrices LED desde una ISR.
-   No realizar operaciones bloqueantes dentro del camino crítico.
-   Medir el tiempo de procesamiento por bloque y conservar margen antes
    del próximo vencimiento.
-   Mantener un contador de underruns: al terminar de generar un bloque, la
    ISR comprueba si el canal de audio ya terminó el buffer siguiente y, en
    ese caso, incrementa el contador.

## 7.2. Memoria

El LPC1769 dispone de recursos limitados de SRAM. Antes de fijar tamaños
definitivos se deberá realizar un presupuesto de memoria que contemple:

-   Buffers de audio.
-   Tablas de ondas.
-   Estados de las voces.
-   Estados de filtros.
-   Buffers de FFT.
-   Buffers de transmisión LED.
-   Buffers UART y estructuras auxiliares.

No se deben asignar buffers grandes sin justificar su necesidad.

## 7.3. Punto fijo

-   Documentar el formato numérico de cada señal.
-   Documentar los rangos permitidos.
-   Controlar el crecimiento de la suma de voces.
-   Evitar overflow en multiplicaciones y acumulaciones.
-   Verificar la estabilidad de los Biquad.
-   Medir el error numérico con señales de referencia.

## 7.4. Propiedad de los buffers

Cada buffer debe tener un propietario y un estado claramente definido.

-   El DMA es el único consumidor del buffer de audio que está
    reproduciendo.
-   La CPU genera muestras en el buffer que no está siendo reproducido.
-   Las tablas activas no se modifican durante su utilización.
-   Los datos recibidos por UART se validan en un espacio separado antes
    de activarse.
-   El análisis FFT debe utilizar una copia o una región de muestras
    cuyo acceso esté coordinado.

## 7.5. Integración incremental

-   Mantener una versión funcional al finalizar cada bloque.
-   No mezclar cambios de varios bloques sin pruebas intermedias.
-   Registrar problemas conocidos.
-   Conservar pruebas que puedan repetirse.
-   Si una integración rompe un bloque aprobado, se corrige antes de
    continuar.

## 7.6. Tests

  Qué se prueba                 Dónde         Cómo
  ----------------------------- ------------- --------------------------------
  Módulos de lógica del         PC            `firmware/tests/`, con gcc y make,
  firmware                                    sin frameworks externos
  Lógica de la aplicación       PC            `app/tests/`, con pytest
  Módulos de hardware           Placa         Mediciones con evidencia en
                                              `docs/evidencia/`

-   Los tests deben pasar antes de cerrar una story.
-   Los módulos de lógica usan tipos de `stdint.h`, para comportarse igual en
    la PC y en el LPC1769.
-   Los módulos de lógica se prueban con señales de referencia: frecuencias
    del DDS, tiempos de la ADSR, respuesta de los Biquad, picos de la FFT y
    tramas válidas e inválidas.

------------------------------------------------------------------------

# 8. Riesgos técnicos que deben vigilarse

  -----------------------------------------------------------------------
  Riesgo                  Consecuencia            Mitigación
  ----------------------- ----------------------- -----------------------
  Sobrecarga de CPU       Underrun de audio       Medir tiempo por bloque
                                                  y reducir costo de DSP

  Overflow en mezcla      Distorsión digital      Escalado, acumuladores
                                                  adecuados y pruebas de
                                                  rango

  Alias en tablas         Timbre incorrecto a     Limitar armónicos y
                          ciertas notas           evaluar tablas
                                                  band-limited

  Cambios abruptos de     Clicks audibles         Doble tabla y cambio
  tabla                                           seguro entre bloques

  Inestabilidad de Biquad Señal incorrecta o      Limitar parámetros y
                          saturación              validar coeficientes

  FFT demasiado costosa   Interrupciones de audio FFT por bloques y
                                                  ejecución de menor
                                                  prioridad

  Transmisión LED         Pérdida de muestras     Temporizador con DMA
  bloqueante                                      

  Consumo elevado de LEDs Caída de tensión o      Fuente regulable con
                          calentamiento           límite de corriente y
                                                  limitación de brillo

  Conflictos de DMA       Transferencias          Asignar canales y
                          incorrectas             prioridades desde el
                                                  diseño

  SRAM insuficiente       Corrupción o            Presupuesto de memoria
                          imposibilidad de        temprano
                          integrar                
  -----------------------------------------------------------------------

------------------------------------------------------------------------

# 9. Definiciones pendientes para la etapa de diseño detallado

Las siguientes decisiones no impiden iniciar los primeros bloques, pero
deben resolverse antes de implementar los bloques afectados.

-   Frecuencia de muestreo definitiva (se comienza con 32 kHz).
-   Factor Q de cada banda del ecualizador (Bloque 10). Resuelto: 1/√2 en
    los shelves y 1 en el peaking (§3.5).
-   Límites exactos de las bandas (Bloque 11). Resuelto: 8 bandas de una
    octava desde 62,5 Hz hasta 16 kHz, una por columna de la matriz (§3.6).
-   Confirmación en placa del esquema de Timer + DMA para los WS2812B y de
    que P0.0 esté disponible (Bloque 12).
-   Validación de la etapa analógica y del nivel de salida (Bloque 3).

Cada decisión deberá documentarse antes de comenzar el bloque que
dependa de ella.

------------------------------------------------------------------------

# 10. Definición de finalización

El sintetizador principal estará terminado cuando:

1.  Pueda reproducir cuatro notas simultáneamente.
2.  Cada voz tenga su propio DDS y ADSR.
3.  Se puedan generar y transferir tablas desde Python.
4.  El ecualizador de tres bandas funcione en tiempo real.
5.  El audio se reproduzca mediante DMA y DAC sin interrupciones, con el
    contador de underruns en 0.
6.  La salida analógica sea adecuada para auriculares.
7.  La matriz muestre el espectro de audio mediante FFT.
8.  El sistema completo supere las pruebas de integración y estabilidad.

La evolución del timbre mediante múltiples wavetables se considera una
ampliación posterior.

**Principio rector del desarrollo: un bloque funcional y probado es la
base del siguiente. No se agregan funcionalidades para compensar o
esconder fallas de bloques anteriores.**
