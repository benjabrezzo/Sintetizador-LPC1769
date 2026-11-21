/* Tests del ecualizador de tres bandas (spec 3.5, story 14). Los valores esperados salen de los criterios de la
 * story y de la respuesta teórica de cada banda, no del código del ecualizador. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "config.h"          /* AUDIO_BLOCK_SIZE */
#include "equalizer.h"
#include "test_check.h"

#define TWO_PI        6.283185307179586
#define FS            32010.243      /* Hz: 25 MHz / 781 (spec 3.1) */
#define FULL_SCALE    65534          /* pico de la mezcla con 4 voces al máximo: (4 · 32767) >> 1 */
#define SINE_SAMPLES  4096

#define Q2_30_SCALE      1073741824.0    /* 2^30 */
#define SHELF_Q          0.7071067811865476    /* 1/√2 en los dos shelves (spec 3.5) */
#define LOW_SHELF_HZ     200.0
#define PEAKING_HZ       1000.0
#define HIGH_SHELF_HZ    4000.0
#define PEAKING_Q        1.0             /* spec 3.5 */
#define TOLERANCE_DB     1.0             /* tolerancia de la respuesta (story 14) */
#define WARMUP_SAMPLES   4096            /* transitorio del filtro: se descarta */
#define MEASURE_SAMPLES  32768           /* un segundo: decenas de períodos de las frecuencias de prueba */
#define TEST_AMPLITUDE   20000.0         /* lejos del límite de 32 bits, con la salida bien por encima del ruido de truncado */

static int32_t toQ2_30(double value) {
    return (int32_t)lround(value * Q2_30_SCALE);
}

static EqBandCoefs identityCoefs(void) {
    const EqBandCoefs identity = {1 << 30, 0, 0, 0, 0};
    return identity;
}

/* Coeficientes del low-shelf del Audio EQ Cookbook (RBJ), normalizados con a0 = 1 y cuantizados a Q2.30. Se calculan
 * acá en double porque el firmware no calcula en flotante: la aplicación los calcula y los envía (spec 3.5). Sirven
 * de entrada del test; los valores esperados salen de la curva teórica del shelf, no de estas fórmulas */
static EqBandCoefs lowShelfCoefs(double f0, double gainDb, double q) {
    const double A = pow(10.0, gainDb / 40.0);
    const double w0 = TWO_PI * f0 / FS;
    const double cosW0 = cos(w0);
    const double twoSqrtAAlpha = 2.0 * sqrt(A) * (sin(w0) / (2.0 * q));
    const double a0 = (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
    EqBandCoefs c;
    c.b0 = toQ2_30(A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha) / a0);
    c.b1 = toQ2_30(2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0) / a0);
    c.b2 = toQ2_30(A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
    c.a1 = toQ2_30(-2.0 * ((A - 1.0) + (A + 1.0) * cosW0) / a0);
    c.a2 = toQ2_30(((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
    return c;
}

/* Coeficientes del peaking EQ del Audio EQ Cookbook (RBJ), con el mismo criterio que lowShelfCoefs */
static EqBandCoefs peakingCoefs(double f0, double gainDb, double q) {
    const double A = pow(10.0, gainDb / 40.0);
    const double w0 = TWO_PI * f0 / FS;
    const double cosW0 = cos(w0);
    const double alpha = sin(w0) / (2.0 * q);
    const double a0 = 1.0 + alpha / A;
    EqBandCoefs c;
    c.b0 = toQ2_30((1.0 + alpha * A) / a0);
    c.b1 = toQ2_30(-2.0 * cosW0 / a0);
    c.b2 = toQ2_30((1.0 - alpha * A) / a0);
    c.a1 = toQ2_30(-2.0 * cosW0 / a0);
    c.a2 = toQ2_30((1.0 - alpha / A) / a0);
    return c;
}

/* Coeficientes del high-shelf del Audio EQ Cookbook (RBJ), con el mismo criterio que lowShelfCoefs */
static EqBandCoefs highShelfCoefs(double f0, double gainDb, double q) {
    const double A = pow(10.0, gainDb / 40.0);
    const double w0 = TWO_PI * f0 / FS;
    const double cosW0 = cos(w0);
    const double twoSqrtAAlpha = 2.0 * sqrt(A) * (sin(w0) / (2.0 * q));
    const double a0 = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
    EqBandCoefs c;
    c.b0 = toQ2_30(A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha) / a0);
    c.b1 = toQ2_30(-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0) / a0);
    c.b2 = toQ2_30(A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
    c.a1 = toQ2_30(2.0 * ((A - 1.0) - (A + 1.0) * cosW0) / a0);
    c.a2 = toQ2_30(((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
    return c;
}

/* Ganancia en dB de una senoide de frecuencia freq (Hz) a través del ecualizador con esos coeficientes: cociente
 * entre el valor eficaz de la salida y el de la entrada, medido sobre MEASURE_SAMPLES muestras después de descartar
 * el transitorio. Arranca con el estado en cero */
static double measureGainDb(const EqBandCoefs coefs[EQ_NUM_BANDS], double freq) {
    Equalizer eq;
    equalizerInit(&eq);
    equalizerSetCoefs(&eq, coefs);

    double sumIn = 0.0;
    double sumOut = 0.0;
    for (uint32_t n = 0; n < WARMUP_SAMPLES + MEASURE_SAMPLES; n++) {
        const int32_t x = (int32_t)lround(TEST_AMPLITUDE * sin(TWO_PI * freq * n / FS));
        const int32_t y = equalizerProcess(&eq, x);
        if (n >= WARMUP_SAMPLES) {
            sumIn += (double)x * x;
            sumOut += (double)y * y;
        }
    }
    return 10.0 * log10(sumOut / sumIn);
}

/* Con las tres bandas en 0 dB el audio queda igual que antes del ecualizador (criterio 1 de la story): cada muestra
 * sale idéntica a como entró, bit a bit. Se prueban valores extremos y de signo alternado (el producto de 65534 por
 * b0 = 2^30 pasa de 32 bits) y senoides a escala máxima en la zona de cada banda */
static void testTresBandasEn0dBSonIdentidadBitABit(void) {
    Equalizer eq;
    equalizerInit(&eq);

    const int32_t extremes[] = {FULL_SCALE, -FULL_SCALE, 0, 1, -1, FULL_SCALE, FULL_SCALE, -FULL_SCALE, -FULL_SCALE,
                                32767, -32768, 0, 0};
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < sizeof extremes / sizeof extremes[0]; i++) {
        mismatches += (equalizerProcess(&eq, extremes[i]) != extremes[i]);
    }
    CHECK(mismatches == 0);

    const double freqs[] = {100.0, 1000.0, 4000.0, 10000.0};
    for (uint32_t f = 0; f < sizeof freqs / sizeof freqs[0]; f++) {
        mismatches = 0;
        for (uint32_t n = 0; n < SINE_SAMPLES; n++) {
            const int32_t x = (int32_t)lround(FULL_SCALE * sin(TWO_PI * freqs[f] * n / FS));
            mismatches += (equalizerProcess(&eq, x) != x);
        }
        CHECK(mismatches == 0);
    }
}

/* Low-shelf solo (peaking y high-shelf en 0 dB), a −12 y a +6 dB. La curva teórica del shelf de 200 Hz con Q = 1/√2
 * da: la ganancia G lejos de la rodilla (a 40 Hz, G ± 0,03 dB), G/2 en la frecuencia del shelf (200 Hz), 0 dB en los
 * agudos (a 5 kHz) y, en el flanco, a 133 Hz, −9,71 dB con G = −12 y +4,98 dB con G = +6 (un punto que fija el Q: con
 * Q = 1 o 0,5 el flanco de −12 dB cae a más de 1,4 dB de acá). La tolerancia es la de la story: ±1 dB */
static void testLowShelfSoloCambiaLosGravesYDejaLosAgudosEn0dB(void) {
    const double gainsDb[] = {-12.0, 6.0};
    const double flankDb[] = {-9.71, 4.98};
    for (uint32_t g = 0; g < sizeof gainsDb / sizeof gainsDb[0]; g++) {
        const double gainDb = gainsDb[g];
        const EqBandCoefs bands[EQ_NUM_BANDS] = {lowShelfCoefs(LOW_SHELF_HZ, gainDb, SHELF_Q), identityCoefs(), identityCoefs()};

        CHECK(fabs(measureGainDb(bands, 40.0) - gainDb) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 133.0) - flankDb[g]) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 200.0) - gainDb / 2.0) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 5000.0) - 0.0) <= TOLERANCE_DB);
    }
}

/* Peaking solo (low-shelf y high-shelf en 0 dB), a −12 y a +6 dB. La curva teórica del peaking de 1 kHz con Q = 1 da
 * la ganancia G en el centro; en el flanco, a 500 Hz, −3,96 dB con G = −12 y +1,88 dB con G = +6 (un punto que fija el
 * Q: con Q = 1/√2 el flanco de −12 dB ya cae a más de 1 dB de acá); y 0 dB lejos (a 100 Hz y a 10 kHz, a menos de 0,2 dB de 0).
 * Tolerancia de la story: ±1 dB */
static void testPeakingSoloCambiaLos1kHzYDejaElRestoEn0dB(void) {
    const double gainsDb[] = {-12.0, 6.0};
    const double flankDb[] = {-3.96, 1.88};
    for (uint32_t g = 0; g < sizeof gainsDb / sizeof gainsDb[0]; g++) {
        const double gainDb = gainsDb[g];
        const EqBandCoefs bands[EQ_NUM_BANDS] = {identityCoefs(), peakingCoefs(PEAKING_HZ, gainDb, PEAKING_Q), identityCoefs()};

        CHECK(fabs(measureGainDb(bands, 1000.0) - gainDb) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 500.0) - flankDb[g]) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 100.0) - 0.0) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 10000.0) - 0.0) <= TOLERANCE_DB);
    }
}

/* High-shelf solo (low-shelf y peaking en 0 dB), a −12 y a +6 dB. La respuesta del shelf de 4 kHz con Q = 1/√2 a
 * Fs = 32,01 kHz da: 0 dB en los graves (a 500 Hz), G/2 en la frecuencia del shelf (4 kHz), G arriba (a 12 kHz, G ± 0,02 dB)
 * y, en el flanco, a 6 kHz, −10,15 dB con G = −12 y +5,19 dB con G = +6 (un punto que fija el Q: con Q = 1 o 0,5 el
 * flanco de −12 dB cae a más de 1,5 dB de acá). Tolerancia de la story: ±1 dB */
static void testHighShelfSoloCambiaLosAgudosYDejaLosGravesEn0dB(void) {
    const double gainsDb[] = {-12.0, 6.0};
    const double flankDb[] = {-10.15, 5.19};
    for (uint32_t g = 0; g < sizeof gainsDb / sizeof gainsDb[0]; g++) {
        const double gainDb = gainsDb[g];
        const EqBandCoefs bands[EQ_NUM_BANDS] = {identityCoefs(), identityCoefs(), highShelfCoefs(HIGH_SHELF_HZ, gainDb, SHELF_Q)};

        CHECK(fabs(measureGainDb(bands, 500.0) - 0.0) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 4000.0) - gainDb / 2.0) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 6000.0) - flankDb[g]) <= TOLERANCE_DB);
        CHECK(fabs(measureGainDb(bands, 12000.0) - gainDb) <= TOLERANCE_DB);
    }
}

/* Las tres bandas a la vez (criterio 1 de la story: "las tres juntas"). Cada fila es una configuración de ganancias
 * (low-shelf, peaking, high-shelf) y cada columna, la ganancia esperada a 100, 200, 1000, 4000 y 8000 Hz: el producto
 * de las respuestas digitales de las tres bandas (RBJ, Fs = 32,01 kHz, Q = 1/√2, 1 y 1/√2), o sea la suma de sus dB.
 * Se calculó con numpy, fuera del firmware. Las dos primeras filas son las tres perillas juntas en −12 y en +6 dB; las
 * otras dos, un extremo distinto en cada banda */
static void testCascadaDeLasTresBandasCoincideConLaCurvaTeorica(void) {
    static const double configsDb[4][EQ_NUM_BANDS] = {
        {-12.0, -12.0, -12.0},
        {6.0, 6.0, 6.0},
        {-12.0, 6.0, -12.0},
        {6.0, -12.0, 6.0},
    };
    static const double freqs[5] = {100.0, 200.0, 1000.0, 4000.0, 8000.0};
    static const double expectedDb[4][5] = {
        {-11.26, -6.64, -12.08, -6.91, -11.71},
        {5.69, 3.27, 6.03, 3.38, 5.88},
        {-11.04, -5.74, 5.92, -5.62, -11.49},
        {5.46, 2.36, -11.97, 2.09, 5.66},
    };
    for (uint32_t c = 0; c < 4; c++) {
        const EqBandCoefs bands[EQ_NUM_BANDS] = {
            lowShelfCoefs(LOW_SHELF_HZ, configsDb[c][0], SHELF_Q),
            peakingCoefs(PEAKING_HZ, configsDb[c][1], PEAKING_Q),
            highShelfCoefs(HIGH_SHELF_HZ, configsDb[c][2], SHELF_Q),
        };
        for (uint32_t f = 0; f < 5; f++) {
            CHECK(fabs(measureGainDb(bands, freqs[f]) - expectedDb[c][f]) <= TOLERANCE_DB);
        }
    }
}

#define GRID_SAMPLES        512
#define GRID_INPUTS         4
#define WRAP_TOLERANCE      50.0         /* cuentas: ≈ 0,08 % de FULL_SCALE; el error real de la grilla es de 12,7. Un wrap-around da miles o millones */

/* Entradas a escala máxima (±FULL_SCALE): un escalón (la ganancia en continua), la alternancia muestra a muestra (la
 * ganancia en Nyquist) y senoides de 100 Hz (zona del low-shelf) y de 1 kHz (centro del peaking) */
static int32_t gridInput(uint32_t kind, uint32_t n) {
    switch (kind) {
    case 0:  return FULL_SCALE;
    case 1:  return (n % 2 == 0) ? FULL_SCALE : -FULL_SCALE;
    case 2:  return (int32_t)lround(FULL_SCALE * sin(TWO_PI * 100.0 * n / FS));
    default: return (int32_t)lround(FULL_SCALE * sin(TWO_PI * 1000.0 * n / FS));
    }
}

/* Una muestra por una banda en doble precisión, con los mismos coeficientes ya cuantizados. s = {x1, x2, y1, y2} */
static double referenceStep(const EqBandCoefs *c, double s[4], double x) {
    const double y = (c->b0 * x + c->b1 * s[0] + c->b2 * s[1] - c->a1 * s[2] - c->a2 * s[3]) / Q2_30_SCALE;
    s[1] = s[0];
    s[0] = x;
    s[3] = s[2];
    s[2] = y;
    return y;
}

/* Sin wrap-around en toda la grilla de ganancias: las 19 · 19 · 19 combinaciones de −12 a +6 dB en pasos de 1 dB, con
 * entrada a escala máxima (el pico de 4 voces, ±65534). La salida entera tiene que seguir a la de una cascada en doble
 * precisión con los mismos coeficientes cuantizados: si una cuenta diera la vuelta en 16 o 32 bits, el error sería de
 * miles o millones de cuentas, no de unas pocas decenas. Con la realimentación del error de equalizer.c, el error de
 * cuantización se queda en pocas cuentas incluso con el low-shelf a +6 dB (con truncar, llegaba a 933) */
static void testSinWrapAroundConEntradaAEscalaMaximaEnTodaLaGrilla(void) {
    uint32_t outOfTolerance = 0;
    for (int low = -12; low <= 6; low++) {
        for (int mid = -12; mid <= 6; mid++) {
            for (int high = -12; high <= 6; high++) {
                const EqBandCoefs bands[EQ_NUM_BANDS] = {
                    lowShelfCoefs(LOW_SHELF_HZ, low, SHELF_Q),
                    peakingCoefs(PEAKING_HZ, mid, PEAKING_Q),
                    highShelfCoefs(HIGH_SHELF_HZ, high, SHELF_Q),
                };
                for (uint32_t kind = 0; kind < GRID_INPUTS; kind++) {
                    Equalizer eq;
                    equalizerInit(&eq);
                    equalizerSetCoefs(&eq, bands);
                    double ref[EQ_NUM_BANDS][4] = {{0.0}};
                    for (uint32_t n = 0; n < GRID_SAMPLES; n++) {
                        const int32_t x = gridInput(kind, n);
                        const int32_t y = equalizerProcess(&eq, x);
                        double expected = x;
                        for (uint32_t b = 0; b < EQ_NUM_BANDS; b++) {
                            expected = referenceStep(&bands[b], ref[b], expected);
                        }
                        if (fabs((double)y - expected) > WRAP_TOLERANCE) {
                            if (outOfTolerance == 0) {
                                printf("primera falla: ganancias (%d, %d, %d) dB, entrada %u, muestra %u: salida %d, esperada %.0f\n",
                                       low, mid, high, (unsigned)kind, (unsigned)n, (int)y, expected);
                            }
                            outOfTolerance++;
                        }
                    }
                }
            }
        }
    }
    CHECK(outOfTolerance == 0);
}

#define SILENCE_SAMPLES      2048        /* largo de cada prueba */
#define SILENCE_ACTIVE       512         /* muestras con señal; después, la entrada es cero (tecla soltada) */
#define SILENCE_FROM         1536        /* desde esta muestra la salida tiene que ser exactamente 0 (el peor caso medido de la grilla se asienta en la 1185) */
#define SILENCE_INPUTS       4

/* Entradas a escala máxima que se cortan de golpe: un impulso, un escalón de SILENCE_ACTIVE muestras y senoides de 100 Hz
 * y de 1 kHz de SILENCE_ACTIVE muestras (cortadas en una fase cualquiera). Después, entrada cero */
static int32_t releaseInput(uint32_t kind, uint32_t n) {
    if (kind == 0) {
        return (n == 0) ? FULL_SCALE : 0;
    }
    if (n >= SILENCE_ACTIVE) {
        return 0;
    }
    switch (kind) {
    case 1:  return FULL_SCALE;
    case 2:  return (int32_t)lround(FULL_SCALE * sin(TWO_PI * 100.0 * n / FS));
    default: return (int32_t)lround(FULL_SCALE * sin(TWO_PI * 1000.0 * n / FS));
    }
}

/* La salida vuelve al silencio exacto en toda la grilla de ganancias (criterio 2 de la story: al soltar las teclas, con
 * cualquier combinación de perillas, la salida vuelve al silencio, sin oscilación ni zumbido). Con entrada cero, un
 * filtro estable y bien calculado decae a 0: si queda en un valor distinto (un offset de continua o un ciclo límite),
 * el DAC no vuelve al código 512 y suena un pop o un zumbido. Se pide y == 0 exacto en cada muestra desde
 * SILENCE_FROM, después de 1536 muestras (48 ms) de entrada cero o de decaimiento */
static void testLaSalidaVuelveAlSilencioExactoEnTodaLaGrilla(void) {
    uint32_t notSilent = 0;
    for (int low = -12; low <= 6; low++) {
        for (int mid = -12; mid <= 6; mid++) {
            for (int high = -12; high <= 6; high++) {
                const EqBandCoefs bands[EQ_NUM_BANDS] = {
                    lowShelfCoefs(LOW_SHELF_HZ, low, SHELF_Q),
                    peakingCoefs(PEAKING_HZ, mid, PEAKING_Q),
                    highShelfCoefs(HIGH_SHELF_HZ, high, SHELF_Q),
                };
                for (uint32_t kind = 0; kind < SILENCE_INPUTS; kind++) {
                    Equalizer eq;
                    equalizerInit(&eq);
                    equalizerSetCoefs(&eq, bands);
                    int32_t lastOutput = 0;
                    uint32_t nonZeroSamples = 0;
                    for (uint32_t n = 0; n < SILENCE_SAMPLES; n++) {
                        lastOutput = equalizerProcess(&eq, releaseInput(kind, n));
                        nonZeroSamples += (n >= SILENCE_FROM && lastOutput != 0);
                    }
                    if (nonZeroSamples > 0) {
                        if (notSilent == 0) {
                            printf("primera falla: ganancias (%d, %d, %d) dB, entrada %u: %u muestras distintas de 0, la última vale %d\n",
                                   low, mid, high, (unsigned)kind, (unsigned)nonZeroSamples, (int)lastOutput);
                        }
                        notSilent++;
                    }
                }
            }
        }
    }
    CHECK(notSilent == 0);
}

#define SWITCH_FIRST_BLOCK    10         /* el cambio ocurre en una muestra múltiplo del bloque de audio (64), entre bloques */
#define SWITCH_POSITIONS      5
#define SWITCH_SAMPLES_AFTER  512        /* muestras que se comparan después del cambio */
#define SWITCH_TOLERANCE      50.0       /* cuentas: la misma que el test de la grilla */

static void bandCoefsFromGains(const double gainsDb[EQ_NUM_BANDS], EqBandCoefs bands[EQ_NUM_BANDS]) {
    bands[0] = lowShelfCoefs(LOW_SHELF_HZ, gainsDb[0], SHELF_Q);
    bands[1] = peakingCoefs(PEAKING_HZ, gainsDb[1], PEAKING_Q);
    bands[2] = highShelfCoefs(HIGH_SHELF_HZ, gainsDb[2], SHELF_Q);
}

/* Cambiar los coeficientes entre bloques conserva el estado del filtro (criterio 2 de la story: al llevar una perilla de
 * un extremo al otro, el timbre cambia sin clicks ni pausas). Una senoide a escala máxima suena con una configuración y,
 * en una muestra múltiplo de 64, se cambia a otra. La salida tiene que seguir a la de una cascada en doble precisión que
 * cambia de coeficientes en la misma muestra y conserva sus estados: si el cambio reiniciara el estado, la salida
 * arrancaría de cero con la nueva configuración y el error sería de miles de cuentas. Se prueban pasos desde la
 * identidad y desde los dos extremos (−12 y +6 dB en las tres bandas) hacia la identidad y hacia las ocho combinaciones
 * de extremos, con senoides de 100 Hz y 1 kHz y cinco posiciones del cambio (fases distintas) */
static void testCambiarCoeficientesConservaElEstadoDelFiltro(void) {
    static const double fromDb[3][EQ_NUM_BANDS] = {{0.0, 0.0, 0.0}, {6.0, 6.0, 6.0}, {-12.0, -12.0, -12.0}};
    static const double toDb[9][EQ_NUM_BANDS] = {
        {0.0, 0.0, 0.0},
        {-12.0, -12.0, -12.0}, {-12.0, -12.0, 6.0}, {-12.0, 6.0, -12.0}, {-12.0, 6.0, 6.0},
        {6.0, -12.0, -12.0}, {6.0, -12.0, 6.0}, {6.0, 6.0, -12.0}, {6.0, 6.0, 6.0},
    };
    static const double freqs[2] = {100.0, 1000.0};
    uint32_t outOfTolerance = 0;

    for (uint32_t from = 0; from < 3; from++) {
        for (uint32_t to = 0; to < 9; to++) {
            EqBandCoefs before[EQ_NUM_BANDS];
            EqBandCoefs after[EQ_NUM_BANDS];
            bandCoefsFromGains(fromDb[from], before);
            bandCoefsFromGains(toDb[to], after);
            for (uint32_t f = 0; f < 2; f++) {
                for (uint32_t position = 0; position < SWITCH_POSITIONS; position++) {
                    const uint32_t switchAt = (SWITCH_FIRST_BLOCK + position) * AUDIO_BLOCK_SIZE;
                    Equalizer eq;
                    equalizerInit(&eq);
                    equalizerSetCoefs(&eq, before);
                    double ref[EQ_NUM_BANDS][4] = {{0.0}};
                    for (uint32_t n = 0; n < switchAt + SWITCH_SAMPLES_AFTER; n++) {
                        if (n == switchAt) {
                            equalizerSetCoefs(&eq, after);
                        }
                        const EqBandCoefs *current = (n < switchAt) ? before : after;
                        const int32_t x = (int32_t)lround(FULL_SCALE * sin(TWO_PI * freqs[f] * n / FS));
                        const int32_t y = equalizerProcess(&eq, x);
                        double expected = x;
                        for (uint32_t b = 0; b < EQ_NUM_BANDS; b++) {
                            expected = referenceStep(&current[b], ref[b], expected);
                        }
                        if (fabs((double)y - expected) > SWITCH_TOLERANCE) {
                            if (outOfTolerance == 0) {
                                printf("primera falla: de la configuración %u a la %u, %.0f Hz, cambio en la muestra %u, muestra %u: salida %d, esperada %.0f\n",
                                       (unsigned)from, (unsigned)to, freqs[f], (unsigned)switchAt, (unsigned)n, (int)y, expected);
                            }
                            outOfTolerance++;
                        }
                    }
                }
            }
        }
    }
    CHECK(outOfTolerance == 0);
}

int main(void) {
    testTresBandasEn0dBSonIdentidadBitABit();
    testLowShelfSoloCambiaLosGravesYDejaLosAgudosEn0dB();
    testPeakingSoloCambiaLos1kHzYDejaElRestoEn0dB();
    testHighShelfSoloCambiaLosAgudosYDejaLosGravesEn0dB();
    testCascadaDeLasTresBandasCoincideConLaCurvaTeorica();
    testSinWrapAroundConEntradaAEscalaMaximaEnTodaLaGrilla();
    testLaSalidaVuelveAlSilencioExactoEnTodaLaGrilla();
    testCambiarCoeficientesConservaElEstadoDelFiltro();

    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
