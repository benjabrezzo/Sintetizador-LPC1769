/* Tests de las tablas de onda (spec 3.8, Bloque 6).
 * Cuadrada, sierra y triangular se generan limitadas en banda y se normalizan para que su pico valga
 * exactamente el máximo de Q15: el rizado de Gibbs de la suma no puede desbordar. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "tables.h"
#include "test_check.h"

#define Q15_PEAK  32767    /* el máximo de una muestra Q15 (spec 3.8) */

#define NUM_HARMONIC_WAVES  3

/* Las tres ondas armónicas de la spec 3.8 */
static const int16_t *const harmonicWaves[NUM_HARMONIC_WAVES] = {squareTable, sawTable, triangleTable};

/* Cada tabla llega al pico positivo y al negativo, y ninguna muestra pasa de ahí: el valor mínimo de un
 * int16_t es -32768, que sería un desborde de la normalización, no un pico */
static void testCuadradaSierraYTriangularLlegaAlPicoQ15SinPasarlo(void) {
    for (uint32_t w = 0; w < NUM_HARMONIC_WAVES; w++) {
        int32_t maxSample = harmonicWaves[w][0];
        int32_t minSample = harmonicWaves[w][0];
        for (uint32_t n = 1; n < WAVE_SIZE; n++) {
            if (harmonicWaves[w][n] > maxSample) {
                maxSample = harmonicWaves[w][n];
            }
            if (harmonicWaves[w][n] < minSample) {
                minSample = harmonicWaves[w][n];
            }
        }
        CHECK(maxSample == Q15_PEAK);
        CHECK(minSample == -Q15_PEAK);
    }
}

/* Cada tabla es una suma de senoides: el coeficiente de la senoide de orden k se mide con una DFT en double.
 * La tolerancia es de 1 LSB. Medido con las tablas generadas: el redondeo a Q15 deja el desvío por debajo
 * de 0,15 LSB, y el armónico presente más chico (el 29 de la triangular) mide 32 LSB. */
#define TWO_PI            6.283185307179586
#define HALF_WAVE         (WAVE_SIZE / 2)
#define MAX_HARMONIC      30                  /* spec 3.3: 30 × 523,25 Hz = 15,7 kHz < 16 kHz */
#define LAST_HARMONIC     (HALF_WAVE - 1)     /* el último que una tabla de 256 muestras representa sin plegarse */
#define TOLERANCE_LSB     1.0

static double sineCoefficient(const int16_t *table, uint32_t k) {
    double sum = 0.0;
    for (uint32_t n = 0; n < WAVE_SIZE; n++) {
        sum += table[n] * sin(TWO_PI * k * n / WAVE_SIZE);
    }
    return 2.0 * sum / WAVE_SIZE;
}

static double cosineCoefficient(const int16_t *table, uint32_t k) {
    double sum = 0.0;
    for (uint32_t n = 0; n < WAVE_SIZE; n++) {
        sum += table[n] * cos(TWO_PI * k * n / WAVE_SIZE);
    }
    return 2.0 * sum / WAVE_SIZE;
}

/* Amplitud de cada armónico respecto de la fundamental, de la spec 3.8 */
static double squareRatio(uint32_t k) {
    return (k % 2 == 1) ? 1.0 / k : 0.0;
}

static double sawRatio(uint32_t k) {
    return ((k % 2 == 1) ? 1.0 : -1.0) / k;
}

static double triangleRatio(uint32_t k) {
    if (k % 2 == 0) {
        return 0.0;
    }
    return (((k - 1) / 2 % 2 == 0) ? 1.0 : -1.0) / ((double)k * k);
}

/* 1 si, de la fundamental al armónico MAX_HARMONIC, cada coeficiente es la fundamental por la razón de la
 * spec (con su signo: define si la onda sube o baja desde cero). Si no, dice en cuál falló. */
static int harmonicsMatchSpec(const int16_t *table, double (*ratio)(uint32_t)) {
    const double fundamental = sineCoefficient(table, 1);
    if (fundamental <= 0.0) {
        printf("la fundamental no es positiva: %.2f\n", fundamental);
        return 0;
    }
    for (uint32_t k = 1; k <= MAX_HARMONIC; k++) {
        const double measured = sineCoefficient(table, k);
        const double expected = fundamental * ratio(k);
        if (fabs(measured - expected) >= TOLERANCE_LSB) {
            printf("armónico %u: medido %.2f, esperado %.2f\n", (unsigned)k, measured, expected);
            return 0;
        }
    }
    return 1;
}

/* El armónico más alto de la tabla con amplitud de 1 LSB o más (0 si no tiene ninguno) */
static uint32_t highestHarmonic(const int16_t *table) {
    uint32_t highest = 0;
    for (uint32_t k = 1; k <= LAST_HARMONIC; k++) {
        if (hypot(sineCoefficient(table, k), cosineCoefficient(table, k)) >= TOLERANCE_LSB) {
            highest = k;
        }
    }
    return highest;
}

/* Con senoides puras, la muestra 0 vale cero y la tabla es impar: x[N - n] = -x[n] */
static void testLasTresOndasSonImparesRespectoDelPeriodoYEmpiezanEnCero(void) {
    for (uint32_t w = 0; w < NUM_HARMONIC_WAVES; w++) {
        CHECK(harmonicWaves[w][0] == 0);
        for (uint32_t n = 1; n < WAVE_SIZE; n++) {
            CHECK(harmonicWaves[w][WAVE_SIZE - n] == -harmonicWaves[w][n]);
        }
    }
}

/* Solo con armónicos impares la segunda mitad del período es la primera invertida: x[n + N/2] = -x[n].
 * La sierra tiene los pares, así que no cumple esto */
static void testCuadradaYTriangularTienenSimetriaDeMediaOnda(void) {
    const int16_t *const oddOnly[2] = {squareTable, triangleTable};
    for (uint32_t w = 0; w < 2; w++) {
        for (uint32_t n = 0; n < HALF_WAVE; n++) {
            CHECK(oddOnly[w][n + HALF_WAVE] == -oddOnly[w][n]);
        }
    }
}

static void testCuadradaTieneSoloArmonicosImparesConAmplitudUnoSobreK(void) {
    CHECK(harmonicsMatchSpec(squareTable, squareRatio));
}

static void testSierraTieneTodosLosArmonicosConAmplitudUnoSobreK(void) {
    CHECK(harmonicsMatchSpec(sawTable, sawRatio));
}

static void testTriangularTieneSoloImparesConAmplitudUnoSobreKCuadrado(void) {
    CHECK(harmonicsMatchSpec(triangleTable, triangleRatio));
}

static void testNingunaOndaTieneArmonicosPorEncimaDelTreinta(void) {
    for (uint32_t w = 0; w < NUM_HARMONIC_WAVES; w++) {
        CHECK(highestHarmonic(harmonicWaves[w]) <= MAX_HARMONIC);
    }
}

/* La sierra tiene todos los armónicos, así que llega al 30; la cuadrada y la triangular, solo impares: al 29 */
static void testLasOndasLleganHastaElArmonicoTreinta(void) {
    CHECK(highestHarmonic(sawTable) == MAX_HARMONIC);
    CHECK(highestHarmonic(squareTable) == MAX_HARMONIC - 1);
    CHECK(highestHarmonic(triangleTable) == MAX_HARMONIC - 1);
}

/* Ventana Hann de la FFT (spec 3.6, story 15). Periódica, como se usa para análisis espectral: w[0] = 0 y el
 * máximo (1,0) cae en N/2. Los valores esperados salen de la definición, no del generador:
 *  - w[0] = 0 y w[N/2] = 1,0 (32767 en Q15);
 *  - simétrica: w[N − n] = w[n];
 *  - w[N/4] = 0,5 (16384, con 1 LSB de tolerancia);
 *  - la suma de la ventana es N/2 (ganancia coherente 0,5), con 256 de tolerancia (≤ 0,5 LSB por muestra). */
#define HANN_HALF_GAIN_SUM   (FFT_SIZE / 2 * 32768)
#define HANN_SUM_TOLERANCE   256

static void testHannEsSimetricaConExtremoEnCeroYMaximoEnElCentro(void) {
    CHECK(hannWindow[0] == 0);
    CHECK(hannWindow[FFT_SIZE / 2] == Q15_PEAK);
    for (uint32_t n = 1; n < FFT_SIZE; n++) {
        CHECK(hannWindow[FFT_SIZE - n] == hannWindow[n]);
    }
    for (uint32_t n = 1; n <= FFT_SIZE / 2; n++) {
        CHECK(hannWindow[n] >= hannWindow[n - 1]);
    }
}

static void testHannVale0_5ALaMitadDelFlancoYSuGananciaCoherenteEs0_5(void) {
    CHECK(abs(hannWindow[FFT_SIZE / 4] - 16384) <= 1);
    int32_t sum = 0;
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        sum += hannWindow[n];
    }
    CHECK(abs(sum - HANN_HALF_GAIN_SUM) <= HANN_SUM_TOLERANCE);
}

/* Factores de giro de la FFT (spec 3.6, story 15): W_N^k = cos(2πk/N) − j·sin(2πk/N) para k = 0 ... N/2 − 1, en
 * dos tablas Q15 con el coseno y el seno (positivo) de 2πk/N. Se comparan con libm, con 1 LSB de tolerancia, y con
 * los ángulos conocidos: 0° (k = 0), 45° (k = N/8) y 90° (k = N/4). */
#define TWIDDLE_COUNT   (FFT_SIZE / 2)
#define Q15_ONE         32768.0

static void testFactoresDeGiroCoincidenConCosenoYSenoDeLaDefinicion(void) {
    for (uint32_t k = 0; k < TWIDDLE_COUNT; k++) {
        const double angle = TWO_PI * k / FFT_SIZE;
        CHECK(fabs(fftTwiddleCos[k] - Q15_ONE * cos(angle)) <= 1.0);
        CHECK(fabs(fftTwiddleSin[k] - Q15_ONE * sin(angle)) <= 1.0);
    }
}

static void testFactoresDeGiroEnLosAngulosConocidos(void) {
    CHECK(fftTwiddleCos[0] == Q15_PEAK);
    CHECK(fftTwiddleSin[0] == 0);
    CHECK(abs(fftTwiddleCos[FFT_SIZE / 8] - 23170) <= 1);
    CHECK(abs(fftTwiddleSin[FFT_SIZE / 8] - 23170) <= 1);
    CHECK(abs(fftTwiddleCos[FFT_SIZE / 4]) <= 1);
    CHECK(fftTwiddleSin[FFT_SIZE / 4] == Q15_PEAK);
}

int main(void) {
    testFactoresDeGiroCoincidenConCosenoYSenoDeLaDefinicion();
    testFactoresDeGiroEnLosAngulosConocidos();
    testHannEsSimetricaConExtremoEnCeroYMaximoEnElCentro();
    testHannVale0_5ALaMitadDelFlancoYSuGananciaCoherenteEs0_5();
    testCuadradaSierraYTriangularLlegaAlPicoQ15SinPasarlo();
    testLasTresOndasSonImparesRespectoDelPeriodoYEmpiezanEnCero();
    testCuadradaYTriangularTienenSimetriaDeMediaOnda();
    testCuadradaTieneSoloArmonicosImparesConAmplitudUnoSobreK();
    testSierraTieneTodosLosArmonicosConAmplitudUnoSobreK();
    testTriangularTieneSoloImparesConAmplitudUnoSobreKCuadrado();
    testNingunaOndaTieneArmonicosPorEncimaDelTreinta();
    testLasOndasLleganHastaElArmonicoTreinta();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
