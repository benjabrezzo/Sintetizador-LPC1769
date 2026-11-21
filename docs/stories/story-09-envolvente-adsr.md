# Story 9: Envolvente ADSR por voz

**Bloque:** 7 · **Spec:** §3.2, §3.4, §6 Bloque 7 · **Estado:** terminada

## Story
Como músico, quiero que cada nota suba, se sostenga y se apague con su propia envolvente, para que suene como un instrumento y no haga click al tocarla o soltarla.

## Criterios de aceptación
- [x] Dados los auriculares y las teclas armados, con la envolvente en sus valores fijos de prueba (A 300 ms, D 300 ms, S 50 %, R 1000 ms), cuando toco una nota, la mantengo y la suelto, entonces el volumen sube, baja hasta el nivel de Sustain y se queda ahí mientras la aprieto, y al soltarla se apaga en el tiempo de Release. No hay clicks al tocar ni al soltar, tampoco si la suelto antes de que termine el Attack o el Decay.
- [x] Dadas varias teclas apretadas en momentos distintos, cuando suelto una, entonces solo esa se apaga y las otras siguen cada una con su envolvente. Cuando termina de apagarse, su voz queda libre (se ve en el depurador). Los underruns siguen en 0.
- [x] Dadas cuatro voces ocupadas, con dos notas ya soltadas que se están apagando, cuando aprieto otra tecla, entonces se calla la que se apagaba con menos volumen, la otra sigue apagándose y las apretadas no se cortan. Si vuelvo a apretar una tecla mientras su nota se apaga, la nota vuelve a subir desde el volumen en que estaba, sin click y en la misma voz. Con las cuatro apretadas, la quinta sigue reemplazando a la más antigua, sin click: la nota nueva sigue desde la fase y el nivel de esa voz.

## Plan
- Registrar primero en la spec:
  - La división del Bloque 7: la story 9 cubre los criterios 1, 2 y 5, y la story 10, el 3 y el 4.
  - La envolvente es lineal y cada etapa dura su tiempo desde el nivel en que arranca: soltar a mitad del Attack igual tarda R en apagarse.
  - Los valores fijos de prueba hasta la story 10: A 300 ms, D 300 ms, S 50 % y R 1000 ms.
  - Una voz reutilizada sigue desde su fase y su nivel.
- Escribir primero los tests y después la lógica pura de la envolvente: cada etapa en los extremos (2 y 2000 ms, con error ≤ 1 ms), Sustain en 0 y 100 %, Release desde cualquier etapa, Attack desde el nivel actual, el nivel siempre entre 0 y 32767, y la voz libre al terminar el Release. Con Sustain en 0 y la tecla apretada, la voz sigue ocupada.
- Escribir primero los tests y después la asignación: voz libre → voz en Release con menor nivel → voz más antigua, más la reutilización de la voz de una tecla que vuelve a sonar. Para saber si una voz está libre o en Release, la asignación mira su envolvente y deja de llevar su propio "ocupada".
- SysTick avanza las cuatro envolventes cada 1 ms, y soltar una tecla pasa su voz a Release en vez de apagarla. La mezcla usa el nivel de la envolvente y se retira la amplitud fija. Una voz que suena y se reutiliza no vuelve a fase 0, y la ISR la sigue viendo completa con el esquema de la story 7.
- Probar en placa con y sin depurador y leer los underruns. Después, guardar la evidencia y marcar en la spec los criterios 1, 2 y 5 del Bloque 7.

## Fuera de alcance
- Potenciómetros y ADC (burst, filtro y mapeo a 2–2000 ms y 0–100 %): story 10. Acá los parámetros son fijos.
- Que mover un parámetro mientras suena una nota no genere saltos (criterio 4 del bloque): story 10.
- Suavizar el nivel dentro del bloque de audio: cambia cada 1 ms, como pide §3.4.
- El click cuando una tecla vuelve a sonar con otra onda activa: la nota toma la onda nueva al arrancar (§3.8).
- Mensajes de ADSR por instrumento (Bloque 8), prueba prolongada (B14) y contacto intermitente.

## Sugerencias (aceptadas)
- **Paso 3 sin click.** Hoy reemplazar a la voz más antigua hace click (story 7). Si la nota nueva toma la voz con la misma regla que la reutilización (sigue desde la fase y el nivel que tenía), el click desaparece por el mismo camino de código. Registrada en §3.2 y en el criterio 3.
- **Estado de las envolventes legible en Expressions**, como `activeWave` y `keyPressCount`: `envStage[4]` y `envLevel[4]`, globales `volatile` que escribe SysTick. Así hay evidencia de la voz libre (criterio 2) y de qué voz se calló (criterio 3) aunque el oído dude.

## Preparación de hardware (laboratorio)
- Lo mismo que en la story 8: placa por USB, las ocho teclas en P2.0–P2.7, la etapa de audio del Bloque 3 con los 5 V y masa común, y los auriculares al mínimo al conectar. El pulsador de EINT1 puede quedar armado, pero no se usa.
- Todavía sin potenciómetros.
