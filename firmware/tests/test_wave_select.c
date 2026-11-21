/* Tests de la selección de onda con EINT1 (spec 3.8 y 3.11, Bloque 6).
 * Rotación: cuadrada, sierra, triangular, senoidal y Python (se saltea si no llegó ninguna). La placa arranca
 * en senoide. Una pulsación cuenta si pasaron 150 ms desde la última aceptada y el pulsador se soltó (10 ms
 * seguidos en alto) desde entonces: una pulsación mantenida cuenta una sola vez y el rebote al soltar no suma. */
#include <stdio.h>
#include <stdint.h>
#include "wave_select.h"
#include "test_check.h"

#define START_MS  1000    /* hora de la primera pulsación de los tests: lejos del arranque */

static void testArrancaEnSenoide(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectCurrent(&s) == WAVE_SINE);
}

/* Sin onda de Python (pythonLoaded = 0): de la senoide, la primera pulsación va a la cuadrada */
static void testLaPrimeraPulsacionVaALaCuadrada(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    CHECK(waveSelectCurrent(&s) == WAVE_SQUARE);
}

/* El pulsador sigue apretado y llegan más flancos descendentes (ruido, rebote): ninguno suma, sin
 * importar cuánto tiempo pasó. Solo cuenta la primera pulsación */
static void testUnaPulsacionMantenidaMasDe150msCuentaUnaSola(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    CHECK(waveSelectPress(&s, START_MS + 200, 0) == 0);
    CHECK(waveSelectPress(&s, START_MS + 1000, 0) == 0);
    CHECK(waveSelectPress(&s, START_MS + 60000, 0) == 0);
    CHECK(waveSelectCurrent(&s) == WAVE_SQUARE);
}

#define RELEASE_MS  10    /* ms seguidos en alto para dar el pulsador por soltado (spec 3.11) */

/* SysTick lee el pin cada 1 ms: el pulsador pasa 'ms' lecturas seguidas en el nivel dado (1 = alto, suelto) */
static void holdPin(WaveSelect *s, uint32_t ms, uint8_t pinHigh) {
    for (uint32_t i = 0; i < ms; i++) {
        waveSelectSample(s, pinHigh);
    }
}

/* Con 9 ms seguidos en alto todavía no está soltado; con el décimo, sí. Y en la segunda ronda la cuenta
 * arranca de nuevo: los 10 ms de la ronda anterior no valen para la pulsación nueva. Las pulsaciones están
 * a más de 150 ms una de otra, así que la ventana de 150 ms no interviene */
static void testNoSeRearmaHastaQueElPulsadorSueltaYQuedeEstableEnAlto(void) {
    WaveSelect s;
    waveSelectInit(&s);

    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    holdPin(&s, RELEASE_MS - 1, 1);
    CHECK(waveSelectPress(&s, START_MS + 1000, 0) == 0);
    holdPin(&s, 1, 1);
    CHECK(waveSelectPress(&s, START_MS + 2000, 0) == 1);

    holdPin(&s, RELEASE_MS - 1, 1);
    CHECK(waveSelectPress(&s, START_MS + 3000, 0) == 0);
    holdPin(&s, 1, 1);
    CHECK(waveSelectPress(&s, START_MS + 4000, 0) == 1);
}

/* Pulsador apretado más de 150 ms que se suelta con rebote: el pin sube 6 ms, baja un instante (otro flanco
 * descendente), vuelve a subir 6 ms y rebota otra vez. Suma 12 ms en alto, pero nunca 10 seguidos, así que
 * ninguno de los dos flancos cuenta. Recién con 10 ms seguidos en alto el pulsador queda soltado */
static void testElReboteAlSoltarNoCuentaComoPulsacion(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);

    holdPin(&s, 6, 1);
    CHECK(waveSelectPress(&s, START_MS + 500, 0) == 0);
    holdPin(&s, 1, 0);
    holdPin(&s, 6, 1);
    CHECK(waveSelectPress(&s, START_MS + 510, 0) == 0);
    holdPin(&s, 1, 0);

    holdPin(&s, RELEASE_MS, 1);
    CHECK(waveSelectPress(&s, START_MS + 600, 0) == 1);
}

#define LOCKOUT_MS  150   /* ventana desde la última pulsación aceptada (spec 3.11) */

/* El pulsador ya se soltó (10 ms en alto), pero pasaron menos de 150 ms desde la pulsación anterior */
static void testUnaPulsacionAMenosDe150msDeLaAnteriorSeIgnora(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    holdPin(&s, RELEASE_MS, 1);
    CHECK(waveSelectPress(&s, START_MS + LOCKOUT_MS - 1, 0) == 0);
}

/* "A menos de 150 ms" se ignora: a exactamente 150 ms ya se acepta */
static void testUnaPulsacionA150msDeLaAnteriorSeAcepta(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    holdPin(&s, RELEASE_MS, 1);
    CHECK(waveSelectPress(&s, START_MS + LOCKOUT_MS, 0) == 1);
}

/* La ventana se cuenta desde la última pulsación aceptada: una ignorada a los 100 ms no la corre, y a los
 * 150 ms de la aceptada (solo 50 de la ignorada) la pulsación siguiente se acepta */
static void testUnaPulsacionIgnoradaNoExtiendeLaVentana(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, START_MS, 0) == 1);
    holdPin(&s, RELEASE_MS, 1);

    CHECK(waveSelectPress(&s, START_MS + 100, 0) == 0);
    holdPin(&s, 20, 0);                      /* ese segundo toque se mantiene apretado un rato */
    holdPin(&s, RELEASE_MS, 1);

    CHECK(waveSelectPress(&s, START_MS + LOCKOUT_MS, 0) == 1);
}

/* Antes de la primera pulsación no hay una anterior de la que contar los 150 ms: con el contador de
 * milisegundos en 0 (recién encendida) o a menos de 150 ms del arranque, se acepta igual */
static void testLaPrimeraPulsacionTrasElArranqueSeAceptaConElContadorEnCero(void) {
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, 0, 0) == 1);

    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, LOCKOUT_MS - 1, 0) == 1);
}

/* El contador de milisegundos es de 32 bits y da la vuelta a los ~49 días. La primera pulsación es 50 ms antes
 * de la vuelta: a los 10 ms el contador todavía no la dio, a los 149 ya la dio (vale 99) y a los 150 vale 100.
 * Los 150 ms se miden igual en los tres casos */
static void testLaVentanaSigueBienCuandoElContadorDeMilisegundosDaLaVuelta(void) {
    const uint32_t beforeWrap = 0xFFFFFFFFu - 49u;     /* 2^32 - 50 */
    WaveSelect s;
    waveSelectInit(&s);
    CHECK(waveSelectPress(&s, beforeWrap, 0) == 1);
    holdPin(&s, RELEASE_MS, 1);

    CHECK(waveSelectPress(&s, beforeWrap + 10u, 0) == 0);
    CHECK(waveSelectPress(&s, beforeWrap + (LOCKOUT_MS - 1u), 0) == 0);
    CHECK(waveSelectPress(&s, beforeWrap + LOCKOUT_MS, 0) == 1);
}

/* Un toque completo: el flanco descendente a la hora nowMs y después el pulsador suelto. Devuelve lo que
 * devolvió la pulsación (1 = se aceptó) */
static uint8_t tapAt(WaveSelect *s, uint32_t nowMs, uint8_t pythonLoaded) {
    const uint8_t accepted = waveSelectPress(s, nowMs, pythonLoaded);
    holdPin(s, RELEASE_MS, 1);
    return accepted;
}

#define TAP_SPACING_MS  200   /* entre toques: más que la ventana de 150 ms */

/* El orden de la spec 3.8 sin onda de Python: de la senoide a la cuadrada, y de ahí en círculo */
static void testLasPulsacionesRecorrenCuadradaSierraTriangularSenoidalYVuelven(void) {
    static const uint8_t expected[5] = {WAVE_SQUARE, WAVE_SAW, WAVE_TRIANGLE, WAVE_SINE, WAVE_SQUARE};
    WaveSelect s;
    waveSelectInit(&s);
    for (uint32_t i = 0; i < 5; i++) {
        CHECK(tapAt(&s, START_MS + TAP_SPACING_MS * i, 0) == 1);
        CHECK(waveSelectCurrent(&s) == expected[i]);
    }
}

/* Con una onda de Python cargada (pythonLoaded = 1), la quinta posición entra entre la senoidal y la cuadrada:
 * la placa arranca en senoide, así que la primera pulsación va a Python y de ahí vuelve a la cuadrada */
static void testConOndaDePythonLaRotacionIncluyeLaQuintaPosicion(void) {
    static const uint8_t expected[6] = {WAVE_PYTHON, WAVE_SQUARE, WAVE_SAW, WAVE_TRIANGLE, WAVE_SINE, WAVE_PYTHON};
    WaveSelect s;
    waveSelectInit(&s);
    for (uint32_t i = 0; i < 6; i++) {
        CHECK(tapAt(&s, START_MS + TAP_SPACING_MS * i, 1) == 1);
        CHECK(waveSelectCurrent(&s) == expected[i]);
    }
}

/* Sin onda de Python (pythonLoaded = 0) la rotación nunca cae en la quinta posición, en tres vueltas seguidas */
static void testSinOndaDePythonSeSalteaLaQuintaPosicion(void) {
    static const uint8_t expected[4] = {WAVE_SQUARE, WAVE_SAW, WAVE_TRIANGLE, WAVE_SINE};
    WaveSelect s;
    waveSelectInit(&s);
    for (uint32_t i = 0; i < 3 * 4; i++) {
        CHECK(tapAt(&s, START_MS + TAP_SPACING_MS * i, 0) == 1);
        CHECK(waveSelectCurrent(&s) == expected[i % 4]);
    }
}

/* Simulación de un pulsador real: el tiempo avanza de a 1 ms, SysTick lee el pin en cada milisegundo y cada
 * flanco descendente del pin llama a la ISR de EINT1 */
typedef struct {
    WaveSelect s;
    uint32_t nowMs;
    uint8_t pinHigh;     /* nivel de P2.11: 1 = suelto */
    uint32_t edges;      /* flancos descendentes que llegaron a la ISR */
    uint32_t accepted;   /* los que la lógica aceptó como pulsación */
} ButtonSim;

static void simPin(ButtonSim *b, uint8_t pinHigh) {
    if (b->pinHigh && !pinHigh) {
        b->edges++;
        b->accepted += waveSelectPress(&b->s, b->nowMs, 0);
    }
    b->pinHigh = pinHigh;
}

static void simWait(ButtonSim *b, uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        b->nowMs++;
        waveSelectSample(&b->s, b->pinHigh);
    }
}

/* Una pulsación completa: cinco flancos descendentes (el primero, dos rebotes al apretar y dos al soltar),
 * apretado holdMs ms en el medio, y después el pulsador suelto y estable 200 ms */
static void simPush(ButtonSim *b, uint32_t holdMs) {
    simPin(b, 0); simWait(b, 1);
    simPin(b, 1); simWait(b, 1);          /* rebote al apretar */
    simPin(b, 0); simWait(b, 2);
    simPin(b, 1); simWait(b, 1);
    simPin(b, 0); simWait(b, holdMs);     /* queda apretado */
    simPin(b, 1); simWait(b, 2);          /* rebote al soltar */
    simPin(b, 0); simWait(b, 1);
    simPin(b, 1); simWait(b, 3);
    simPin(b, 0); simWait(b, 1);
    simPin(b, 1); simWait(b, 200);        /* suelto y estable */
}

/* Veinte pulsaciones, con tiempos apretado de 5 ms a 2 s (alrededor de los 150 ms de la ventana también) y
 * rebote al apretar y al soltar: 100 flancos descendentes, 20 aceptados. Con cuatro posiciones, 20 pulsaciones
 * son cinco vueltas completas: la onda vuelve a la senoide */
static void testVeintePulsacionesConRebotesAvanzanExactamenteVeintePosiciones(void) {
    static const uint32_t holdMs[20] = {20, 80, 140, 149, 150, 151, 160, 300, 1000, 5,
                                        30, 90, 120, 250, 400, 700, 2000, 10, 60, 180};
    ButtonSim b = {0};
    waveSelectInit(&b.s);
    b.nowMs = START_MS;
    b.pinHigh = 1;

    for (uint32_t i = 0; i < 20; i++) {
        simPush(&b, holdMs[i]);
        CHECK(b.accepted == i + 1);
    }
    CHECK(b.edges == 20 * 5);
    CHECK(waveSelectCurrent(&b.s) == WAVE_SINE);
}

int main(void) {
    testArrancaEnSenoide();
    testLaPrimeraPulsacionVaALaCuadrada();
    testUnaPulsacionMantenidaMasDe150msCuentaUnaSola();
    testNoSeRearmaHastaQueElPulsadorSueltaYQuedeEstableEnAlto();
    testElReboteAlSoltarNoCuentaComoPulsacion();
    testUnaPulsacionAMenosDe150msDeLaAnteriorSeIgnora();
    testUnaPulsacionA150msDeLaAnteriorSeAcepta();
    testUnaPulsacionIgnoradaNoExtiendeLaVentana();
    testLaPrimeraPulsacionTrasElArranqueSeAceptaConElContadorEnCero();
    testLaVentanaSigueBienCuandoElContadorDeMilisegundosDaLaVuelta();
    testLasPulsacionesRecorrenCuadradaSierraTriangularSenoidalYVuelven();
    testConOndaDePythonLaRotacionIncluyeLaQuintaPosicion();
    testSinOndaDePythonSeSalteaLaQuintaPosicion();
    testVeintePulsacionesConRebotesAvanzanExactamenteVeintePosiciones();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
