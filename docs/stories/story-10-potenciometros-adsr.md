# Story 10: Potenciómetros de la ADSR

**Bloque:** 7 · **Spec:** §3.4, §3.13, §6 Bloque 7 · **Estado:** cerrada con evidencia parcial (ver `docs/evidencia/story-10-potenciometros-adsr.md`)

## Story
Como músico, quiero ajustar con cuatro potenciómetros el ataque, decaimiento, sostenido y liberación, también mientras suena una nota, para dar forma a cada sonido.

## Criterios de aceptación
- [ ] Dados los cuatro potenciómetros armados, cuando giro cada uno de punta a punta, entonces cambia solo su parámetro: Attack, Decay y Release van de 2 a 2000 ms, y Sustain de 0 a 100 %. Se ve en el depurador en las dos puntas y se oye al tocar: ataque seco o lento, nota que se apaga sola o se sostiene al máximo, corte seco o cola larga. Con los potes quietos, una nota sostenida suena estable, sin vibración de volumen.
- [ ] Dada una nota sonando en cualquier etapa, cuando giro el pote de Attack, Decay o Release, incluso de golpe de punta a punta, entonces la nota sigue desde el volumen en que estaba, solo más rápido o más lenta, sin cortes ni saltos hacia atrás.
- [ ] Dada una nota sostenida, cuando giro el pote de Sustain, entonces su volumen sigue al pote sin clicks. En todas estas pruebas, con varias voces sonando, el nivel de las envolventes no sale de 0…32767 y los underruns siguen en 0.

## Plan
- Registrar primero en la spec (§3.4 y Bloque 7):
  - Cambiar A, D o R a mitad de una etapa conserva lo ya recorrido, y la etapa termina lo que le falta a la velocidad nueva.
  - El Sustain de una nota sostenida sigue al pote.
  - El mapeo es lineal.
  - Se retiran los valores fijos de la story 9.
- Escribir primero los tests y después la lógica pura del mapeo y del filtro:
  - La lectura de 12 bits se mapea linealmente a 2–2000 ms y a 0–100 %, con un margen en las puntas para que el recorrido real del pote llegue a los extremos.
  - El filtro sigue un escalón sin pasarse y atenúa el ruido de pocas cuentas.
- Escribir primero los tests y después la envolvente:
  - Cambiar A, D o R a mitad de cualquier etapa no hace retroceder el nivel ni lo adelanta más de lo que avanza la rampa a la velocidad nueva.
  - La etapa termina cuando corresponde según lo que le faltaba.
  - Cambiar S lleva el nivel del Decay y del Sustain hacia el valor nuevo.
  - El nivel queda siempre en 0…32767, y los tests de la story 9 siguen pasando.
- ADC en burst sobre AD0.0, AD0.1, AD0.2 y AD0.4, sin AD0.3 (es AOUT) y sin interrupción. SysTick lee los cuatro resultados, los filtra y los mapea antes de avanzar las envolventes.
- Probar en placa con y sin depurador: las puntas de cada pote, giros con notas en cada etapa, varias voces y los underruns. Después, guardar la evidencia y marcar los criterios 3 y 4 del Bloque 7, con lo que el bloque queda cerrado.

## Fuera de alcance
- Curva exponencial o logarítmica para los tiempos: el mapeo es lineal.
- Publicar los parámetros como globales para el depurador.
- Interrupción o DMA para el ADC: SysTick lee los resultados del burst.
- Suavizar el nivel dentro del bloque de audio: sigue cambiando cada 1 ms.
- Mensajes de ajuste de la ADSR por instrumento (Bloque 8).
- Presupuesto de la ISR (B10), prueba prolongada (B14) y contacto intermitente.

## Notas
- **Lectura en el depurador (criterio 1):** los parámetros siguen siendo `static` en main. GDB suele resolver por nombre las `static` de archivo en Expressions, pero no está verificado en este IDE. Si no aparecen, las puntas se validan con los tests del mapeo y de oído en placa.

## Preparación de hardware (laboratorio)
- Lo mismo que en la story 9: placa por USB, las teclas en P2.0–P2.7, la etapa de audio con 5 V y masa común, y los auriculares al mínimo al conectar.
- Cuatro potes de 10 k, con los extremos a 3,3 V y GND de la placa. Los cursores van a P0.23 (A), P0.24 (D), P0.25 (S) y P1.30 (R).
- ⚠️ Los potes nunca van a los 5 V de la etapa de audio: en modo ADC esos pines no toleran 5 V.
