/* Tests de la envolvente ADSR de una voz (spec 3.4, Bloque 7).
 * El nivel es Q15 (0 ... 32767) y la envolvente avanza un paso por milisegundo (SysTick). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "envelope.h"
#include "test_check.h"

/* Una envolvente recién inicializada está libre y en nivel 0. Se parte de memoria llena de 0xFF para que el
 * resultado salga de envInit y no de que la memoria ya estuviera en cero */
static void testUnaEnvolventeNuevaEstaLibreYEnNivelCero(void) {
    Envelope e;
    memset(&e, 0xFF, sizeof e);

    envInit(&e);

    CHECK(envIsIdle(&e));
    CHECK(e.level == 0);
}

#define MAX_LEVEL  32767   /* el máximo de Q15: el nivel al que llega el Attack */

/* Parámetros de prueba fijos de la story 9, salvo el que cada test varía. El Sustain se da en porcentaje, como en la
 * spec, y se pasa al nivel Q15 que lleva EnvParams al armar la entrada */
static EnvParams testParams(uint16_t attackMs, uint16_t decayMs, uint8_t sustainPercent, uint16_t releaseMs) {
    const EnvParams p = {attackMs, decayMs, (int16_t)(MAX_LEVEL * sustainPercent / 100), releaseMs};
    return p;
}

/* Toca una nota con una envolvente libre y avanza paso a paso (un paso = 1 ms). Devuelve el primer paso (el
 * primero es el 1) en que el nivel llega al máximo, o 0 si no llega en maxSteps. Se mide así porque después
 * del pico el Decay baja el nivel: mirar el nivel en un paso fijo no serviría. */
static uint32_t firstStepAtMax(const EnvParams *p, uint32_t maxSteps) {
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    for (uint32_t step = 1; step <= maxSteps; step++) {
        envStep(&e, p);
        if (e.level == MAX_LEVEL) {
            return step;
        }
    }
    return 0;
}

/* El Attack llega al máximo en attackMs, con error de a lo sumo 1 ms (criterio de la story 9), en los dos
 * extremos del rango de la spec 3.4: 2 y 2000 ms */
static void testElAttackLlegaAlMaximoEnSuTiempo(void) {
    const uint16_t attackTimes[] = {2, 2000};

    for (uint32_t i = 0; i < sizeof attackTimes / sizeof attackTimes[0]; i++) {
        const uint16_t attackMs = attackTimes[i];
        const EnvParams p = testParams(attackMs, 300, 50, 1000);

        const uint32_t step = firstStepAtMax(&p, attackMs + 10u);

        CHECK(step != 0);                       /* llegó al máximo */
        CHECK(step + 1u >= attackMs);           /* no antes de attackMs - 1 */
        CHECK(step <= attackMs + 1u);           /* ni después de attackMs + 1 */
    }
}

#define SUSTAIN_50_LEVEL  16383   /* 50 % de 32767 = 16383,5: se acepta 1 de diferencia por el redondeo */

/* Toca una nota con una envolvente libre y mide el Decay: cuántos pasos pasan desde el primer paso en que el
 * nivel llegó al máximo hasta el primer paso en que bajó al nivel de Sustain (con 1 de tolerancia) o por
 * debajo. Devuelve 0 si no llega en un tiempo razonable; en *levelAtEnd deja el nivel de ese paso. */
static uint32_t decaySteps(const EnvParams *p, int16_t sustainLevel, int16_t *levelAtEnd) {
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    uint32_t peakStep = 0;
    const uint32_t maxSteps = p->attackMs + p->decayMs + 10u;
    for (uint32_t step = 1; step <= maxSteps; step++) {
        envStep(&e, p);
        if (peakStep == 0) {
            if (e.level == MAX_LEVEL) {
                peakStep = step;
            }
        } else if (e.level <= sustainLevel + 1) {
            *levelAtEnd = e.level;
            return step - peakStep;
        }
    }
    return 0;
}

/* El Decay baja del máximo al nivel de Sustain en decayMs, con error de a lo sumo 1 ms, en los dos extremos
 * del rango de la spec 3.4: 2 y 2000 ms. No tiene que bajar de más: el nivel donde termina es el de Sustain. */
static void testElDecayBajaAlNivelDeSustainEnSuTiempo(void) {
    const uint16_t decayTimes[] = {2, 2000};

    for (uint32_t i = 0; i < sizeof decayTimes / sizeof decayTimes[0]; i++) {
        const uint16_t decayMs = decayTimes[i];
        const EnvParams p = testParams(300, decayMs, 50, 1000);
        int16_t levelAtEnd = 0;

        const uint32_t steps = decaySteps(&p, SUSTAIN_50_LEVEL, &levelAtEnd);

        CHECK(steps != 0);                           /* llegó al nivel de Sustain */
        CHECK(steps + 1u >= decayMs);                /* no antes de decayMs - 1 */
        CHECK(steps <= decayMs + 1u);                /* ni después de decayMs + 1 */
        CHECK(levelAtEnd >= SUSTAIN_50_LEVEL - 1);   /* y no bajó de más */
    }
}

/* Avanza la envolvente la cantidad de pasos (1 ms) indicada */
static void advance(Envelope *e, const EnvParams *p, uint32_t steps) {
    for (uint32_t i = 0; i < steps; i++) {
        envStep(e, p);
    }
}

/* Con Sustain en 100 % el Decay no tiene a dónde bajar: desde el pico el nivel queda en el máximo, y la voz
 * sigue ocupada (la tecla sigue apretada). Se mira cada paso desde attackMs + 1, el último en que el Attack
 * puede llegar al pico con el error de 1 ms que admite el test del Attack. */
static void testConSustainAl100PorCientoElDecayNoBaja(void) {
    const uint16_t attackMs = 300;
    const EnvParams p = testParams(attackMs, 300, 100, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);

    int16_t lowestAfterPeak = MAX_LEVEL;
    for (uint32_t step = 1; step <= attackMs + 300u + 100u; step++) {
        envStep(&e, &p);
        if (step >= attackMs + 1u && e.level < lowestAfterPeak) {
            lowestAfterPeak = e.level;
        }
    }

    CHECK(lowestAfterPeak == MAX_LEVEL);
    CHECK(!envIsIdle(&e));
}

/* Con Sustain en 0 % el Decay baja a 0 y la voz sigue ocupada mientras la tecla esté apretada: no queda libre
 * (eso solo pasa al terminar el Release). Se mira al terminar el Decay (con 1 ms de margen) y 100 ms después. */
static void testConSustainEnCeroElDecayBajaACeroYLaVozSigueOcupada(void) {
    const EnvParams p = testParams(300, 300, 0, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);

    advance(&e, &p, 300u + 300u + 1u);
    CHECK(e.level == 0);
    CHECK(!envIsIdle(&e));

    advance(&e, &p, 100u);
    CHECK(e.level == 0);
    CHECK(!envIsIdle(&e));
}

/* El Sustain mantiene el nivel mientras la tecla sigue apretada: ni baja, ni sube, ni la voz se libera. Se
 * mantiene 70 000 ms, más de lo que cuenta un contador de 16 bits de milisegundos (65 535): ningún contador de
 * la etapa puede dar la vuelta durante el Sustain y cambiar el nivel. El nivel de Sustain sale del
 * porcentaje, con 1 de tolerancia por el redondeo. */
static void testElSustainSeMantieneMientrasLaTeclaSigueApretada(void) {
    const EnvParams p = testParams(300, 300, 50, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);

    advance(&e, &p, 300u + 300u + 1u);          /* terminó el Decay (con 1 ms de margen) */
    const int16_t sustainLevel = e.level;
    CHECK(sustainLevel >= SUSTAIN_50_LEVEL - 1 && sustainLevel <= SUSTAIN_50_LEVEL + 1);

    int16_t lowest = sustainLevel;
    int16_t highest = sustainLevel;
    for (uint32_t step = 0; step < 70000u; step++) {
        envStep(&e, &p);
        if (e.level < lowest) {
            lowest = e.level;
        }
        if (e.level > highest) {
            highest = e.level;
        }
    }

    CHECK(lowest == sustainLevel);
    CHECK(highest == sustainLevel);
    CHECK(!envIsIdle(&e));
}

/* Soltar la tecla en Sustain: el Release baja el nivel a 0 en releaseMs, con error de a lo sumo 1 ms, en los
 * dos extremos del rango de la spec 3.4 (2 y 2000 ms). La voz queda libre recién cuando termina: mientras el
 * nivel sea mayor que 0 sigue ocupada, y al terminar el Release (con 1 ms de margen) está libre y en 0. */
static void testElReleaseBajaACeroEnSuTiempoYRecienAhiLaVozQuedaLibre(void) {
    const uint16_t releaseTimes[] = {2, 2000};

    for (uint32_t i = 0; i < sizeof releaseTimes / sizeof releaseTimes[0]; i++) {
        const uint16_t releaseMs = releaseTimes[i];
        const EnvParams p = testParams(300, 300, 50, releaseMs);
        Envelope e;
        envInit(&e);
        envNoteOn(&e);
        advance(&e, &p, 300u + 300u + 1u);       /* en Sustain */

        envNoteOff(&e);

        uint32_t zeroStep = 0;                   /* primer paso con el nivel en 0; 0 si no llegó */
        uint8_t freedWhileSounding = 0;          /* 1 = hubo un paso con nivel > 0 y la voz libre */
        for (uint32_t step = 1; step <= releaseMs + 1u; step++) {
            envStep(&e, &p);
            if (e.level > 0 && envIsIdle(&e)) {
                freedWhileSounding = 1;
            }
            if (e.level == 0 && zeroStep == 0) {
                zeroStep = step;
            }
        }

        CHECK(zeroStep != 0);                    /* llegó a 0 */
        CHECK(zeroStep + 1u >= releaseMs);       /* no antes de releaseMs - 1 */
        CHECK(zeroStep <= releaseMs + 1u);       /* ni después de releaseMs + 1 */
        CHECK(!freedWhileSounding);              /* libre solo con el nivel en 0 */
        CHECK(e.level == 0);
        CHECK(envIsIdle(&e));                    /* y al terminar, libre */
    }
}

/* Toca una nota con los valores fijos de la story 9 (A 300, D 300, S 50 %, R 1000), la suelta después de
 * stepsBeforeRelease pasos y mide el Release (spec 3.4: sale del nivel actual y dura releaseMs sea cual sea la
 * etapa en que se soltó). Comprueba que el nivel nunca sube, que no baja de golpe (en un paso cae a lo sumo lo
 * que cae una rampa lineal desde el nivel de partida en releaseMs pasos, más 1 por el redondeo), que llega a 0
 * en releaseMs con 1 ms de error y que la voz queda libre. Devuelve el nivel en que se soltó. */
static int16_t releaseFromStep(uint32_t stepsBeforeRelease) {
    const uint16_t releaseMs = 1000;
    const EnvParams p = testParams(300, 300, 50, releaseMs);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    advance(&e, &p, stepsBeforeRelease);
    const int16_t levelAtRelease = e.level;

    envNoteOff(&e);

    uint32_t zeroStep = 0;                       /* primer paso con el nivel en 0; 0 si no llegó */
    uint8_t rose = 0;                            /* 1 = el nivel subió en algún paso */
    uint8_t jumped = 0;                          /* 1 = el nivel cayó en un paso más que la rampa lineal */
    int16_t previous = levelAtRelease;
    const int16_t maxDropPerStep = (int16_t)(levelAtRelease / releaseMs + 1);
    for (uint32_t step = 1; step <= releaseMs + 1u; step++) {
        envStep(&e, &p);
        if (e.level > previous) {
            rose = 1;
        }
        if (previous - e.level > maxDropPerStep) {
            jumped = 1;
        }
        if (e.level == 0 && zeroStep == 0) {
            zeroStep = step;
        }
        previous = e.level;
    }

    CHECK(!rose);
    CHECK(!jumped);
    CHECK(zeroStep != 0);
    CHECK(zeroStep + 1u >= releaseMs);           /* tarda releaseMs aunque haya soltado a media rampa */
    CHECK(zeroStep <= releaseMs + 1u);
    CHECK(envIsIdle(&e));
    return levelAtRelease;
}

/* Soltar a mitad del Attack (150 de 300 ms): el Release sale del nivel que había subido hasta ahí, sin saltos, y
 * tarda releaseMs, no menos por haber salido de un nivel más bajo que el máximo */
static void testElReleaseALaMitadDelAttackSaleDelNivelActualYTardaLoMismo(void) {
    const int16_t level = releaseFromStep(150u);

    CHECK(level > 0 && level < MAX_LEVEL);       /* de verdad estaba a mitad de la subida */
}

/* Soltar a mitad del Decay (150 de 300 ms, ya pasado el pico): lo mismo, desde un nivel entre el máximo y el
 * de Sustain */
static void testElReleaseALaMitadDelDecaySaleDelNivelActualYTardaLoMismo(void) {
    const int16_t level = releaseFromStep(300u + 150u);

    CHECK(level > SUSTAIN_50_LEVEL + 1 && level < MAX_LEVEL);   /* de verdad estaba a mitad de la bajada */
}

/* Volver a tocar mientras la nota se apaga (spec 3.4): pasa a Attack desde el nivel actual, sin salto (el nivel
 * no cae ni sube de golpe en ningún paso: en uno sube a lo sumo lo que sube una rampa lineal desde ese nivel en
 * attackMs pasos, más 1 por el redondeo), la voz sigue ocupada, y el Attack tarda attackMs completos aunque salga
 * de un nivel bajo: llega al máximo a attackMs, con 1 ms de error. Se mira hasta el primer paso con el máximo:
 * después el Decay lo baja. */
static void testTocarDuranteElReleaseHaceAttackDesdeElNivelActual(void) {
    const uint16_t attackMs = 300;
    const EnvParams p = testParams(attackMs, 300, 50, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    advance(&e, &p, 300u + 300u + 1u);           /* en Sustain */
    envNoteOff(&e);
    advance(&e, &p, 500u);                       /* a mitad del Release */
    const int16_t levelAtRetrigger = e.level;
    CHECK(levelAtRetrigger > 0 && levelAtRetrigger < SUSTAIN_50_LEVEL);   /* de verdad se estaba apagando */

    envNoteOn(&e);

    uint32_t maxStep = 0;                        /* primer paso con el nivel en el máximo; 0 si no llegó */
    uint8_t fell = 0;                            /* 1 = el nivel bajó en un paso antes del pico */
    uint8_t jumped = 0;                          /* 1 = el nivel subió en un paso más que la rampa lineal */
    uint8_t freed = 0;                           /* 1 = la voz quedó libre en algún paso */
    int16_t previous = levelAtRetrigger;
    const int16_t maxRisePerStep = (int16_t)((MAX_LEVEL - levelAtRetrigger) / attackMs + 1);
    for (uint32_t step = 1; step <= attackMs + 1u; step++) {
        envStep(&e, &p);
        if (maxStep == 0) {
            if (e.level < previous) {
                fell = 1;
            }
            if (e.level - previous > maxRisePerStep) {
                jumped = 1;
            }
            if (e.level == MAX_LEVEL) {
                maxStep = step;
            }
        }
        if (envIsIdle(&e)) {
            freed = 1;
        }
        previous = e.level;
    }

    CHECK(!fell);
    CHECK(!jumped);
    CHECK(!freed);
    CHECK(maxStep != 0);
    CHECK(maxStep + 1u >= attackMs);             /* tarda attackMs aunque haya salido de un nivel más alto que 0 */
    CHECK(maxStep <= attackMs + 1u);
}

/* 1 = el nivel está entre 0 y 32767. El nivel es un int16_t: pasarse de 32767 da un valor negativo (la vuelta
 * del complemento a dos), así que con el cast a uint16_t una sola comparación cubre los dos extremos. */
static uint8_t levelInRange(int16_t level) {
    return (uint16_t)level <= MAX_LEVEL;
}

/* Avanza steps pasos y suma a *outOfRange los pasos en que el nivel quedó fuera de 0 ... 32767 */
static void advanceChecked(Envelope *e, const EnvParams *p, uint32_t steps, uint32_t *outOfRange) {
    for (uint32_t i = 0; i < steps; i++) {
        envStep(e, p);
        if (!levelInRange(e->level)) {
            (*outOfRange)++;
        }
    }
}

/* El nivel nunca sale de 0 ... 32767 (criterio 4 del Bloque 7). Se barren los extremos de la spec 3.4 para Attack,
 * Decay y Release (2 y 2000 ms) y tres Sustain (0, 50 y 100 %), y con cada combinación se suelta la nota en
 * cualquier momento del ciclo (Attack, Decay y Sustain, con un paso que recorre todas las etapas aunque sean de
 * 2 ms) y se prueban cuatro continuaciones: dejar que termine el Release, o volver a tocar 1 paso, a mitad y a
 * un paso del final del Release. Es un barrido fijo, sin números aleatorios: la falla se reproduce igual. */
static void testElNivelNuncaSaleDeCeroAMaximo(void) {
    const uint16_t times[] = {2, 2000};
    const uint8_t sustains[] = {0, 50, 100};
    uint32_t outOfRange = 0;

    for (uint32_t a = 0; a < 2; a++) {
        for (uint32_t d = 0; d < 2; d++) {
            for (uint32_t s = 0; s < 3; s++) {
                for (uint32_t r = 0; r < 2; r++) {
                    const uint16_t releaseMs = times[r];
                    const EnvParams p = testParams(times[a], times[d], sustains[s], releaseMs);
                    const uint32_t cycle = (uint32_t)times[a] + times[d] + 10u;   /* con un poco de Sustain */
                    const uint32_t stride = cycle / 64u + 1u;                      /* cada paso en los ciclos cortos */
                    /* 0 = no vuelve a tocar */
                    const uint32_t retriggerAfter[] = {0, 1, releaseMs / 2u, releaseMs - 1u};

                    for (uint32_t offAt = 0; offAt <= cycle; offAt += stride) {
                        for (uint32_t k = 0; k < sizeof retriggerAfter / sizeof retriggerAfter[0]; k++) {
                            Envelope e;
                            envInit(&e);
                            envNoteOn(&e);
                            advanceChecked(&e, &p, offAt, &outOfRange);
                            envNoteOff(&e);
                            if (retriggerAfter[k] == 0) {
                                advanceChecked(&e, &p, releaseMs + 10u, &outOfRange);
                            } else {
                                advanceChecked(&e, &p, retriggerAfter[k], &outOfRange);
                                envNoteOn(&e);
                                advanceChecked(&e, &p, cycle, &outOfRange);
                            }
                        }
                    }
                }
            }
        }
    }

    CHECK(outOfRange == 0);
}

/* El ciclo completo del criterio 1 de la story 9, con los valores fijos que tenía la placa hasta la story 10
 * (A 300 ms, D 300 ms, S 50 %, R 1000 ms): sube, baja hasta el Sustain, se queda ahí mientras la tecla sigue
 * apretada y, al soltarla, se apaga en el tiempo de Release. Los valores esperados son los del criterio, escritos
 * como números. Cada tiempo se acepta con 1 ms de error. */
static void testElCicloCompletoConLosValoresFijosDeLaStory(void) {
    const EnvParams p = testParams(300, 300, 50, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);

    /* Attack y Decay: el paso del pico y el primer paso en que el nivel llega al Sustain */
    uint32_t peakStep = 0;
    uint32_t sustainStep = 0;
    for (uint32_t step = 1; step <= 700u; step++) {
        envStep(&e, &p);
        if (peakStep == 0) {
            if (e.level == MAX_LEVEL) {
                peakStep = step;
            }
        } else if (sustainStep == 0 && e.level <= SUSTAIN_50_LEVEL + 1) {
            sustainStep = step;
        }
    }
    CHECK(peakStep != 0);
    CHECK(peakStep + 1u >= 300u && peakStep <= 300u + 1u);                       /* Attack: 300 ms */
    CHECK(sustainStep != 0);
    CHECK(sustainStep > peakStep);
    CHECK(sustainStep - peakStep + 1u >= 300u && sustainStep - peakStep <= 300u + 1u);   /* Decay: 300 ms */

    /* Sustain: el nivel de 50 % se queda ahí mientras la tecla sigue apretada */
    uint8_t leftSustain = 0;
    for (uint32_t step = 0; step < 2000u; step++) {
        envStep(&e, &p);
        if (e.level < SUSTAIN_50_LEVEL - 1 || e.level > SUSTAIN_50_LEVEL + 1) {
            leftSustain = 1;
        }
    }
    CHECK(!leftSustain);
    CHECK(!envIsIdle(&e));

    /* Release: al soltar, el nivel baja a 0 en 1000 ms y la voz queda libre */
    envNoteOff(&e);
    uint32_t zeroStep = 0;
    for (uint32_t step = 1; step <= 1000u + 1u; step++) {
        envStep(&e, &p);
        if (e.level == 0 && zeroStep == 0) {
            zeroStep = step;
        }
    }
    CHECK(zeroStep != 0);
    CHECK(zeroStep + 1u >= 1000u && zeroStep <= 1000u + 1u);                      /* Release: 1000 ms */
    CHECK(envIsIdle(&e));
}

/* Story 10: mapeo de los potenciómetros (spec 3.4). La lectura del ADC es de 12 bits (0 ... 4095), ya filtrada. */

/* Attack, Decay y Release van de 2 a 2000 ms. Las puntas tienen un margen de 50 cuentas: el giro real del pote no
 * siempre llega a 0 ni a 4095, y aun así tiene que alcanzar los extremos. Los valores esperados son los de la spec,
 * escritos como números. */
static void testElTiempoVaDe2A2000MsConMargenEnLasPuntas(void) {
    CHECK(envMapTimeMs(0) == 2);
    CHECK(envMapTimeMs(50) == 2);          /* dentro del margen de abajo */
    CHECK(envMapTimeMs(4045) == 2000);     /* dentro del margen de arriba */
    CHECK(envMapTimeMs(4095) == 2000);
}

/* Entre las puntas, el tiempo es lineal (spec 3.4): en el medio del recorrido útil (50 ... 4045) da el medio del
 * rango, (2 + 2000) / 2 = 1001 ms, con 1 ms de tolerancia por el redondeo. Además, al girar el pote el tiempo nunca
 * baja: se recorren las 4096 lecturas. */
static void testElTiempoEsLinealEntreLasPuntas(void) {
    const uint16_t middle = envMapTimeMs(2048);     /* (50 + 4045) / 2 = 2047,5 */
    CHECK(middle >= 1000 && middle <= 1002);

    uint8_t fell = 0;                               /* 1 = el tiempo bajó al subir la lectura */
    uint16_t previous = envMapTimeMs(0);
    for (uint32_t raw = 1; raw <= 4095u; raw++) {
        const uint16_t timeMs = envMapTimeMs((uint16_t)raw);
        if (timeMs < previous) {
            fell = 1;
        }
        previous = timeMs;
    }
    CHECK(!fell);
}

#define SUSTAIN_HALF_LEVEL  16383   /* 50 % de 32767 = 16383,5 */

/* Sustain va de 0 a 100 %, en Q15 (0 ... 32767), con el mismo margen de 50 cuentas en las puntas. Entre las puntas
 * es lineal: el 50 % cae entre las dos lecturas del medio del recorrido útil, (50 + 4045) / 2 = 2047,5. Se compara
 * así porque cada cuenta del ADC sube unas 8 unidades de Q15 y ninguna lectura cae justo en el medio. Al girar el
 * pote el nivel nunca baja: se recorren las 4096 lecturas. */
static void testElSustainVaDe0A100PorCientoConMargenEnLasPuntas(void) {
    CHECK(envMapSustainLevel(0) == 0);
    CHECK(envMapSustainLevel(50) == 0);              /* dentro del margen de abajo */
    CHECK(envMapSustainLevel(4045) == MAX_LEVEL);    /* dentro del margen de arriba */
    CHECK(envMapSustainLevel(4095) == MAX_LEVEL);

    CHECK(envMapSustainLevel(2047) <= SUSTAIN_HALF_LEVEL);
    CHECK(envMapSustainLevel(2048) >= SUSTAIN_HALF_LEVEL);

    uint8_t fell = 0;                                /* 1 = el nivel bajó al subir la lectura */
    int16_t previous = envMapSustainLevel(0);
    for (uint32_t raw = 1; raw <= 4095u; raw++) {
        const int16_t level = envMapSustainLevel((uint16_t)raw);
        if (level < previous) {
            fell = 1;
        }
        previous = level;
    }
    CHECK(!fell);
}

/* Story 10: los potenciómetros cambian los parámetros mientras la nota suena (spec 3.4). Lo ya recorrido de la
 * etapa se conserva y lo que le falta se hace a la velocidad nueva. */

/* Toca una nota con una envolvente libre y un Attack de attackBefore ms. Después de changeAt pasos, el pote pasa a
 * attackAfter ms. Sigue paso a paso hasta el pico y comprueba que, desde el cambio, el nivel nunca baja ni sube en un
 * paso más que la rampa nueva (de 0 al máximo en attackAfter pasos, más 1 por el redondeo). Devuelve el paso del pico,
 * contado desde que se tocó la nota, o 0 si no llegó. */
static uint32_t attackPeakWithChange(uint16_t attackBefore, uint32_t changeAt, uint16_t attackAfter) {
    const EnvParams before = testParams(attackBefore, 300, 50, 1000);
    const EnvParams after = testParams(attackAfter, 300, 50, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    advance(&e, &before, changeAt);

    const int16_t maxRisePerStep = (int16_t)(MAX_LEVEL / attackAfter + 1);
    uint8_t fell = 0;                            /* 1 = el nivel bajó en algún paso */
    uint8_t jumped = 0;                          /* 1 = el nivel subió en un paso más que la rampa nueva */
    uint32_t peakStep = 0;
    int16_t previous = e.level;
    for (uint32_t step = changeAt + 1u; step <= changeAt + attackAfter + 10u && peakStep == 0; step++) {
        envStep(&e, &after);
        if (e.level < previous) {
            fell = 1;
        }
        if (e.level - previous > maxRisePerStep) {
            jumped = 1;
        }
        if (e.level == MAX_LEVEL) {
            peakStep = step;
        }
        previous = e.level;
    }

    CHECK(!fell);
    CHECK(!jumped);
    return peakStep;
}

/* Girar el pote de Attack a mitad de la subida: la nota sigue desde el nivel en que estaba y llega al pico cuando
 * corresponde según lo que le faltaba. Cada tiempo se acepta con 1 ms de error. */
static void testCambiarElAttackAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta(void) {
    /* Más lento: de 1000 a 2000 ms al 40 %. Falta el 60 %, que a 2000 ms son 1200 ms: pico a los 400 + 1200 */
    const uint32_t slower = attackPeakWithChange(1000, 400, 2000);
    CHECK(slower != 0);
    CHECK(slower + 1u >= 1600u && slower <= 1600u + 1u);

    /* Más rápido y de golpe: de 2000 a 2 ms al 10 %. Falta el 90 %, que a 2 ms es 1,8 ms: pico a los 200 + 2 */
    const uint32_t faster = attackPeakWithChange(2000, 200, 2);
    CHECK(faster != 0);
    CHECK(faster + 1u >= 202u && faster <= 202u + 1u);
}

/* Toca una nota con una envolvente libre (Attack 300 ms, Sustain 50 %) y un Decay de decayBefore ms. changeAt pasos
 * después del pico, el pote pasa a decayAfter ms. Sigue paso a paso hasta que el nivel llega al de Sustain (con 1 de
 * tolerancia) y comprueba que, desde el cambio, el nivel nunca sube ni baja en un paso más que la rampa nueva (del
 * máximo al Sustain en decayAfter pasos, más 1 por el redondeo), y que no baja de más. Devuelve los pasos desde el
 * pico hasta el Sustain, o 0 si no llegó. */
static uint32_t decayEndWithChange(uint16_t decayBefore, uint32_t changeAt, uint16_t decayAfter) {
    const EnvParams before = testParams(300, decayBefore, 50, 1000);
    const EnvParams after = testParams(300, decayAfter, 50, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    for (uint32_t i = 0; i < 300u + 10u && e.level != MAX_LEVEL; i++) {
        envStep(&e, &before);
    }
    CHECK(e.level == MAX_LEVEL);                 /* en el pico */
    advance(&e, &before, changeAt);

    const int16_t maxDropPerStep = (int16_t)((MAX_LEVEL - SUSTAIN_50_LEVEL) / decayAfter + 1);
    uint8_t rose = 0;                            /* 1 = el nivel subió en algún paso */
    uint8_t jumped = 0;                          /* 1 = el nivel bajó en un paso más que la rampa nueva */
    uint32_t endStep = 0;
    int16_t previous = e.level;
    for (uint32_t step = changeAt + 1u; step <= changeAt + decayAfter + 10u && endStep == 0; step++) {
        envStep(&e, &after);
        if (e.level > previous) {
            rose = 1;
        }
        if (previous - e.level > maxDropPerStep) {
            jumped = 1;
        }
        if (e.level <= SUSTAIN_50_LEVEL + 1) {
            endStep = step;
        }
        previous = e.level;
    }

    CHECK(!rose);
    CHECK(!jumped);
    CHECK(e.level >= SUSTAIN_50_LEVEL - 1);      /* no bajó de más */
    return endStep;
}

/* Girar el pote de Decay a mitad de la bajada: la nota sigue desde el nivel en que estaba y llega al Sustain cuando
 * corresponde según lo que le faltaba. Cada tiempo se acepta con 1 ms de error. */
static void testCambiarElDecayAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta(void) {
    /* Más lento: de 1000 a 2000 ms al 40 %. Falta el 60 %, que a 2000 ms son 1200 ms: Sustain a los 400 + 1200 */
    const uint32_t slower = decayEndWithChange(1000, 400, 2000);
    CHECK(slower != 0);
    CHECK(slower + 1u >= 1600u && slower <= 1600u + 1u);

    /* Más rápido y de golpe: de 2000 a 2 ms al 10 %. Falta el 90 %, que a 2 ms es 1,8 ms: Sustain a los 200 + 2 */
    const uint32_t faster = decayEndWithChange(2000, 200, 2);
    CHECK(faster != 0);
    CHECK(faster + 1u >= 202u && faster <= 202u + 1u);
}

/* Toca una nota con una envolvente libre (A 300 ms, D 300 ms, S 50 %), la suelta en Sustain con un Release de
 * releaseBefore ms y, changeAt pasos después, el pote pasa a releaseAfter ms. Sigue paso a paso hasta que el nivel
 * llega a 0 y comprueba que, desde el cambio, el nivel nunca sube ni baja en un paso más que la rampa nueva (del nivel
 * en que se soltó a 0 en releaseAfter pasos, más 1 por el redondeo), y que la voz queda libre recién con el nivel en
 * 0. Devuelve los pasos desde que se soltó hasta el 0, o 0 si no llegó. */
static uint32_t releaseEndWithChange(uint16_t releaseBefore, uint32_t changeAt, uint16_t releaseAfter) {
    const EnvParams before = testParams(300, 300, 50, releaseBefore);
    const EnvParams after = testParams(300, 300, 50, releaseAfter);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    advance(&e, &before, 300u + 300u + 1u);      /* en Sustain */
    const int16_t levelAtRelease = e.level;
    envNoteOff(&e);
    advance(&e, &before, changeAt);

    const int16_t maxDropPerStep = (int16_t)(levelAtRelease / releaseAfter + 1);
    uint8_t rose = 0;                            /* 1 = el nivel subió en algún paso */
    uint8_t jumped = 0;                          /* 1 = el nivel bajó en un paso más que la rampa nueva */
    uint8_t freedWhileSounding = 0;              /* 1 = hubo un paso con nivel > 0 y la voz libre */
    uint32_t zeroStep = 0;
    int16_t previous = e.level;
    for (uint32_t step = changeAt + 1u; step <= changeAt + releaseAfter + 10u && zeroStep == 0; step++) {
        envStep(&e, &after);
        if (e.level > previous) {
            rose = 1;
        }
        if (previous - e.level > maxDropPerStep) {
            jumped = 1;
        }
        if (e.level > 0 && envIsIdle(&e)) {
            freedWhileSounding = 1;
        }
        if (e.level == 0) {
            zeroStep = step;
        }
        previous = e.level;
    }

    CHECK(!rose);
    CHECK(!jumped);
    CHECK(!freedWhileSounding);
    CHECK(envIsIdle(&e));                        /* y con el nivel en 0, libre */
    return zeroStep;
}

/* Girar el pote de Release mientras la nota se apaga: sigue desde el nivel en que estaba y llega a 0 cuando
 * corresponde según lo que le faltaba. Cada tiempo se acepta con 1 ms de error. */
static void testCambiarElReleaseAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta(void) {
    /* Más lento: de 1000 a 2000 ms al 40 %. Falta el 60 %, que a 2000 ms son 1200 ms: 0 a los 400 + 1200 */
    const uint32_t slower = releaseEndWithChange(1000, 400, 2000);
    CHECK(slower != 0);
    CHECK(slower + 1u >= 1600u && slower <= 1600u + 1u);

    /* Más rápido y de golpe: de 2000 a 2 ms al 10 %. Falta el 90 %, que a 2 ms es 1,8 ms: 0 a los 200 + 2 */
    const uint32_t faster = releaseEndWithChange(2000, 200, 2);
    CHECK(faster != 0);
    CHECK(faster + 1u >= 202u && faster <= 202u + 1u);
}

/* Girar el pote de Sustain con una nota sostenida: el nivel sigue al pote (spec 3.4), ya en el paso siguiente, y la
 * voz sigue ocupada. Se sube de 50 a 80 % y se baja a 20 %. Cada nivel se acepta con 1 de tolerancia por el
 * redondeo. */
static void testCambiarElSustainMueveElNivelDelSustain(void) {
    const EnvParams half = testParams(300, 300, 50, 1000);
    const EnvParams high = testParams(300, 300, 80, 1000);
    const EnvParams low = testParams(300, 300, 20, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    advance(&e, &half, 300u + 300u + 1u);         /* en Sustain */
    CHECK(e.level >= SUSTAIN_50_LEVEL - 1 && e.level <= SUSTAIN_50_LEVEL + 1);

    envStep(&e, &high);
    CHECK(e.level >= 26213 - 1 && e.level <= 26213 + 1);   /* 80 % de 32767 = 26213,6 */
    CHECK(!envIsIdle(&e));

    envStep(&e, &low);
    CHECK(e.level >= 6553 - 1 && e.level <= 6553 + 1);     /* 20 % de 32767 = 6553,4 */
    CHECK(!envIsIdle(&e));
}

/* Girar el pote de Sustain a mitad del Decay, de 50 a 20 %: el Decay va hacia el nivel nuevo (spec 3.4). El nivel no
 * sube en ningún paso, y el Decay termina en el 20 % (6553, con 1 de tolerancia) en su tiempo de siempre, 300 ms
 * desde el pico (con 1 ms de error): el pote de Sustain no cambia cuánto dura. Desde el paso siguiente al cambio, el
 * nivel tampoco baja en un paso más que la rampa nueva (del máximo al 20 % en 300 pasos, más 1 por el redondeo): así
 * no puede llegar al 20 % con un salto al final. El paso del cambio queda afuera: ahí el nivel baja de una vez el
 * cambio de S por la fracción recorrida, igual que en Sustain baja el cambio entero, y en placa lo suaviza el
 * filtro del pote. */
static void testCambiarElSustainDuranteElDecayTerminaEnElNivelNuevo(void) {
    const EnvParams half = testParams(300, 300, 50, 1000);
    const EnvParams low = testParams(300, 300, 20, 1000);
    Envelope e;
    envInit(&e);
    envNoteOn(&e);
    for (uint32_t i = 0; i < 300u + 10u && e.level != MAX_LEVEL; i++) {
        envStep(&e, &half);
    }
    CHECK(e.level == MAX_LEVEL);                 /* en el pico */
    advance(&e, &half, 150u);                    /* a mitad del Decay */
    CHECK(e.level > SUSTAIN_50_LEVEL + 1 && e.level < MAX_LEVEL);   /* de verdad estaba bajando */

    const int16_t maxDropPerStep = (int16_t)((MAX_LEVEL - 6553) / 300 + 1);
    uint8_t rose = 0;                            /* 1 = el nivel subió en algún paso */
    uint8_t jumped = 0;                          /* 1 = después del cambio, bajó en un paso más que la rampa nueva */
    uint32_t endStep = 0;                        /* pasos desde el pico hasta el 20 %; 0 si no llegó */
    int16_t previous = e.level;
    for (uint32_t step = 150u + 1u; step <= 300u + 10u && endStep == 0; step++) {
        envStep(&e, &low);
        if (e.level > previous) {
            rose = 1;
        }
        if (step > 150u + 1u && previous - e.level > maxDropPerStep) {
            jumped = 1;
        }
        if (e.level <= 6553 + 1) {
            endStep = step;
        }
        previous = e.level;
    }

    CHECK(!rose);
    CHECK(!jumped);
    CHECK(endStep != 0);
    CHECK(endStep + 1u >= 300u && endStep <= 300u + 1u);
    CHECK(e.level >= 6553 - 1);                  /* no bajó de más */
}

/* Parámetros en los extremos de la spec 3.4: cada bit de bits elige uno. Bit 0: Attack (2 o 2000 ms), bit 1: Decay
 * (2 o 2000 ms), bit 2: Sustain (0 o 100 %), bit 3: Release (2 o 2000 ms). 15 - bits los invierte a todos. */
static EnvParams extremeParams(uint32_t bits) {
    return testParams((bits & 1u) ? 2000 : 2, (bits & 2u) ? 2000 : 2, (bits & 4u) ? 100 : 0, (bits & 8u) ? 2000 : 2);
}

/* Avanza steps pasos y alterna los parámetros entre a y b cada period pasos, contados con *clock desde que se tocó la
 * nota. Suma a *outOfRange los pasos en que el nivel quedó fuera de 0 ... 32767. */
static void advanceAlternating(Envelope *e, const EnvParams *a, const EnvParams *b, uint32_t period, uint32_t *clock,
                               uint32_t steps, uint32_t *outOfRange) {
    for (uint32_t i = 0; i < steps; i++) {
        envStep(e, ((*clock / period) % 2u == 0) ? a : b);
        (*clock)++;
        if (!levelInRange(e->level)) {
            (*outOfRange)++;
        }
    }
}

/* El nivel nunca sale de 0 ... 32767 aunque los potes se muevan mientras la nota suena (criterio 4 del Bloque 7). Es
 * el barrido de la story 9 con los parámetros cambiando: para cada combinación de extremos, los cuatro parámetros
 * saltan a su extremo opuesto y vuelven, cada 1, 7 o 150 pasos (así cambian a mitad de cada etapa, también de las
 * de 2 ms). La nota se suelta en cualquier momento del ciclo (cada 64 pasos, hasta 4100: Attack y Decay de 2000 ms
 * y un poco de Sustain) y después se deja que termine el Release o se vuelve a tocar a los 500 pasos. Es un barrido
 * fijo, sin números aleatorios: la falla se reproduce igual. */
static void testConParametrosQueCambianElNivelNuncaSaleDeCeroAMaximo(void) {
    const uint32_t periods[] = {1, 7, 150};
    uint32_t outOfRange = 0;

    for (uint32_t bits = 0; bits < 16u; bits++) {
        const EnvParams a = extremeParams(bits);
        const EnvParams b = extremeParams(15u - bits);
        for (uint32_t k = 0; k < sizeof periods / sizeof periods[0]; k++) {
            for (uint32_t offAt = 0; offAt <= 4100u; offAt += 64u) {
                for (uint32_t retrigger = 0; retrigger < 2u; retrigger++) {
                    Envelope e;
                    uint32_t clock = 0;
                    envInit(&e);
                    envNoteOn(&e);
                    advanceAlternating(&e, &a, &b, periods[k], &clock, offAt, &outOfRange);
                    envNoteOff(&e);
                    if (retrigger) {
                        advanceAlternating(&e, &a, &b, periods[k], &clock, 500u, &outOfRange);
                        envNoteOn(&e);
                    }
                    advanceAlternating(&e, &a, &b, periods[k], &clock, 4100u, &outOfRange);
                }
            }
        }
    }

    CHECK(outOfRange == 0);
}

int main(void) {
    testUnaEnvolventeNuevaEstaLibreYEnNivelCero();
    testElAttackLlegaAlMaximoEnSuTiempo();
    testElDecayBajaAlNivelDeSustainEnSuTiempo();
    testConSustainAl100PorCientoElDecayNoBaja();
    testConSustainEnCeroElDecayBajaACeroYLaVozSigueOcupada();
    testElSustainSeMantieneMientrasLaTeclaSigueApretada();
    testElReleaseBajaACeroEnSuTiempoYRecienAhiLaVozQuedaLibre();
    testElReleaseALaMitadDelAttackSaleDelNivelActualYTardaLoMismo();
    testElReleaseALaMitadDelDecaySaleDelNivelActualYTardaLoMismo();
    testTocarDuranteElReleaseHaceAttackDesdeElNivelActual();
    testElNivelNuncaSaleDeCeroAMaximo();
    testElCicloCompletoConLosValoresFijosDeLaStory();
    testElTiempoVaDe2A2000MsConMargenEnLasPuntas();
    testElTiempoEsLinealEntreLasPuntas();
    testElSustainVaDe0A100PorCientoConMargenEnLasPuntas();
    testCambiarElAttackAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta();
    testCambiarElDecayAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta();
    testCambiarElReleaseAMitadDeLaRampaSigueDesdeElNivelYTerminaLoQueLeFalta();
    testCambiarElSustainMueveElNivelDelSustain();
    testCambiarElSustainDuranteElDecayTerminaEnElNivelNuevo();
    testConParametrosQueCambianElNivelNuncaSaleDeCeroAMaximo();

    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
