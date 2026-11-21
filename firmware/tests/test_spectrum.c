/* Tests del análisis espectral (spec 3.6, story 15). Los valores esperados salen de los criterios de la story y de
 * la teoría de la DFT con ventana Hann, no del código de spectrum. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "spectrum.h"
#include "test_check.h"

#define TWO_PI  6.283185307179586
#define FS      32010.24         /* Hz: 25 MHz / 781 (spec 3.1) */

/* Senoide de amplitud dada (Q15) cuya frecuencia cae exactamente en el bin k: k períodos en FFT_SIZE muestras */
static void fillSineAtBin(int16_t samples[FFT_SIZE], uint32_t bin, double amplitude) {
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = (int16_t)lround(amplitude * sin(TWO_PI * bin * n / FFT_SIZE));
    }
}

/* Bin de mayor magnitud (el primero, si hay empate) */
static uint32_t peakBin(const uint16_t magnitudes[SPECTRUM_BINS]) {
    uint32_t peak = 0;
    for (uint32_t k = 1; k < SPECTRUM_BINS; k++) {
        if (magnitudes[k] > magnitudes[peak]) {
            peak = k;
        }
    }
    return peak;
}

/* Criterio 1: una senoide centrada en un bin deja su pico en ese bin, en toda la banda de análisis (del bin 4,
 * en la banda 3, al 255, el último de la banda 8) */
static void testPicoEnBinExacto(void) {
    static const uint32_t bins[] = {4, 8, 16, 100, 255};
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    for (uint32_t i = 0; i < sizeof(bins) / sizeof(bins[0]); i++) {
        fillSineAtBin(samples, bins[i], 16000.0);
        spectrumMagnitudes(samples, magnitudes);
        if (peakBin(magnitudes) != bins[i]) {
            printf("senoide en el bin %u: pico en el bin %u\n", (unsigned)bins[i], (unsigned)peakBin(magnitudes));
        }
        CHECK(peakBin(magnitudes) == bins[i]);
        CHECK(magnitudes[bins[i]] > 0);
    }
}

/* Senoide de amplitud dada (Q15) y frecuencia en Hz, con el Fs real del DAC: cae entre bins */
static void fillSineAtHz(int16_t samples[FFT_SIZE], double hz, double amplitude) {
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = (int16_t)lround(amplitude * sin(TWO_PI * hz * n / FS));
    }
}

/* Criterio 1: una frecuencia entre bins deja el pico en el bin más cercano. Las ocho notas de la spec 3.11, con el
 * bin esperado calculado a mano como f / (Fs / 512), con Fs / 512 = 62,52 Hz (posición en el comentario) */
static void testPicoEntreBins(void) {
    static const struct { double hz; uint32_t bin; } notes[] = {
        {261.63, 4},    /* Do4: 4,19 */
        {293.66, 5},    /* Re4: 4,70 */
        {329.63, 5},    /* Mi4: 5,27 */
        {349.23, 6},    /* Fa4: 5,59 */
        {392.00, 6},    /* Sol4: 6,27 */
        {440.00, 7},    /* La4: 7,04 */
        {493.88, 8},    /* Si4: 7,90 */
        {523.25, 8},    /* Do5: 8,37 */
    };
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    for (uint32_t i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
        fillSineAtHz(samples, notes[i].hz, 16000.0);
        spectrumMagnitudes(samples, magnitudes);
        if (peakBin(magnitudes) != notes[i].bin) {
            printf("%.2f Hz: pico en el bin %u, esperado %u\n", notes[i].hz, (unsigned)peakBin(magnitudes),
                   (unsigned)notes[i].bin);
        }
        CHECK(peakBin(magnitudes) == notes[i].bin);
    }
}

/* Criterio 1 ("las vecinas pueden subir por la ventana, con menos nivel"): la fuga de Hann está acotada. Su primer
 * lóbulo lateral queda unos 31,5 dB por debajo del principal y después cae 18 dB por octava; el bin pico es el más
 * cercano a la frecuencia (a ≤ 0,5 bin), así que un bin a más de 3 del pico está a ≥ 2,5 bins de la frecuencia y
 * tiene que quedar al menos 30 dB abajo (magnitud ≤ pico / 32). Tonos desde el bin 4: más abajo, la imagen
 * negativa de la senoide se acerca a la continua. */
#define LEAKAGE_GUARD_BINS   3
#define LEAKAGE_RATIO        32      /* −30 dB */

static void testFugaDeHannAcotada(void) {
    static const double tonesHz[] = {261.63, 493.88, 1000.0, 1234.5, 5000.0, 8000.0};
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    for (uint32_t i = 0; i < sizeof(tonesHz) / sizeof(tonesHz[0]); i++) {
        fillSineAtHz(samples, tonesHz[i], 16000.0);
        spectrumMagnitudes(samples, magnitudes);
        const uint32_t peak = peakBin(magnitudes);
        for (uint32_t k = 0; k < SPECTRUM_BINS; k++) {
            const uint32_t distance = (k > peak) ? k - peak : peak - k;
            if (distance > LEAKAGE_GUARD_BINS && (uint32_t)magnitudes[k] * LEAKAGE_RATIO > magnitudes[peak]) {
                printf("%.2f Hz: bin %u con magnitud %u, pico %u en el bin %u\n", tonesHz[i], (unsigned)k,
                       (unsigned)magnitudes[k], (unsigned)magnitudes[peak], (unsigned)peak);
            }
            if (distance > LEAKAGE_GUARD_BINS) {
                CHECK((uint32_t)magnitudes[k] * LEAKAGE_RATIO <= magnitudes[peak]);
            }
        }
    }
}

/* Criterio 1 ("con distintas amplitudes") y spec 3.6: la magnitud de un bin se normaliza para que una senoide
 * centrada en él dé su amplitud, así que escala con ella. Tolerancia de 2 % más 16 LSB (el truncado de las
 * nueve etapas de la FFT en Q15 y el de la raíz entera). Amplitudes de a una octava: de casi el fondo de
 * escala a 250 (unos 8 LSB del DAC de 10 bits) */
#define AMPLITUDE_TOLERANCE_LSB   16
#define AMPLITUDE_TOLERANCE_DIV   50     /* 2 % */

static int magnitudeNear(double expected, uint16_t measured) {
    return fabs(measured - expected) <= expected / AMPLITUDE_TOLERANCE_DIV + AMPLITUDE_TOLERANCE_LSB;
}

static void testMagnitudIgualALaAmplitudEnBinCentrado(void) {
    static const double amplitudes[] = {32000.0, 16000.0, 8000.0, 4000.0, 2000.0, 1000.0, 500.0, 250.0};
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    for (uint32_t i = 0; i < sizeof(amplitudes) / sizeof(amplitudes[0]); i++) {
        fillSineAtBin(samples, 40, amplitudes[i]);
        spectrumMagnitudes(samples, magnitudes);
        if (!magnitudeNear(amplitudes[i], magnitudes[40])) {
            printf("amplitud %.0f: magnitud %u en el bin 40\n", amplitudes[i], (unsigned)magnitudes[40]);
        }
        CHECK(magnitudeNear(amplitudes[i], magnitudes[40]));
    }
}

/* Escala máxima: el audio capturado llega a ±32767 (el DAC de 10 bits saturado, llevado a Q15). El oráculo es una
 * DFT directa en double con Hann = ½(1 − cos(2πn/N)) y la normalización de la spec 3.6 (una senoide centrada de
 * amplitud A da A, o sea |X| · 4 / N; la continua D da 2·D). Se compara cada bin, con la tolerancia de la
 * amplitud. La magnitud no cabe en uint16_t con la continua al mínimo (2 · 32768): ahí tiene que quedar saturada en
 * 65535, no dar la vuelta a 0 */
#define MAGNITUDE_MAX   65535.0

static void referenceMagnitudes(const int16_t samples[FFT_SIZE], double reference[SPECTRUM_BINS]) {
    for (uint32_t k = 0; k < SPECTRUM_BINS; k++) {
        double re = 0.0;
        double im = 0.0;
        for (uint32_t n = 0; n < FFT_SIZE; n++) {
            const double w = 0.5 * (1.0 - cos(TWO_PI * n / FFT_SIZE));
            re += samples[n] * w * cos(TWO_PI * k * n / FFT_SIZE);
            im -= samples[n] * w * sin(TWO_PI * k * n / FFT_SIZE);
        }
        reference[k] = fmin(sqrt(re * re + im * im) * 4.0 / FFT_SIZE, MAGNITUDE_MAX);
    }
}

/* Cuenta los bins que se apartan del oráculo más que la tolerancia, y muestra el primero */
static uint32_t binsOutOfTolerance(const char *name, const int16_t samples[FFT_SIZE]) {
    static double reference[SPECTRUM_BINS];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint32_t bad = 0;

    referenceMagnitudes(samples, reference);
    spectrumMagnitudes(samples, magnitudes);
    for (uint32_t k = 0; k < SPECTRUM_BINS; k++) {
        if (!magnitudeNear(reference[k], magnitudes[k])) {
            if (bad == 0) {
                printf("%s: bin %u con magnitud %u, esperado %.1f\n", name, (unsigned)k, (unsigned)magnitudes[k],
                       reference[k]);
            }
            bad++;
        }
    }
    return bad;
}

static void testSinDesbordeAEscalaMaxima(void) {
    static int16_t samples[FFT_SIZE];

    /* Senoide a fondo de escala, centrada en un bin */
    fillSineAtBin(samples, 40, 32767.0);
    CHECK(binsOutOfTolerance("senoide a fondo de escala", samples) == 0);

    /* Cuatro notas (Do4, Mi4, Sol4 y Do5) a fondo de escala sumadas y recortadas a ±32767, como las deja el DAC */
    static const double notesHz[] = {261.63, 329.63, 392.00, 523.25};
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        double sum = 0.0;
        for (uint32_t v = 0; v < 4; v++) {
            sum += 32767.0 * sin(TWO_PI * notesHz[v] * n / FS);
        }
        samples[n] = (int16_t)lround(fmax(-32767.0, fmin(32767.0, sum)));
    }
    CHECK(binsOutOfTolerance("cuatro voces recortadas", samples) == 0);

    /* Continua al mínimo de int16_t: el peor caso para el desborde de la magnitud */
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = -32768;
    }
    CHECK(binsOutOfTolerance("continua al mínimo", samples) == 0);

    /* Alternancia de fondo a fondo (Nyquist): la energía cae junto al último bin */
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = (n % 2 == 0) ? 32767 : -32767;
    }
    CHECK(binsOutOfTolerance("alternancia a fondo de escala", samples) == 0);
}

/* Spec 3.6: el bin 0 (continua) no entra en ninguna banda. La ventana Hann, en cambio, reparte la continua en los
 * bins 0 y ±1 (W[0] = N/2, W[±1] = −N/4): una continua D da 2·D en el bin 0 y D en el bin 1. Entonces la banda 1
 * (el bin 1) vale D, y no 2·D como valdría si entrara el bin 0; las bandas 2 a 8, que no reciben nada, valen 0 */
static void testContinuaNoEntraEnNingunaBanda(void) {
    static const double offsets[] = {16000.0, -16000.0};
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];

    for (uint32_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        for (uint32_t n = 0; n < FFT_SIZE; n++) {
            samples[n] = (int16_t)offsets[i];
        }
        spectrumMagnitudes(samples, magnitudes);
        spectrumBands(magnitudes, bands);
        if (!magnitudeNear(fabs(offsets[i]), bands[0])) {
            printf("continua %.0f: banda 1 vale %u, esperado %.0f (la mitad de la del bin 0, %u)\n", offsets[i],
                   (unsigned)bands[0], fabs(offsets[i]), (unsigned)magnitudes[0]);
        }
        CHECK(magnitudeNear(fabs(offsets[i]), bands[0]));
        for (uint32_t b = 1; b < SPECTRUM_NUM_BANDS; b++) {
            CHECK(bands[b] <= AMPLITUDE_TOLERANCE_LSB);
        }
    }
}

/* Spec 3.6 (tabla de bandas) y criterio 1: un tono centrado en el primero o el último bin de cada banda deja
 * esa banda como la más alta. La ventana Hann pasa la mitad de la magnitud (−6 dB) al bin vecino, que puede ser de la
 * banda de al lado; el resto de las bandas queda en 0. Entonces la banda del tono vale la amplitud y las demás, a lo
 * sumo la mitad (con la tolerancia de siempre). Bandas numeradas de 1 a 8, como en la spec. */
static void testAgrupacionDeBins(void) {
    static const struct { uint32_t bin; uint32_t band; } tones[] = {
        {1, 1},
        {2, 2},   {3, 2},
        {4, 3},   {7, 3},
        {8, 4},   {15, 4},
        {16, 5},  {31, 5},
        {32, 6},  {63, 6},
        {64, 7},  {127, 7},
        {128, 8}, {255, 8},
    };
    static const double amplitude = 16000.0;
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];

    for (uint32_t i = 0; i < sizeof(tones) / sizeof(tones[0]); i++) {
        fillSineAtBin(samples, tones[i].bin, amplitude);
        spectrumMagnitudes(samples, magnitudes);
        spectrumBands(magnitudes, bands);
        for (uint32_t b = 1; b <= SPECTRUM_NUM_BANDS; b++) {
            const double expected = (b == tones[i].band) ? amplitude : amplitude / 2;
            const int ok = (b == tones[i].band) ? magnitudeNear(expected, bands[b - 1])
                                                : bands[b - 1] <= expected + expected / AMPLITUDE_TOLERANCE_DIV +
                                                                  AMPLITUDE_TOLERANCE_LSB;
            if (!ok) {
                printf("tono en el bin %u (banda %u): la banda %u vale %u\n", (unsigned)tones[i].bin,
                       (unsigned)tones[i].band, (unsigned)b, (unsigned)bands[b - 1]);
            }
            CHECK(ok);
        }
    }
}

/* Notas cerca de un borde de banda (criterio 1: "las vecinas pueden subir por la ventana, con menos nivel").
 * Los valores esperados son la respuesta teórica de la Hann, |W(δ)| = |sinc(δ) / (1 − δ²)| con δ la distancia del bin
 * a la frecuencia real (en bins de 62,52 Hz), y salen de la cuenta a mano de cada comentario, no del código. Cada
 * banda vale el máximo de sus bins: la de menor δ. Diferencia en dB con ±1 dB de tolerancia. */
#define NOTE_AMPLITUDE      16000.0
#define DB_TOLERANCE        1.0

static double bandDb(const uint16_t bands[SPECTRUM_NUM_BANDS], uint32_t upper, uint32_t lower) {
    return 20.0 * log10((double)bands[upper - 1] / (double)bands[lower - 1]);      /* bandas numeradas de 1 a 8 */
}

static void bandsOfNote(double hz, uint16_t bands[SPECTRUM_NUM_BANDS]) {
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    fillSineAtHz(samples, hz, NOTE_AMPLITUDE);
    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
}

/* Do4 (261,63 Hz) está en la posición 4,185: banda 3 por el bin 4 (δ = 0,185, |W| = 0,980) y banda 2 por el bin 3
 * (δ = 1,185, |W| = 0,365). La banda 3 queda 20·log10(0,980 / 0,365) = 8,6 dB sobre la 2 */
static void testDo4Banda3SobreBanda2(void) {
    uint16_t bands[SPECTRUM_NUM_BANDS];
    bandsOfNote(261.63, bands);
    const double db = bandDb(bands, 3, 2);
    if (fabs(db - 8.6) > DB_TOLERANCE) {
        printf("Do4: banda 3 sobre banda 2 por %.1f dB, esperado 8,6\n", db);
    }
    CHECK(fabs(db - 8.6) <= DB_TOLERANCE);
    for (uint32_t b = 1; b <= SPECTRUM_NUM_BANDS; b++) {
        CHECK(b == 3 || bands[b - 1] < bands[2]);
    }
}

/* Si4 (493,88 Hz) está en la posición 7,899: banda 4 por el bin 8 (δ = 0,101, |W| = 0,993) y banda 3 por el bin 7
 * (δ = 0,899, |W| = 0,572). La banda 4 queda 20·log10(0,993 / 0,572) = 4,8 dB sobre la 3 */
static void testSi4Banda4SobreBanda3(void) {
    uint16_t bands[SPECTRUM_NUM_BANDS];
    bandsOfNote(493.88, bands);
    const double db = bandDb(bands, 4, 3);
    if (fabs(db - 4.8) > DB_TOLERANCE) {
        printf("Si4: banda 4 sobre banda 3 por %.1f dB, esperado 4,8\n", db);
    }
    CHECK(fabs(db - 4.8) <= DB_TOLERANCE);
    for (uint32_t b = 1; b <= SPECTRUM_NUM_BANDS; b++) {
        CHECK(b == 4 || bands[b - 1] < bands[3]);
    }
}

/* Sol4 (392,00 Hz) está en la posición 6,270: banda 3 por el bin 6 (δ = 0,270, |W| = 0,953) y banda 4 por el bin 8
 * (δ = 1,730, |W| = 0,069): la banda 3 queda 22,8 dB sobre la 4, y las demás, todavía más abajo. Casi una sola banda:
 * pedimos al menos 20 dB de diferencia con cualquier otra */
static void testSol4CasiEnUnaSolaBanda(void) {
    uint16_t bands[SPECTRUM_NUM_BANDS];
    bandsOfNote(392.00, bands);
    for (uint32_t b = 1; b <= SPECTRUM_NUM_BANDS; b++) {
        if (b == 3) {
            continue;
        }
        const double db = (bands[b - 1] == 0) ? 99.0 : bandDb(bands, 3, b);
        if (db < 20.0) {
            printf("Sol4: banda 3 sobre banda %u por %.1f dB, esperado al menos 20\n", (unsigned)b, db);
        }
        CHECK(db >= 20.0);
    }
}

/* Criterio 1: "con una onda de varios armónicos, suben las bandas de cada armónico". Tonos centrados en un bin, así
 * que la teoría da valores exactos: cada banda vale la amplitud de su tono, más la fuga de la Hann (la mitad de la
 * amplitud) donde el bin vecino cae en otra banda. Las bandas sin tono ni fuga valen 0 */
static void expectBands(const char *name, const uint16_t bands[SPECTRUM_NUM_BANDS],
                        const double expected[SPECTRUM_NUM_BANDS]) {
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        if (!magnitudeNear(expected[b], bands[b])) {
            printf("%s: banda %u vale %u, esperado %.0f\n", name, (unsigned)(b + 1), (unsigned)bands[b], expected[b]);
        }
        CHECK(magnitudeNear(expected[b], bands[b]));
    }
}

static void addSineAtBin(int16_t samples[FFT_SIZE], uint32_t bin, double amplitude) {
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = (int16_t)(samples[n] + lround(amplitude * sin(TWO_PI * bin * n / FFT_SIZE)));
    }
}

static void testVariosArmonicosSubenSusBandas(void) {
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];

    /* Tonos en los bins 5, 20 y 80 (bandas 3, 5 y 7): sus vecinos (4 y 6, 19 y 21, 79 y 81) son de la misma banda */
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = 0;
    }
    addSineAtBin(samples, 5, 12000.0);
    addSineAtBin(samples, 20, 6000.0);
    addSineAtBin(samples, 80, 3000.0);
    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
    static const double threeTones[SPECTRUM_NUM_BANDS] = {0, 0, 12000, 0, 6000, 0, 3000, 0};
    expectBands("bins 5, 20 y 80", bands, threeTones);

    /* Octavas de un fundamental en el bin 4 (bins 4, 8, 16, 32, 64 y 128: una banda cada una, de la 3 a la 8), con la
     * mitad de amplitud en cada octava. El tono del bin 4 deja la mitad de su amplitud en el bin 3, de la banda 2 */
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        samples[n] = 0;
    }
    static const uint32_t octaveBins[] = {4, 8, 16, 32, 64, 128};
    double amplitude = 8000.0;
    for (uint32_t i = 0; i < sizeof(octaveBins) / sizeof(octaveBins[0]); i++) {
        addSineAtBin(samples, octaveBins[i], amplitude);
        amplitude /= 2.0;
    }
    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
    static const double octaves[SPECTRUM_NUM_BANDS] = {0, 4000, 8000, 4000, 2000, 1000, 500, 250};
    expectBands("octavas desde el bin 4", bands, octaves);
}

/* Niveles de 0 a 8 (spec 3.6, criterio 2). Escala de la spec: el nivel 8 desde 1/2 del fondo de escala (32768) y el
 * nivel 1 desde 1/256, un nivel por cada duplicación. Todos estos tests parten de las barras en 0: miden la subida */
#define LEVEL_1_MAGNITUDE    128     /* 32768 / 256 */
#define LEVEL_8_MAGNITUDE    16384   /* 32768 / 2 */

static void zeroLevels(uint8_t levels[SPECTRUM_NUM_BANDS]) {
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        levels[b] = 0;
    }
}

/* Los niveles de un tono centrado en el bin 40 (banda 6), con las barras en 0 antes */
static uint8_t levelOfTone(double amplitude) {
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];
    uint8_t levels[SPECTRUM_NUM_BANDS];

    fillSineAtBin(samples, 40, amplitude);
    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
    zeroLevels(levels);
    spectrumLevels(bands, levels);
    return levels[5];
}

/* Criterio 2: cada nivel queda entre 0 y 8, con cualquier amplitud, y a fondo de escala la barra llega a 8 */
static void testNivelesEntre0Y8(void) {
    static const double amplitudes[] = {1, 10, 100, 1000, 5000, 10000, 20000, 32767};
    for (uint32_t i = 0; i < sizeof(amplitudes) / sizeof(amplitudes[0]); i++) {
        CHECK(levelOfTone(amplitudes[i]) <= SPECTRUM_MAX_LEVEL);
    }
    CHECK(levelOfTone(32767.0) == SPECTRUM_MAX_LEVEL);

    /* La magnitud más alta que cabe en una banda no pasa de 8 */
    uint16_t loud[SPECTRUM_NUM_BANDS];
    uint8_t levels[SPECTRUM_NUM_BANDS];
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        loud[b] = 65535;
    }
    zeroLevels(levels);
    spectrumLevels(loud, levels);
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        CHECK(levels[b] == SPECTRUM_MAX_LEVEL);
    }
}

/* Con silencio, las 8 barras quedan en 0 */
static void testSilencioDaCero(void) {
    static int16_t samples[FFT_SIZE];       /* en 0 */
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];
    uint8_t levels[SPECTRUM_NUM_BANDS];

    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
    zeroLevels(levels);
    spectrumLevels(bands, levels);
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        CHECK(levels[b] == 0);
    }
}

/* Escala de 6 dB: un nivel más cada vez que la amplitud se duplica. Por tono, con amplitudes en el centro geométrico
 * de cada escalón (el umbral por √2), lejos de los bordes; y los bordes exactos, con la magnitud de la banda */
static void testNivelSubeUnPasoPorCada6dB(void) {
    static const struct { double amplitude; uint8_t level; } steps[] = {
        {90, 0}, {181, 1}, {362, 2}, {724, 3}, {1448, 4}, {2896, 5}, {5793, 6}, {11585, 7}, {23170, 8},
    };
    for (uint32_t i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
        const uint8_t level = levelOfTone(steps[i].amplitude);
        if (level != steps[i].level) {
            printf("amplitud %.0f: nivel %u, esperado %u\n", steps[i].amplitude, (unsigned)level,
                   (unsigned)steps[i].level);
        }
        CHECK(level == steps[i].level);
    }

    static const struct { uint16_t magnitude; uint8_t level; } edges[] = {
        {0, 0}, {LEVEL_1_MAGNITUDE - 1, 0}, {LEVEL_1_MAGNITUDE, 1}, {2 * LEVEL_1_MAGNITUDE - 1, 1},
        {2 * LEVEL_1_MAGNITUDE, 2}, {LEVEL_8_MAGNITUDE / 2 - 1, 6}, {LEVEL_8_MAGNITUDE / 2, 7},
        {LEVEL_8_MAGNITUDE - 1, 7}, {LEVEL_8_MAGNITUDE, 8}, {65535, 8},
    };
    for (uint32_t i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        uint16_t bands[SPECTRUM_NUM_BANDS] = {0};
        uint8_t levels[SPECTRUM_NUM_BANDS];
        bands[3] = edges[i].magnitude;
        zeroLevels(levels);
        spectrumLevels(bands, levels);
        if (levels[3] != edges[i].level) {
            printf("magnitud %u: nivel %u, esperado %u\n", (unsigned)edges[i].magnitude, (unsigned)levels[3],
                   (unsigned)edges[i].level);
        }
        CHECK(levels[3] == edges[i].level);
    }
}

/* Estabilidad (criterio 2): con una nota sostenida, cada barra no varía más de ±1 nivel entre lecturas. El cuadro de
 * captura arranca en cualquier fase de la nota, así que se mide el mismo tono con 16 fases de arranque distintas.
 * Se pide algo más estricto que la story: que entre todas las lecturas cada barra varíe a lo sumo 1 nivel */
#define PHASE_COUNT   16
#define DAC_LSB_Q15   64            /* una cuenta del DAC de 10 bits, llevada a Q15 (spec 3.6) */

/* Niveles de un tono con la fase de arranque dada; con quantize, las muestras pasan por la resolución del DAC */
static void levelsOfPhasedTone(double hz, double amplitude, double phase, int quantize,
                               uint8_t levels[SPECTRUM_NUM_BANDS]) {
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];

    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        double v = amplitude * sin(TWO_PI * hz * n / FS + phase);
        if (quantize) {
            v = DAC_LSB_Q15 * round(v / DAC_LSB_Q15);
        }
        samples[n] = (int16_t)lround(v);
    }
    spectrumMagnitudes(samples, magnitudes);
    spectrumBands(magnitudes, bands);
    zeroLevels(levels);
    spectrumLevels(bands, levels);
}

/* Máxima diferencia entre lecturas de cada banda, con PHASE_COUNT fases. Devuelve el mayor de los ocho rangos */
static uint32_t levelSpanOverPhases(double hz, double amplitude, int quantize, uint8_t first[SPECTRUM_NUM_BANDS]) {
    uint8_t lo[SPECTRUM_NUM_BANDS];
    uint8_t hi[SPECTRUM_NUM_BANDS];
    uint8_t levels[SPECTRUM_NUM_BANDS];
    uint32_t span = 0;

    for (uint32_t i = 0; i < PHASE_COUNT; i++) {
        levelsOfPhasedTone(hz, amplitude, TWO_PI * i / PHASE_COUNT, quantize, levels);
        for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
            if (i == 0) {
                lo[b] = hi[b] = levels[b];
                first[b] = levels[b];
            } else {
                lo[b] = (levels[b] < lo[b]) ? levels[b] : lo[b];
                hi[b] = (levels[b] > hi[b]) ? levels[b] : hi[b];
            }
        }
    }
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        if ((uint32_t)(hi[b] - lo[b]) > span) {
            span = (uint32_t)(hi[b] - lo[b]);
        }
    }
    return span;
}

static void testNivelesEstablesConTonoFijo(void) {
    static const double notesHz[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};
    uint8_t first[SPECTRUM_NUM_BANDS];

    for (uint32_t i = 0; i < sizeof(notesHz) / sizeof(notesHz[0]); i++) {
        const uint32_t span = levelSpanOverPhases(notesHz[i], 8000.0, 0, first);
        if (span > 1) {
            printf("%.2f Hz: una barra varía %u niveles entre fases\n", notesHz[i], (unsigned)span);
        }
        CHECK(span <= 1);
    }
}

/* Poca amplitud: un tono de 5 cuentas del DAC (320 en Q15), cuantizado como el DAC. Tiene que verse (nivel ≥ 1), no
 * variar más de 1 nivel entre fases y no levantar bandas lejanas: el ruido de cuantización del DAC (≈ 18 rms en Q15)
 * queda muy por debajo del piso del nivel 1 (128) */
static void testNivelEstableConPocaAmplitud(void) {
    static const struct { double hz; uint32_t band; } tones[] = {
        {440.0, 3}, {2000.0, 5}, {8000.0, 7},          /* banda de cada tono, de 1 a 8 */
    };
    uint8_t first[SPECTRUM_NUM_BANDS];

    for (uint32_t i = 0; i < sizeof(tones) / sizeof(tones[0]); i++) {
        const uint32_t span = levelSpanOverPhases(tones[i].hz, 5.0 * DAC_LSB_Q15, 1, first);
        if (span > 1) {
            printf("%.0f Hz con 5 cuentas: una barra varía %u niveles entre fases\n", tones[i].hz, (unsigned)span);
        }
        CHECK(span <= 1);
        CHECK(first[tones[i].band - 1] >= 1);
        for (uint32_t b = 1; b <= SPECTRUM_NUM_BANDS; b++) {
            const uint32_t gap = (b > tones[i].band) ? b - tones[i].band : tones[i].band - b;
            if (gap >= 2) {
                CHECK(first[b - 1] == 0);
            }
        }
    }
}

/* Caída gradual (spec 3.6): con el tono cortado, la barra baja un nivel por cuadro, en vez de caer de golpe, hasta 0;
 * y si el tono vuelve, sube de inmediato */
static void testCaidaGradualAlCortar(void) {
    static int16_t silence[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];
    uint16_t bands[SPECTRUM_NUM_BANDS];
    uint8_t levels[SPECTRUM_NUM_BANDS];

    /* Un tono a fondo de escala en la banda 6 (bin 40): la barra sube a 8 */
    zeroLevels(levels);
    {
        static int16_t tone[FFT_SIZE];
        fillSineAtBin(tone, 40, 32000.0);
        spectrumMagnitudes(tone, magnitudes);
        spectrumBands(magnitudes, bands);
        spectrumLevels(bands, levels);
        CHECK(levels[5] == SPECTRUM_MAX_LEVEL);
    }

    /* Silencio: 7, 6, 5, ... 0 en los cuadros siguientes, y se queda en 0 */
    spectrumMagnitudes(silence, magnitudes);
    spectrumBands(magnitudes, bands);
    for (int expected = SPECTRUM_MAX_LEVEL - 1; expected >= 0; expected--) {
        spectrumLevels(bands, levels);
        if (levels[5] != expected) {
            printf("cuadro de silencio: barra en %u, esperado %d\n", (unsigned)levels[5], expected);
        }
        CHECK(levels[5] == expected);
    }
    spectrumLevels(bands, levels);
    CHECK(levels[5] == 0);

    /* Sin dar un salto hacia abajo: de 8, con un tono de nivel 5, la barra queda en 7; y si el tono sube, salta */
    levels[5] = 8;
    bands[5] = 1448;                                 /* nivel 4: debería ir a 7 (baja un paso), no a 4 */
    spectrumLevels(bands, levels);
    CHECK(levels[5] == 7);
    bands[5] = 16384;                                /* nivel 8: sube de inmediato */
    spectrumLevels(bands, levels);
    CHECK(levels[5] == 8);
}

/* Bin del pico, para leerlo en el depurador (story 15, criterio 1): el de mayor magnitud entre el 1 y el 255. Se prueba con
 * magnitudes armadas a mano, para que no dependa de la FFT */
static void testSpectrumPeakBinElMayorSinContarLaContinua(void) {
    static uint16_t magnitudes[SPECTRUM_BINS];

    for (uint32_t k = 0; k < SPECTRUM_BINS; k++) {
        magnitudes[k] = 0;
    }
    CHECK(spectrumPeakBin(magnitudes) == 0);          /* silencio: no hay pico */

    magnitudes[40] = 1000;
    CHECK(spectrumPeakBin(magnitudes) == 40);
    magnitudes[255] = 1001;                           /* el último bin también cuenta */
    CHECK(spectrumPeakBin(magnitudes) == 255);
    magnitudes[1] = 2000;                             /* y el primero */
    CHECK(spectrumPeakBin(magnitudes) == 1);
    magnitudes[0] = 60000;                            /* la continua no es un pico, por grande que sea */
    CHECK(spectrumPeakBin(magnitudes) == 1);
    magnitudes[7] = 2000;                             /* empate: el más bajo */
    CHECK(spectrumPeakBin(magnitudes) == 1);
    magnitudes[1] = 1999;
    CHECK(spectrumPeakBin(magnitudes) == 7);
}

/* Con la FFT real: una nota entre bins da su bin más cercano (Si4, a 7,90, da el 8) */
static void testSpectrumPeakBinDeUnaNota(void) {
    static int16_t samples[FFT_SIZE];
    static uint16_t magnitudes[SPECTRUM_BINS];

    fillSineAtHz(samples, 493.88, 16000.0);
    spectrumMagnitudes(samples, magnitudes);
    CHECK(spectrumPeakBin(magnitudes) == 8);
}

int main(void) {
    testSpectrumPeakBinElMayorSinContarLaContinua();
    testSpectrumPeakBinDeUnaNota();
    testNivelesEstablesConTonoFijo();
    testNivelEstableConPocaAmplitud();
    testCaidaGradualAlCortar();
    testNivelesEntre0Y8();
    testSilencioDaCero();
    testNivelSubeUnPasoPorCada6dB();
    testVariosArmonicosSubenSusBandas();
    testDo4Banda3SobreBanda2();
    testSi4Banda4SobreBanda3();
    testSol4CasiEnUnaSolaBanda();
    testAgrupacionDeBins();
    testContinuaNoEntraEnNingunaBanda();
    testSinDesbordeAEscalaMaxima();
    testMagnitudIgualALaAmplitudEnBinCentrado();
    testPicoEnBinExacto();
    testPicoEntreBins();
    testFugaDeHannAcotada();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
