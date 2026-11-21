# Story 12: Instrumentos precargados

**Bloque:** 8 · **Spec:** §3.3, §6 Bloque 8, §9 · **Estado:** terminada (ver `docs/evidencia/story-12-tests.md`)

## Story
Como músico, quiero partir de una configuración de guitarra, piano, órgano o flauta y saber cómo ajustar los potes de la ADSR, para acercarme al sonido de ese instrumento.

## Criterios de aceptación
- [x] Dado el editor abierto, cuando elijo guitarra, piano, órgano o flauta, entonces los 30 controles toman las amplitudes que la spec define para ese instrumento, y la onda y el espectro se actualizan con picos en esas proporciones.
- [x] Dado un instrumento elegido, entonces veo un mensaje que dice cómo dejar cada uno de los cuatro potes (Attack, Decay, Sustain y Release) para acercarme a su sonido, y el mensaje cambia al elegir otro instrumento.
- [x] Dado un instrumento cargado, cuando muevo un control o cambio la nota, entonces el editor se comporta como en la story 11: la nota no cambia al cargar y la onda resultante se exporta igual que cualquier otra.

## Plan
- Registrar primero en la spec (§3.3) y sacar el ítem de §9:
  - Las amplitudes salen de un modelo físico simple, salvo la flauta. Van en % enteros, que es lo que admiten los controles, para que el estado del editor sea exactamente el definido.
  - Guitarra: cuerda pulsada a 1/5 del largo, `|sin(kπ/5)|/k²` normalizado a la fundamental (100, 40, 18, 6, 0…; se anulan el 5, el 10, el 15…).
  - Piano: cuerda percutida a 1/7 del largo, `|sin(kπ/7)|/k` normalizado a la fundamental (100, 90, 75, 56, 36, 17, 0…; se anula el 7).
  - Órgano: tiradores 8', 4', 2⅔' y 2' al máximo, es decir, armónicos 1 a 4 al 100 % y el resto en 0.
  - Flauta: casi senoidal, con la fundamental al 100 %, el 2 al 20 %, el 3 al 5 % y el resto en 0.
  - Cada mensaje de ADSR da, para cada pote, la posición en el recorrido y el valor (por ejemplo, "Decay: tres cuartos del recorrido (≈ 1500 ms)"; el mapeo es lineal, así que la posición es más o menos t/2000):
    - Guitarra: A mín · D ≈ 1500 ms · S 0 % · R ≈ 200 ms.
    - Piano: A mín · D máx (2000 ms) · S 0 % · R ≈ 300 ms.
    - Órgano: A mín · D cualquiera · S 100 % · R mín.
    - Flauta: A ≈ 100 ms · D ≈ 200 ms · S ≈ 80 % · R ≈ 150 ms.
- Escribir los tests primero, sobre la lógica pura. Tienen que comprobar que cada instrumento carga sus 30 amplitudes, que el espectro de la tabla cuantizada respeta las proporciones (con la tolerancia de cuantización), que cada mensaje nombra los cuatro potes y que cargar un instrumento no cambia la nota.
- Guardar los instrumentos como datos (amplitudes y mensaje), con una operación del editor que carga uno. Esto va sin GUI.
- En la ventana, agregar un selector de los cuatro instrumentos que mueva los 30 controles y una zona fija con el mensaje de ADSR. Con un smoke test se comprueba que elegir un instrumento mueve los controles y muestra el mensaje.
- Evidencia:
  - Una captura por instrumento con la onda, el espectro y el mensaje.
  - Los tests pasando.
  - Marcar el criterio 6 del Bloque 8: el bloque queda aprobado.

## Fuera de alcance
- Escuchar los instrumentos, en la PC o en la placa. Recién se puede cuando la onda llegue por UART (story 13).
- Que la app cambie la ADSR de la placa: solo da indicaciones, y los potes siguen siendo la única forma de ajustarla.
- Agregar, editar o guardar instrumentos propios.
- Pedir confirmación antes de reemplazar las amplitudes actuales.
- Usar varias tablas por instrumento o hacer que el timbre cambie con el tiempo (Bloque 15).

## Notas
- No hace falta hardware: todo corre en la PC.
- Se elige un instrumento por su espectro, que es una tabla fija para las ocho teclas: es una aproximación. Si suena parecido se juzga a oído recién en la story 13.

## Sugerencias (tu decisión)
- **Usar botones en vez de una lista desplegable.** Cargar un instrumento es una acción y no un estado. Con una lista, después de editar seguiría diciendo "Guitarra" sobre una onda que ya no lo es.
- **Recalcular una sola vez al cargar.** Hoy cada control dispara `valueChanged` → `_refrescar`, así que cargar un instrumento recalcularía 30 veces. Bloqueando las señales mientras se cargan los valores, se refresca una sola vez.
