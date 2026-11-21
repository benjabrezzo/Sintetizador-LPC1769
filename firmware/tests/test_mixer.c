/* Tests del mezclador (spec 3.2, Bloques 5 y 7).
 * Cadena de niveles: cada producto muestra · nivel de la envolvente (AMP_*, Q15) se desplaza 15 bits antes
 * de sumar, y la suma se escala con la ganancia fija de 1/2. Una voz entra en la mezcla si su envolvente
 * no está libre. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "tables.h"
#include "oscillator.h"
#include "voice.h"
#include "mixer.h"
#include "dac_level.h"
#include "test_check.h"

#define ONE_INDEX_STEP  (1u << 16)    /* una posición de la tabla por muestra (Q16.16) */
#define AMP_HALF            0x4000    /* 0,5 en Q15 */
#define AMP_QUARTER         0x2000    /* 0,25 */
#define AMP_THREE_QUARTERS  0x6000    /* 0,75 */
#define AMP_EIGHTH          0x1000    /* 0,125 */

/* Tabla rampa: la muestra i vale i · 128. Todas son múltiplo de 4, así las divisiones de los tests
 * (amplitud 0,5 y ganancia 1/2) son exactas y no hay redondeo que discutir. */
static int16_t ramp[WAVE_SIZE];

static void buildRamp(void) {
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        ramp[i] = (int16_t)(i * 128);
    }
}

/* La mezcla recibe un arreglo de tablas indexado por el identificador de onda de cada voz (Voice.wave).
 * Estos tests usan una sola onda, la 0: sus voces tienen wave = 0 */
static const int16_t *const rampTables[1] = {ramp};
static const int16_t *const sineTables[1] = {sineTable};

/* Una voz que suena con el nivel dado (Q15): envolvente en Sustain. A la mezcla le importan el nivel y que la voz no
 * esté libre; la etapa concreta no cambia nada */
static void setSounding(Voice *v, int16_t level) {
    v->env.stage = ENV_SUSTAIN;
    v->env.level = level;
}

/* Una voz libre: envolvente en ENV_IDLE y nivel 0 */
static void setFree(Voice *v) {
    envInit(&v->env);
}

/* Una voz con amplitud 0,5 y ganancia 1/2 deja la cuarta parte de la muestra: i · 128 / 4 = i · 32 */
static void testUnaVozActivaDaMuestraPorAmplitudConGananciaDeUnMedio(void) {
    Voice voice;
    oscInit(&voice.osc, ONE_INDEX_STEP);
    setSounding(&voice, AMP_HALF);
    voice.wave = 0;

    for (uint32_t i = 0; i < 16; i++) {
        CHECK(mixNextSample(&voice, 1, rampTables) == (int32_t)(i * 32));
    }
}

/* La voz 0 está libre y, a propósito, con el nivel máximo (una voz libre real tiene nivel 0): lo que cuenta es la
 * etapa de la envolvente. Si la voz entrara en la mezcla, o si el mezclador dejara de mirar las voces al
 * encontrarla, la salida dejaría de ser la de la voz 1 sola (i · 32) */
static void testUnaVozInactivaNoAportaALaMezcla(void) {
    Voice voices[2];
    oscInit(&voices[0].osc, ONE_INDEX_STEP);
    setFree(&voices[0]);
    voices[0].env.level = INT16_MAX;
    voices[0].wave = 0;
    oscInit(&voices[1].osc, ONE_INDEX_STEP);
    setSounding(&voices[1], AMP_HALF);
    voices[1].wave = 0;

    for (uint32_t i = 0; i < 16; i++) {
        CHECK(mixNextSample(voices, 2, rampTables) == (int32_t)(i * 32));
    }
}

/* Cuatro voces con notas distintas (1, 2, 3 y 5 posiciones de la tabla por muestra) y amplitudes
 * distintas. Sobre la rampa i · 128, en la muestra n cada voz aporta:
 *   voz 0: 1n · 128 · 0,5   =  64n
 *   voz 1: 2n · 128 · 0,25  =  64n
 *   voz 2: 3n · 128 · 0,75  = 288n
 *   voz 3: 5n · 128 · 0,125 =  80n
 * La suma es 496n y, con la ganancia de 1/2, 248n. Son enteros exactos: sin redondeos. */
static void testCuatroVocesConNotasYAmplitudesDistintasDanLaSumaEsperada(void) {
    static const uint32_t indexesPerSample[4] = {1, 2, 3, 5};
    static const int16_t amplitudes[4] = {AMP_HALF, AMP_QUARTER, AMP_THREE_QUARTERS, AMP_EIGHTH};

    Voice voices[4];
    for (uint32_t v = 0; v < 4; v++) {
        oscInit(&voices[v].osc, indexesPerSample[v] * ONE_INDEX_STEP);
        setSounding(&voices[v], amplitudes[v]);
        voices[v].wave = 0;
    }

    for (uint32_t n = 0; n < 16; n++) {      /* el índice más alto es 5 · 15 = 75: sin vuelta de tabla */
        CHECK(mixNextSample(voices, 4, rampTables) == (int32_t)(248 * n));
    }
}

/* Dos voces con ondas distintas en el mismo bloque (spec 3.8: la onda se fija al arrancar cada nota). Las ondas
 * son la rampa i · 128 (onda 0) y una tabla plana de 4096 (onda 1). La voz 0 usa la plana con amplitud 0,5 y la
 * voz 1 usa la rampa con amplitud 0,25, así el identificador de onda no coincide con el número de voz ni las
 * dos voces dan lo mismo si se cruzan. En la muestra n:
 *   voz 0: 4096 · 0,5      = 2048
 *   voz 1: n · 128 · 0,25  = 32n
 * La suma es 2048 + 32n y, con la ganancia de 1/2, 1024 + 16n. Son enteros exactos: sin redondeos. */
#define FLAT_LEVEL  4096

static void testDosVocesConOndasDistintasLeenCadaUnaSuTablaEnElMismoBloque(void) {
    static int16_t flat[WAVE_SIZE];
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        flat[i] = FLAT_LEVEL;
    }
    const int16_t *const tables[2] = {ramp, flat};        /* onda 0 = rampa, onda 1 = plana */

    Voice voices[2];
    oscInit(&voices[0].osc, ONE_INDEX_STEP);
    setSounding(&voices[0], AMP_HALF);
    voices[0].wave = 1;
    oscInit(&voices[1].osc, ONE_INDEX_STEP);
    setSounding(&voices[1], AMP_QUARTER);
    voices[1].wave = 0;

    for (uint32_t n = 0; n < 16; n++) {
        CHECK(mixNextSample(voices, 2, tables) == (int32_t)(1024 + 16 * n));
    }
}

/* Lo que hace la ISR del DMA con cada bloque: AUDIO_BLOCK_SIZE muestras seguidas con las mismas voces */
static void renderBlock(Voice *voices, uint32_t numVoices) {
    for (uint32_t i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        (void)mixNextSample(voices, numVoices, rampTables);
    }
}

/* Fase de una voz que arrancó en 0 y estuvo activa durante `blocks` bloques: blocks · 64 · ΔP, con la
 * vuelta en 2^24 (spec 3.1). Es la fórmula cerrada, no la suma muestra por muestra del oscilador. */
static uint32_t expectedPhase(uint32_t step, uint32_t blocks) {
    return (uint32_t)(((uint64_t)blocks * AUDIO_BLOCK_SIZE * step) % (1ull << 24));
}

/* Incrementos con parte fraccionaria y distintos entre sí (≈ 3,2 a 6,5 posiciones por muestra): en 3 bloques
 * cada fase da varias vueltas, y una muestra de más o de menos cambia el resultado */
static const uint32_t distinctSteps[4] = {0x00031234, 0x0004A5A5, 0x00057777, 0x00068001};

static void initFourVoices(Voice *voices) {
    for (uint32_t v = 0; v < 4; v++) {
        oscInit(&voices[v].osc, distinctSteps[v]);
        setSounding(&voices[v], AMP_HALF);
        voices[v].wave = 0;
    }
}

/* Parte A: con las cuatro voces activas, tres bloques seguidos dejan cada fase donde la fórmula dice.
 * Parte B: la voz 1 se apaga durante el bloque 1 (como al soltar una tecla) y las demás voces, que no
 * dejaron de sonar, siguen con la fase de tres bloques completos. */
static void testLaFaseDeCadaVozContinuaEntreBloquesSinDependerDeLasOtras(void) {
    Voice voices[4];

    initFourVoices(voices);
    for (uint32_t b = 0; b < 3; b++) {
        renderBlock(voices, 4);
    }
    for (uint32_t v = 0; v < 4; v++) {
        CHECK(voices[v].osc.phase == expectedPhase(distinctSteps[v], 3));
    }

    initFourVoices(voices);
    renderBlock(voices, 4);
    setFree(&voices[1]);
    renderBlock(voices, 4);
    setSounding(&voices[1], AMP_HALF);
    renderBlock(voices, 4);
    CHECK(voices[0].osc.phase == expectedPhase(distinctSteps[0], 3));
    CHECK(voices[2].osc.phase == expectedPhase(distinctSteps[2], 3));
    CHECK(voices[3].osc.phase == expectedPhase(distinctSteps[3], 3));
}

#define DAC_MAX            1023     /* rango del DAC de 10 bits: 0...1023 (spec 3.2) */
#define RENDERED_SAMPLES   256      /* más de dos períodos de Do4 (≈ 122 muestras cada uno) */

/* Un recorte limpio cambia a lo sumo ≈ 76 valores del DAC entre dos muestras seguidas (medido: la mezcla
 * al doble de la escala, y la tabla sin interpolar a veces avanza 3 posiciones). Un wrap-around saltaría
 * ≈ 1000 de un extremo al otro. El umbral es un cuarto del rango: lejos de ambos. */
#define MAX_JUMP           256

/* Cuatro voces con la misma nota, la amplitud máxima y la misma fase: la mezcla pasa del doble de la escala
 * del DAC. Los picos tienen que quedar clavados en 0 y 1023, siempre dentro del rango y sin saltos. */
static void testCuatroVocesAlMaximoEnFaseSaturanEn0Y1023SinWrapAround(void) {
    Voice voices[4];
    for (uint32_t v = 0; v < 4; v++) {
        oscInit(&voices[v].osc, noteSteps[0]);      /* Do4 */
        setSounding(&voices[v], INT16_MAX);         /* 1,0 en Q15 */
        voices[v].wave = 0;
    }

    uint32_t min = DAC_MAX;
    uint32_t max = 0;
    uint32_t previous = 0;
    for (uint32_t i = 0; i < RENDERED_SAMPLES; i++) {
        const uint32_t value = dacValueFromSample(mixNextSample(voices, 4, sineTables));
        CHECK(value <= DAC_MAX);
        if (value < min) {
            min = value;
        }
        if (value > max) {
            max = value;
        }
        if (i > 0) {
            CHECK(abs((int)value - (int)previous) < MAX_JUMP);
        }
        previous = value;
    }
    CHECK(min == 0);
    CHECK(max == DAC_MAX);
}

/* Tabla de recortes: repite {+máx, 0, −máx, 0}. Con las cuatro voces al máximo, en fase y a una posición
 * de la tabla por muestra, la mezcla lee esa secuencia en orden:
 *   +máx → pasa del doble de la escala del DAC: se recorta arriba (1023)
 *   0    → queda en el centro (512), sin recortar
 *   −máx → pasa del doble de la escala: se recorta abajo (0)
 *   0    → centro (512)
 * Hay 2 recortes cada 4 muestras. La tabla tiene 256 muestras (64 repeticiones): 128 recortes. */
static int16_t clipTable[WAVE_SIZE];
static const int16_t *const clipTables[1] = {clipTable};

static void buildClipTable(void) {
    for (uint32_t i = 0; i < WAVE_SIZE; i += 4) {
        clipTable[i]     = INT16_MAX;
        clipTable[i + 1] = 0;
        clipTable[i + 2] = -INT16_MAX;
        clipTable[i + 3] = 0;
    }
}

static void testElContadorDeSaturacionesCuentaExactamenteLasMuestrasRecortadas(void) {
    static const uint32_t expectedValues[4] = {1023, 512, 0, 512};

    Voice voices[4];
    for (uint32_t v = 0; v < 4; v++) {
        oscInit(&voices[v].osc, ONE_INDEX_STEP);
        setSounding(&voices[v], INT16_MAX);
        voices[v].wave = 0;
    }

    uint32_t saturations = 1000;             /* el contador acumula: no se pisa ni se reinicia */
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        const uint32_t value = dacValueFromSampleCounted(mixNextSample(voices, 4, clipTables), &saturations);
        CHECK(value == expectedValues[i % 4]);
    }
    CHECK(saturations == 1000 + 128);
}

/* Dos voces al máximo y en fase caben justo en el DAC: con la ganancia de 1/2 la mezcla llega a ±1 escala
 * completa, ni un valor más. Un período entero de la senoide a una posición por muestra pasa por todas las
 * entradas de la tabla, el pico incluido: el contador queda en 0 y los picos, clavados en 1023 y 0.
 * Por eso recorta recién con 3 y 4 voces. */
static void testDosVocesAlMaximoEnFaseNoRecortan(void) {
    Voice voices[2];
    for (uint32_t v = 0; v < 2; v++) {
        oscInit(&voices[v].osc, ONE_INDEX_STEP);
        setSounding(&voices[v], INT16_MAX);
        voices[v].wave = 0;
    }

    uint32_t saturations = 0;
    uint32_t min = DAC_MAX;
    uint32_t max = 0;
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        const uint32_t value = dacValueFromSampleCounted(mixNextSample(voices, 2, sineTables), &saturations);
        if (value < min) {
            min = value;
        }
        if (value > max) {
            max = value;
        }
    }
    CHECK(saturations == 0);
    CHECK(min == 0);
    CHECK(max == DAC_MAX);
}

#define DAC_PCLK_HZ  25000000u      /* PCLK del DAC: CCLK / 4 (DAC_Init); Fs = PCLK / DAC_COUNTER_VALUE */

/* El acorde de la secuencia de prueba que tuvo el firmware en el Bloque 5 (ya retirada en el Bloque 6): se arma de a
 * una nota por paso y cada paso dura 5 s. Es dato de este test, no configuración del sintetizador */
#define CHORD_STEP_MS  5000
#define CHORD_SIZE     4
#define CHORD_NOTES    {0, 2, 4, 7}     /* Do4, Mi4, Sol4, Do5: índices de noteSteps (spec 3.11) */

/* La secuencia de acordes del Bloque 5 (ya retirada del firmware) con las voces al máximo: un paso por
 * voz, una más en cada uno. Cada voz nueva entra en fase 0 y las que ya sonaban siguen, como hará SysTick.
 * Con 1 y 2 voces ninguna muestra pasa de la escala del DAC, sea cual sea la fase: 0 saturaciones. Con 3
 * y 4 voces la suma pasa del doble de la escala y recorta: el contador tiene que ser mayor que 0
 * (criterio 3 de la story 6). No se fijan los conteos exactos, que no salen de ningún criterio. */
static void testElAcordeDeLaSecuenciaRecortaSoloConTresYCuatroVoces(void) {
    static const uint8_t chord[CHORD_SIZE] = CHORD_NOTES;
    const uint32_t samplesPerStep =
        (uint32_t)((uint64_t)CHORD_STEP_MS *(DAC_PCLK_HZ / DAC_COUNTER_VALUE) / 1000);

    Voice voices[CHORD_SIZE];
    for (uint32_t v = 0; v < CHORD_SIZE; v++) {
        oscInit(&voices[v].osc, 0);
        setFree(&voices[v]);
        voices[v].wave = 0;
    }

    for (uint32_t k = 0; k < CHORD_SIZE; k++) {
        oscInit(&voices[k].osc, noteSteps[chord[k]]);     /* entra en fase 0 */
        setSounding(&voices[k], INT16_MAX);

        uint32_t saturations = 0;
        for (uint32_t n = 0; n < samplesPerStep; n++) {
            (void)dacValueFromSampleCounted(mixNextSample(voices, CHORD_SIZE, sineTables),
                                            &saturations);
        }

        const uint32_t soundingVoices = k + 1;
        if (soundingVoices <= 2) {
            CHECK(saturations == 0);
        } else {
            CHECK(saturations > 0);
        }
    }
}

/* Una voz apretada con Sustain en 0 %: el Decay la dejó en nivel 0 (lo prueba test_envelope), pero la tecla sigue
 * apretada y la voz no está libre. Aporta 0 a la mezcla y, aun así, su fase sigue avanzando: así la nota no
 * arranca de cero cuando cambie el Sustain o al soltarla. Una voz libre, en cambio, ni suma ni avanza. La voz 0 suena
 * sola con nivel 0,5 (i · 32); la 1 es la apretada en silencio y la 2 está libre, ambas con otros incrementos. Tras 16
 * muestras la fase de la 1 vale 16 · 2 posiciones (Q16.16) y la de la 2 sigue en 0. */
static void testUnaVozApretadaEnSilencioSuma0PeroSuFaseAvanza(void) {
    Voice voices[3];
    oscInit(&voices[0].osc, ONE_INDEX_STEP);
    setSounding(&voices[0], AMP_HALF);
    voices[0].wave = 0;
    oscInit(&voices[1].osc, 2 * ONE_INDEX_STEP);
    setSounding(&voices[1], 0);                      /* nivel 0, pero apretada: no está libre */
    voices[1].wave = 0;
    oscInit(&voices[2].osc, 3 * ONE_INDEX_STEP);
    setFree(&voices[2]);
    voices[2].wave = 0;

    for (uint32_t i = 0; i < 16; i++) {
        CHECK(mixNextSample(voices, 3, rampTables) == (int32_t)(i * 32));    /* solo suena la voz 0 */
    }
    CHECK(voices[1].osc.phase == 16u * 2u * ONE_INDEX_STEP);                 /* la apretada en silencio avanzó */
    CHECK(voices[2].osc.phase == 0);                                         /* la libre no */
}

int main(void) {
    buildRamp();
    buildClipTable();
    testUnaVozActivaDaMuestraPorAmplitudConGananciaDeUnMedio();
    testUnaVozInactivaNoAportaALaMezcla();
    testCuatroVocesConNotasYAmplitudesDistintasDanLaSumaEsperada();
    testDosVocesConOndasDistintasLeenCadaUnaSuTablaEnElMismoBloque();
    testLaFaseDeCadaVozContinuaEntreBloquesSinDependerDeLasOtras();
    testCuatroVocesAlMaximoEnFaseSaturanEn0Y1023SinWrapAround();
    testElContadorDeSaturacionesCuentaExactamenteLasMuestrasRecortadas();
    testDosVocesAlMaximoEnFaseNoRecortan();
    testElAcordeDeLaSecuenciaRecortaSoloConTresYCuatroVoces();
    testUnaVozApretadaEnSilencioSuma0PeroSuFaseAvanza();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
