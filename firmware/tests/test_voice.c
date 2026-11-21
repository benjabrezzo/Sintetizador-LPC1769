/* Tests de la asignación de voces (spec 3.2 y 3.11, Bloques 6 y 7).
 * Las teclas se numeran de 0 a 7 (la tecla k + 1 de la spec) y la nota es el índice de noteSteps:
 * Do4 = 0 ... Do5 = 7. Una voz está libre, en Release o apretada según la etapa de su envolvente: el asignador la
 * mira y no lleva su propio "ocupada". Con las cuatro apretadas, la quinta tecla reemplaza a la voz más antigua. */
#include <stdio.h>
#include <stdint.h>
#include "config.h"
#include "envelope.h"
#include "voice.h"
#include "test_check.h"

/* El asignador y las cuatro voces, como los usa SysTick: apretar una tecla elige una voz y arranca su envolvente;
 * soltarla la pasa a Release */
typedef struct {
    VoiceAllocator alloc;
    Voice voices[NUM_VOICES];
} Rig;

static void rigInit(Rig *r) {
    voiceAllocInit(&r->alloc);
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        envInit(&r->voices[v].env);
    }
}

static VoiceAssignment rigPress(Rig *r, uint8_t key) {
    const VoiceAssignment assignment = voiceAllocPress(&r->alloc, key, r->voices);
    envNoteOn(&r->voices[assignment.voice].env);
    return assignment;
}

static uint8_t rigRelease(Rig *r, uint8_t key) {
    const uint8_t voice = voiceAllocRelease(&r->alloc, key, r->voices);
    if (voice != VOICE_NONE) {
        envNoteOff(&r->voices[voice].env);
    }
    return voice;
}

/* Parámetros fijos de la envolvente con los que se prueban las voces: los de la story 9 */
#define TEST_ATTACK_MS        300
#define TEST_DECAY_MS         300
#define TEST_SUSTAIN_PERCENT  50
#define TEST_RELEASE_MS       1000

/* El Sustain de EnvParams es un nivel Q15: los tests lo dan en porcentaje, como la spec, y lo pasan a Q15 al armar los
 * parámetros */
static int16_t sustainFromPercent(uint8_t percent) {
    return (int16_t)(ENV_LEVEL_MAX * percent / 100);
}

/* Avanza las envolventes de las cuatro voces la cantidad de pasos (1 ms) indicada, con los parámetros de prueba */
static void rigAdvance(Rig *r, uint32_t steps) {
    const EnvParams p = {TEST_ATTACK_MS, TEST_DECAY_MS, sustainFromPercent(TEST_SUSTAIN_PERCENT), TEST_RELEASE_MS};
    for (uint32_t i = 0; i < steps; i++) {
        for (uint32_t v = 0; v < NUM_VOICES; v++) {
            envStep(&r->voices[v].env, &p);
        }
    }
}

/* Cada tecla, apretada sola, toma la primera voz libre (la 0) y toca su nota: tecla 1 = Do4 (0) ... tecla 8 = Do5 (7) */
static void testCadaTeclaSolaTomaUnaVozLibreConSuNota(void) {
    for (uint8_t key = 0; key < 8; key++) {
        Rig r;
        rigInit(&r);

        const VoiceAssignment assignment = rigPress(&r, key);
        CHECK(assignment.voice == 0);
        CHECK(assignment.note == key);
    }
}

/* Cuatro teclas apretadas a la vez, en un orden que no es el de las teclas (5, 1, 8 y 4 de la spec), ocupan
 * cuatro voces distintas del rango válido, cada una con su nota */
static void testCuatroTeclasOcupanCuatroVocesDistintas(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};
    Rig r;
    rigInit(&r);

    uint32_t usedVoices = 0;               /* bit v = 1: la voz v ya se asignó */
    for (uint32_t i = 0; i < NUM_VOICES; i++) {
        const VoiceAssignment assignment = rigPress(&r, keys[i]);
        CHECK(assignment.voice < NUM_VOICES);
        CHECK(assignment.note == keys[i]);
        CHECK((usedVoices & (1u << assignment.voice)) == 0);    /* ninguna voz se repite */
        usedVoices |= 1u << assignment.voice;
    }
    CHECK(usedVoices == (1u << NUM_VOICES) - 1);                /* y quedaron ocupadas las cuatro */
}

/* Con cuatro teclas apretadas, soltar una devuelve la voz de esa tecla y solo esa: las otras tres siguen
 * apretadas. Se prueba con cada una de las cuatro. Para ver que las otras siguen apretadas, una tecla nueva
 * tiene que tomar justo la voz que se soltó, que queda en Release (si se hubiera soltado otra, tomaría otra) */
static void testSoltarUnaTeclaApagaSoloSuVoz(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};
    const uint8_t newKey = 1;

    for (uint32_t released = 0; released < NUM_VOICES; released++) {
        Rig r;
        rigInit(&r);
        uint8_t voiceOfKey[NUM_VOICES];
        for (uint32_t i = 0; i < NUM_VOICES; i++) {
            voiceOfKey[i] = rigPress(&r, keys[i]).voice;
        }

        CHECK(rigRelease(&r, keys[released]) == voiceOfKey[released]);
        CHECK(rigPress(&r, newKey).voice == voiceOfKey[released]);
    }
}

/* Con las cuatro voces ocupadas, la quinta tecla (la 2 de la spec) toma la voz de la tecla que se apretó primero
 * y toca su nota. Las otras tres teclas siguen con sus voces, y la nueva queda dueña de la voz reemplazada */
static void testLaQuintaTeclaReemplazaALaVozMasAntigua(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};
    const uint8_t fifthKey = 1;
    Rig r;
    rigInit(&r);
    uint8_t voiceOfKey[NUM_VOICES];
    for (uint32_t i = 0; i < NUM_VOICES; i++) {
        voiceOfKey[i] = rigPress(&r, keys[i]).voice;
    }

    const VoiceAssignment fifth = rigPress(&r, fifthKey);
    CHECK(fifth.voice == voiceOfKey[0]);                  /* la más antigua: la primera en apretarse */
    CHECK(fifth.note == fifthKey);

    for (uint32_t i = 1; i < NUM_VOICES; i++) {           /* las otras tres siguen con su voz */
        CHECK(rigRelease(&r, keys[i]) == voiceOfKey[i]);
    }
    CHECK(rigRelease(&r, fifthKey) == voiceOfKey[0]);     /* la nueva es dueña de la voz reemplazada */
}

/* La más antigua se decide por el orden en que se apretaron las teclas, no por el número de voz. Con las
 * teclas A, B, C y D en las voces 0 a 3 (en ese orden), se suelta A y entra E, que toma la voz 0 y pasa a
 * ser la más nueva: la más antigua es ahora B (voz 1). Entonces F reemplaza a B, y G, que llega después,
 * a C: la tecla que reemplazó cuenta como la más nueva, no conserva la antigüedad de la que tenía la voz */
static void testLaMasAntiguaSeDecidePorOrdenDePulsacionNoPorNumeroDeVoz(void) {
    const uint8_t keyA = 4, keyB = 0, keyC = 7, keyD = 3, keyE = 1, keyF = 5, keyG = 2;
    Rig r;
    rigInit(&r);
    const uint8_t voiceA = rigPress(&r, keyA).voice;
    const uint8_t voiceB = rigPress(&r, keyB).voice;
    const uint8_t voiceC = rigPress(&r, keyC).voice;
    const uint8_t voiceD = rigPress(&r, keyD).voice;

    CHECK(rigRelease(&r, keyA) == voiceA);
    CHECK(rigPress(&r, keyE).voice == voiceA);   /* A quedó en Release: es la única que no está apretada */
    CHECK(rigPress(&r, keyF).voice == voiceB);   /* B es la más antigua, no la voz 0 */
    CHECK(rigPress(&r, keyG).voice == voiceC);   /* y ahora C: F no heredó la antigüedad de B */

    CHECK(rigRelease(&r, keyD) == voiceD);       /* D, E, F y G conservan sus voces */
    CHECK(rigRelease(&r, keyE) == voiceA);
    CHECK(rigRelease(&r, keyF) == voiceB);
    CHECK(rigRelease(&r, keyG) == voiceC);
}

/* La tecla A pierde su voz porque entra una quinta (E) y la reemplaza. Al soltar A no hay nada que apagar
 * (VOICE_NONE) y la nota de E no se corta: E sigue siendo dueña de la voz, que no quedó libre. Se ve porque
 * una tecla nueva (F) no la encuentra libre y reemplaza a la más antigua que queda, B, y porque soltar E
 * devuelve esa voz */
static void testSoltarLaTeclaQuePerdioSuVozNoApagaLaNotaNueva(void) {
    const uint8_t keyA = 4, keyB = 0, keyC = 7, keyD = 3, keyE = 1, keyF = 5;
    Rig r;
    rigInit(&r);
    const uint8_t voiceA = rigPress(&r, keyA).voice;
    const uint8_t voiceB = rigPress(&r, keyB).voice;
    rigPress(&r, keyC);
    rigPress(&r, keyD);
    CHECK(rigPress(&r, keyE).voice == voiceA);   /* E reemplaza a A, la más antigua */

    CHECK(rigRelease(&r, keyA) == VOICE_NONE);   /* A ya no tiene voz: no se apaga nada */
    CHECK(rigPress(&r, keyF).voice == voiceB);   /* ninguna voz quedó libre: F reemplaza a B */
    CHECK(rigRelease(&r, keyE) == voiceA);       /* E conservó su voz */
}

/* Soltar una tecla que no tiene voz no apaga nada y no cambia a las demás: ni una que nunca se apretó, ni una
 * que ya se soltó (soltarla dos veces no puede liberar una voz de otra tecla ni la misma dos veces) */
static void testSoltarUnaTeclaSinVozNoHaceNada(void) {
    const uint8_t keyA = 4, keyB = 0, neverPressed = 6;
    Rig r;
    rigInit(&r);
    CHECK(rigRelease(&r, neverPressed) == VOICE_NONE);       /* con todas las voces libres */

    const uint8_t voiceA = rigPress(&r, keyA).voice;
    const uint8_t voiceB = rigPress(&r, keyB).voice;
    CHECK(rigRelease(&r, neverPressed) == VOICE_NONE);       /* con dos voces ocupadas */

    CHECK(rigRelease(&r, keyA) == voiceA);
    CHECK(rigRelease(&r, keyA) == VOICE_NONE);               /* ya estaba suelta */
    CHECK(rigRelease(&r, keyB) == voiceB);                   /* B no se enteró */
}

/* Mientras haya una voz libre, la tecla siguiente no reemplaza a nadie: con 1, 2, 3 y 4 teclas apretadas, cada
 * una conserva su voz (soltarla devuelve la que tomó al apretarse). Con 4 teclas y una voz libre no hay
 * ningún caso aparte: el reemplazo empieza recién con la quinta */
static void testConUnaVozLibreLaSiguienteTeclaNoReemplazaANadie(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};

    for (uint32_t pressedKeys = 1; pressedKeys <= NUM_VOICES; pressedKeys++) {
        Rig r;
        rigInit(&r);
        uint8_t voiceOfKey[NUM_VOICES];
        for (uint32_t i = 0; i < pressedKeys; i++) {
            voiceOfKey[i] = rigPress(&r, keys[i]).voice;
        }

        for (uint32_t i = 0; i < pressedKeys; i++) {
            CHECK(rigRelease(&r, keys[i]) == voiceOfKey[i]);
        }
    }
}

/* Una voz libre se elige antes que una en Release (spec 3.2), aunque la que se apaga tenga el número menor.
 * Caso 1: con A en la voz 0 y B en la 1, se suelta A (queda en Release) y entra C: toma una voz que nunca sonó, y la
 * nota de A sigue apagándose, sin cortarse.
 * Caso 2: la voz que terminó su Release queda libre. Se suelta B (voz 1) y, 600 ms después, A (voz 0). Cuando la
 * de B ya terminó (1000 ms) y la de A todavía se apaga, entra E: toma la voz libre de B, no la de A. */
static void testUnaVozLibreSeEligeAntesQueUnaEnRelease(void) {
    const uint8_t keyA = 4, keyB = 0, keyC = 7, keyD = 3, keyE = 1;

    {   /* caso 1 */
        Rig r;
        rigInit(&r);
        const uint8_t voiceA = rigPress(&r, keyA).voice;
        const uint8_t voiceB = rigPress(&r, keyB).voice;
        CHECK(rigRelease(&r, keyA) == voiceA);

        const uint8_t voiceC = rigPress(&r, keyC).voice;

        CHECK(voiceC != voiceA && voiceC != voiceB);
        CHECK(r.voices[voiceA].env.stage == ENV_RELEASE);        /* la nota de A sigue apagándose */
    }

    {   /* caso 2 */
        Rig r;
        rigInit(&r);
        const uint8_t voiceA = rigPress(&r, keyA).voice;
        const uint8_t voiceB = rigPress(&r, keyB).voice;
        rigPress(&r, keyC);
        rigPress(&r, keyD);
        CHECK(voiceA < voiceB);                                  /* la voz de A tiene el número menor */
        CHECK(rigRelease(&r, keyB) == voiceB);
        rigAdvance(&r, 600u);
        CHECK(rigRelease(&r, keyA) == voiceA);
        rigAdvance(&r, TEST_RELEASE_MS + 1u - 600u);             /* 1001 ms desde que se soltó B */
        CHECK(envIsIdle(&r.voices[voiceB].env));                 /* B terminó: su voz está libre */
        CHECK(r.voices[voiceA].env.stage == ENV_RELEASE);        /* A todavía se apaga */

        CHECK(rigPress(&r, keyE).voice == voiceB);
        CHECK(r.voices[voiceA].env.stage == ENV_RELEASE);        /* y la nota de A sigue sin cortarse */
    }
}

/* Aprieta cuatro teclas (A, B, C y D en las voces 0 a 3), las deja llegar al Sustain, suelta la tecla keys[first] y
 * 300 ms después la keys[second], espera 100 ms más y aprieta una quinta. Las dos soltadas bajan desde el mismo nivel,
 * y la primera lleva más tiempo, así que tiene el nivel menor: la nota nueva tiene que tomar su voz. La otra sigue
 * apagándose y las dos apretadas no se cortan: siguen en el Sustain. */
static void checkNewKeyTakesLowestOfTwoReleasing(uint32_t first, uint32_t second) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};
    const uint8_t newKey = 1;
    Rig r;
    rigInit(&r);
    uint8_t voiceOfKey[NUM_VOICES];
    for (uint32_t i = 0; i < NUM_VOICES; i++) {
        voiceOfKey[i] = rigPress(&r, keys[i]).voice;
    }
    rigAdvance(&r, 700u);                                    /* las cuatro llegaron al Sustain */
    CHECK(rigRelease(&r, keys[first]) == voiceOfKey[first]);
    rigAdvance(&r, 300u);
    CHECK(rigRelease(&r, keys[second]) == voiceOfKey[second]);
    rigAdvance(&r, 100u);
    CHECK(r.voices[voiceOfKey[first]].env.level < r.voices[voiceOfKey[second]].env.level);   /* la premisa */

    CHECK(rigPress(&r, newKey).voice == voiceOfKey[first]);

    CHECK(r.voices[voiceOfKey[second]].env.stage == ENV_RELEASE);
    for (uint32_t i = 0; i < NUM_VOICES; i++) {
        if (i != first && i != second) {
            CHECK(r.voices[voiceOfKey[i]].env.stage == ENV_SUSTAIN);
        }
    }
}

/* Sin voces libres, la nota nueva toma la voz en Release con menor nivel (spec 3.2). Se prueba que sea la de menor
 * nivel y no otra regla más fácil:
 * - Casos 1 y 2: dos voces en Release, y la de menor nivel es la de número mayor (caso 1: se soltó primero B, la voz 1) o la de
 *   número menor (caso 2: primero A, la voz 0). Así no vale ni elegir la primera ni la última que se encuentra.
 * - Caso 3: la que lleva más tiempo en Release no es la de menor nivel. C se suelta desde el Sustain (nivel alto) y, 30 ms
 *   más tarde, D, que arrancó después, se suelta cuando recién había subido un poco en el Attack (nivel bajo). D se
 *   suelta más tarde pero se apaga más abajo: E toma la voz de D. */
static void testSinVocesLibresSeTomaLaVozEnReleaseConMenorNivel(void) {
    checkNewKeyTakesLowestOfTwoReleasing(1, 0);   /* caso 1: B y después A */
    checkNewKeyTakesLowestOfTwoReleasing(0, 1);   /* caso 2: A y después B */

    {   /* caso 3 */
        const uint8_t keyA = 4, keyB = 0, keyC = 7, keyD = 3, keyE = 1;
        Rig r;
        rigInit(&r);
        rigPress(&r, keyA);
        rigPress(&r, keyB);
        const uint8_t voiceC = rigPress(&r, keyC).voice;
        rigAdvance(&r, 700u);                                /* A, B y C llegaron al Sustain */
        CHECK(rigRelease(&r, keyC) == voiceC);
        rigAdvance(&r, 20u);
        const uint8_t voiceD = rigPress(&r, keyD).voice;     /* la voz 3: la única libre */
        rigAdvance(&r, 10u);
        CHECK(rigRelease(&r, keyD) == voiceD);
        rigAdvance(&r, 30u);
        CHECK(r.voices[voiceD].env.level < r.voices[voiceC].env.level);   /* la premisa */

        CHECK(rigPress(&r, keyE).voice == voiceD);
        CHECK(r.voices[voiceC].env.stage == ENV_RELEASE);    /* la de C sigue apagándose */
    }
}

/* Mientras haya una voz en Release, la nota nueva no corta a ninguna apretada, aunque la más antigua esté apretada
 * (spec 3.2: la más antigua es el último recurso). Se suelta cada una de las cuatro teclas: soltando la 1, la 2 o la 3
 * la voz más antigua (la 0) sigue apretada y sería la elegida si el Release no tuviera prioridad. La nota nueva toma la
 * voz de la tecla soltada, y las otras tres no se cortan: siguen en el Sustain sin reiniciarse y sus teclas siguen
 * siendo las dueñas de sus voces. */
static void testLasVocesApretadasNoSeCortanMientrasHayaUnaEnRelease(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};
    const uint8_t newKey = 1;

    for (uint32_t released = 0; released < NUM_VOICES; released++) {
        Rig r;
        rigInit(&r);
        uint8_t voiceOfKey[NUM_VOICES];
        for (uint32_t i = 0; i < NUM_VOICES; i++) {
            voiceOfKey[i] = rigPress(&r, keys[i]).voice;
        }
        rigAdvance(&r, 700u);                                /* las cuatro llegaron al Sustain */
        CHECK(rigRelease(&r, keys[released]) == voiceOfKey[released]);
        rigAdvance(&r, 50u);

        CHECK(rigPress(&r, newKey).voice == voiceOfKey[released]);

        for (uint32_t i = 0; i < NUM_VOICES; i++) {
            if (i != released) {
                CHECK(r.voices[voiceOfKey[i]].env.stage == ENV_SUSTAIN);   /* no se cortó ni se reinició */
                CHECK(rigRelease(&r, keys[i]) == voiceOfKey[i]);           /* y su tecla sigue siendo la dueña */
            }
        }
    }
}

/* Una tecla que vuelve a sonar mientras su nota se apaga (su voz está en Release) usa esa misma voz (spec 3.2), tenga
 * lo que tenga alrededor. Se prueba que la reutilización gane a las otras dos reglas:
 * - Caso 1: A sola, soltada y vuelta a apretar a 1, 500 y 990 ms de soltarla (el Release dura 1000), con las otras tres
 *   voces libres: tiene que ser la misma voz, no una libre. Las demás siguen libres.
 * - Caso 2: B en la voz 0 y A en la 1. Con A de vuelta toma la 1, no una libre (la 2 o la 3), y B sigue apretada.
 * - Caso 3: sin voces libres, C (voz 2) se suelta primero y A (voz 0) después, así que la de C tiene el menor nivel.
 *   Cuando A vuelve toma su voz, no la de menor nivel. */
static void testUnaTeclaQueVuelveASonarUsaSuVozEnRelease(void) {
    const uint8_t keyA = 4, keyB = 0;
    const uint32_t delays[] = {1u, 500u, 990u};

    for (uint32_t i = 0; i < sizeof delays / sizeof delays[0]; i++) {          /* caso 1 */
        Rig r;
        rigInit(&r);
        const uint8_t voiceA = rigPress(&r, keyA).voice;
        rigAdvance(&r, 700u);                                /* en Sustain */
        CHECK(rigRelease(&r, keyA) == voiceA);
        rigAdvance(&r, delays[i]);

        const VoiceAssignment again = rigPress(&r, keyA);

        CHECK(again.voice == voiceA);
        CHECK(again.note == keyA);
        for (uint32_t v = 0; v < NUM_VOICES; v++) {
            if (v != voiceA) {
                CHECK(envIsIdle(&r.voices[v].env));          /* ninguna otra voz entró */
            }
        }
    }

    {   /* caso 2 */
        Rig r;
        rigInit(&r);
        const uint8_t voiceB = rigPress(&r, keyB).voice;
        const uint8_t voiceA = rigPress(&r, keyA).voice;
        rigAdvance(&r, 700u);
        CHECK(rigRelease(&r, keyA) == voiceA);
        rigAdvance(&r, 100u);

        CHECK(rigPress(&r, keyA).voice == voiceA);
        CHECK(r.voices[voiceB].env.stage == ENV_SUSTAIN);    /* B sigue apretada */
    }

    {   /* caso 3 */
        const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};       /* A, B, C y D en las voces 0 a 3 */
        Rig r;
        rigInit(&r);
        uint8_t voiceOfKey[NUM_VOICES];
        for (uint32_t k = 0; k < NUM_VOICES; k++) {
            voiceOfKey[k] = rigPress(&r, keys[k]).voice;
        }
        rigAdvance(&r, 700u);
        CHECK(rigRelease(&r, keys[2]) == voiceOfKey[2]);     /* C primero */
        rigAdvance(&r, 300u);
        CHECK(rigRelease(&r, keys[0]) == voiceOfKey[0]);     /* A después */
        rigAdvance(&r, 100u);
        CHECK(r.voices[voiceOfKey[2]].env.level < r.voices[voiceOfKey[0]].env.level);   /* la premisa */

        CHECK(rigPress(&r, keys[0]).voice == voiceOfKey[0]);
        CHECK(r.voices[voiceOfKey[2]].env.stage == ENV_RELEASE);    /* la de C sigue apagándose */
    }
}

/* Si otra tecla se llevó la voz de la nota que se apagaba, la tecla que vuelve a sonar no la reutiliza: esa voz ya es de
 * la otra. Con A, B, C y D en las voces 0 a 3 se suelta A y entra E (sin voces libres): toma la voz 0, que estaba en
 * Release. E llega al Sustain y A vuelve a apretarse.
 * - Caso 1: sigue sin haber voces libres. A toma la apretada más antigua, B (voz 1), no la voz 0.
 * - Caso 2: antes de que vuelva A se suelta D y termina su Release, y queda una voz libre (la 3). A toma esa.
 * En los dos casos la nota de E no se corta (sigue en Sustain) y su tecla sigue siendo la dueña de la voz 0. */
static void testSiOtraTeclaSeLlevoLaVozQueSeApagabaLaTeclaTomaOtra(void) {
    const uint8_t keys[NUM_VOICES] = {4, 0, 7, 3};           /* A, B, C y D en las voces 0 a 3 */
    const uint8_t keyE = 1;

    for (uint32_t withFreeVoice = 0; withFreeVoice < 2; withFreeVoice++) {
        Rig r;
        rigInit(&r);
        uint8_t voiceOfKey[NUM_VOICES];
        for (uint32_t k = 0; k < NUM_VOICES; k++) {
            voiceOfKey[k] = rigPress(&r, keys[k]).voice;
        }
        rigAdvance(&r, 700u);
        CHECK(rigRelease(&r, keys[0]) == voiceOfKey[0]);     /* se suelta A */
        rigAdvance(&r, 100u);
        CHECK(rigPress(&r, keyE).voice == voiceOfKey[0]);    /* E se lleva la voz de A */
        rigAdvance(&r, 700u);                                /* E llega al Sustain */
        if (withFreeVoice) {
            CHECK(rigRelease(&r, keys[3]) == voiceOfKey[3]);
            rigAdvance(&r, TEST_RELEASE_MS + 1u);
            CHECK(envIsIdle(&r.voices[voiceOfKey[3]].env));  /* la voz 3 quedó libre */
        }

        const VoiceAssignment again = rigPress(&r, keys[0]); /* A vuelve a sonar */

        CHECK(again.voice != voiceOfKey[0]);                 /* no la voz de E */
        CHECK(again.voice == (withFreeVoice ? voiceOfKey[3] : voiceOfKey[1]));   /* la libre, o la apretada más antigua */
        CHECK(r.voices[voiceOfKey[0]].env.stage == ENV_SUSTAIN);   /* la nota de E no se cortó */
        CHECK(rigRelease(&r, keyE) == voiceOfKey[0]);        /* y E sigue siendo la dueña de su voz */
    }
}

/* voiceStart en una voz libre (spec 3.2): la nota arranca en fase 0 y nivel 0, con el incremento y la onda dados, y la
 * envolvente en Attack. A la voz le quedaron una fase y un incremento viejos de la nota anterior, así que el resultado
 * sale de voiceStart y no de lo que ya había. */
static void testVoiceStartEnUnaVozLibreArrancaEnFaseCeroYNivelCero(void) {
    const uint32_t step = 4096;           /* incremento de la nota nueva */
    const uint8_t wave = 2;               /* cualquier identificador de onda */
    Voice v;
    v.osc.phase = 0x00ABCDEFu;            /* fase vieja */
    v.osc.step = 777u;                    /* incremento viejo */
    v.wave = 0;
    envInit(&v.env);                      /* libre: terminó su Release */

    voiceStart(&v, step, wave);

    CHECK(v.osc.phase == 0);
    CHECK(v.env.level == 0);
    CHECK(v.osc.step == step);
    CHECK(v.wave == wave);
    CHECK(v.env.stage == ENV_ATTACK);
}

/* Arma una voz que suena: con una fase a medio camino de la tabla y un incremento viejo, y su envolvente avanzada
 * `steps` pasos con el Sustain dado; si releaseSteps > 0, se suelta ahí y sigue ese tiempo en Release. */
static Voice soundingVoice(uint8_t sustainPercent, uint32_t steps, uint32_t releaseSteps) {
    const EnvParams p = {TEST_ATTACK_MS, TEST_DECAY_MS, sustainFromPercent(sustainPercent), TEST_RELEASE_MS};
    Voice v = {.env = {.stage = ENV_IDLE}};                  /* el resto de los campos, en cero */
    v.osc.phase = 0x00123456u;
    v.osc.step = 777u;
    envNoteOn(&v.env);
    for (uint32_t i = 0; i < steps; i++) {
        envStep(&v.env, &p);
    }
    if (releaseSteps > 0) {
        envNoteOff(&v.env);
        for (uint32_t i = 0; i < releaseSteps; i++) {
            envStep(&v.env, &p);
        }
    }
    return v;
}

/* voiceStart sobre la voz dada: conserva la fase y el nivel que tenía, toma el incremento y la onda nuevos y la
 * envolvente pasa a Attack desde ese nivel. stageBefore es la etapa en que tiene que estar la voz antes de arrancar. */
static void checkStartKeepsPhaseAndLevel(Voice v, EnvStage stageBefore) {
    const uint32_t newStep = 4096;
    const uint8_t newWave = 2;
    CHECK(v.env.stage == stageBefore);                       /* la premisa: la voz está donde dice el caso */
    const uint32_t phaseBefore = v.osc.phase;
    const int16_t levelBefore = v.env.level;

    voiceStart(&v, newStep, newWave);

    CHECK(v.osc.phase == phaseBefore);
    CHECK(v.env.level == levelBefore);
    CHECK(v.osc.step == newStep);
    CHECK(v.wave == newWave);
    CHECK(v.env.stage == ENV_ATTACK);
}

/* voiceStart en una voz que suena (spec 3.2): la nota nueva sigue desde la fase y el nivel que tenía la voz, para
 * evitar clicks, y toma el incremento y la onda de la nota nueva. Vale para las tres formas de que la voz no esté
 * libre: la que se reutiliza o se lleva la de menor nivel (en Release), y la más antigua (apretada, en Attack o en
 * Sustain). El último caso es una voz apretada en silencio (Sustain 0 %): tiene nivel 0 pero está ocupada, así que
 * conserva la fase: "libre" lo decide la envolvente, no el nivel. */
static void testVoiceStartEnUnaVozQueSuenaConservaFaseYNivel(void) {
    checkStartKeepsPhaseAndLevel(soundingVoice(50, 150u, 0), ENV_ATTACK);          /* apretada, en pleno Attack */
    checkStartKeepsPhaseAndLevel(soundingVoice(50, 700u, 0), ENV_SUSTAIN);         /* apretada, en Sustain */
    checkStartKeepsPhaseAndLevel(soundingVoice(50, 700u, 400u), ENV_RELEASE);      /* apagándose */

    const Voice quiet = soundingVoice(0, 700u, 0);
    CHECK(quiet.env.level == 0);                             /* en silencio, y aun así ocupada */
    checkStartKeepsPhaseAndLevel(quiet, ENV_SUSTAIN);
}

int main(void) {
    testCadaTeclaSolaTomaUnaVozLibreConSuNota();
    testCuatroTeclasOcupanCuatroVocesDistintas();
    testSoltarUnaTeclaApagaSoloSuVoz();
    testLaQuintaTeclaReemplazaALaVozMasAntigua();
    testLaMasAntiguaSeDecidePorOrdenDePulsacionNoPorNumeroDeVoz();
    testSoltarLaTeclaQuePerdioSuVozNoApagaLaNotaNueva();
    testSoltarUnaTeclaSinVozNoHaceNada();
    testConUnaVozLibreLaSiguienteTeclaNoReemplazaANadie();
    testUnaVozLibreSeEligeAntesQueUnaEnRelease();
    testSinVocesLibresSeTomaLaVozEnReleaseConMenorNivel();
    testLasVocesApretadasNoSeCortanMientrasHayaUnaEnRelease();
    testUnaTeclaQueVuelveASonarUsaSuVozEnRelease();
    testSiOtraTeclaSeLlevoLaVozQueSeApagabaLaTeclaTomaOtra();
    testVoiceStartEnUnaVozLibreArrancaEnFaseCeroYNivelCero();
    testVoiceStartEnUnaVozQueSuenaConservaFaseYNivel();
    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
