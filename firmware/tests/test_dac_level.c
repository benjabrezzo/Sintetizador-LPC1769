/* Tests de la conversión de muestra a valor del DAC (spec 3.1, Bloque 1).
 * La función es el paso 4 de la cadena de niveles: (y >> 6) + 512, saturado a 0...1023. */
#include <stdio.h>
#include <stdint.h>
#include "dac_level.h"
#include "test_check.h"

/* Muestra 0 (silencio) = media escala del DAC de 10 bits */
static void testCentroDaMediaEscala(void) {
    CHECK(dacValueFromSample(0) == 512);
}

/* Máximo de una muestra Q15 (+32767) = tope del DAC de 10 bits */
static void testExtremoPositivoDaMaximo(void) {
    CHECK(dacValueFromSample(32767) == 1023);
}

/* Mínimo de una muestra Q15 (-32768) = cero del DAC de 10 bits */
static void testExtremoNegativoDaCero(void) {
    CHECK(dacValueFromSample(-32768) == 0);
}

/* Por encima de +32767 (el ecualizador puede pasarse) el valor se queda en el tope 1023 */
static void testSaturaPorArriba(void) {
    CHECK(dacValueFromSample(32768) == 1023);
    CHECK(dacValueFromSample(1000000) == 1023);
}

/* Por debajo de -32768 el valor se queda en el piso 0 */
static void testSaturaPorAbajo(void) {
    CHECK(dacValueFromSample(-32769) == 0);
    CHECK(dacValueFromSample(-1000000) == 0);
}

/* Contador de saturaciones (spec 3.2, Bloque 5): cuenta solo lo que se recortó. Los extremos de una
 * muestra Q15 (+32767 y −32768) caen justo en 1023 y 0: caben en el DAC y no suman. Un paso más allá
 * (+32768 y −32769) daría 1024 y −1: se recortan y suman 1 cada una. */
static void testElContadorNoCuentaLosValoresExactosEn0Y1023(void) {
    uint32_t saturations = 0;

    CHECK(dacValueFromSampleCounted(0, &saturations) == 512);
    CHECK(dacValueFromSampleCounted(32767, &saturations) == 1023);
    CHECK(dacValueFromSampleCounted(-32768, &saturations) == 0);
    CHECK(saturations == 0);

    CHECK(dacValueFromSampleCounted(32768, &saturations) == 1023);
    CHECK(saturations == 1);
    CHECK(dacValueFromSampleCounted(-32769, &saturations) == 0);
    CHECK(saturations == 2);
}

/* Captura para la FFT (spec 3.6): el valor del DAC sin el 512, llevado a Q15 con 64 por cuenta. 512 es el silencio;
 * 0 y 1023 son los extremos del DAC, que quedan en −32768 y 32704 (la cuenta 1023 es la última antes de 1024) */
static void testDacValueToQ15EnElCentroYLosExtremos(void) {
    CHECK(dacValueToQ15(512) == 0);
    CHECK(dacValueToQ15(0) == -32768);
    CHECK(dacValueToQ15(1023) == 32704);
}

static void testDacValueToQ15SubeDeA64PorCuenta(void) {
    CHECK(dacValueToQ15(513) == 64);
    CHECK(dacValueToQ15(511) == -64);
    CHECK(dacValueToQ15(612) == 6400);
    CHECK(dacValueToQ15(412) == -6400);
}

/* Con la conversión del paso 4: una muestra de la mezcla que cae justo en un múltiplo de 64 vuelve igual; las demás
 * quedan en el múltiplo de 64 de abajo (el desplazamiento de 6 bits trunca), y +32767 en 32704 */
static void testDacValueToQ15ConLaConversionDelPaso4(void) {
    CHECK(dacValueToQ15(dacValueFromSample(0)) == 0);
    CHECK(dacValueToQ15(dacValueFromSample(6400)) == 6400);
    CHECK(dacValueToQ15(dacValueFromSample(-6400)) == -6400);
    CHECK(dacValueToQ15(dacValueFromSample(100)) == 64);
    CHECK(dacValueToQ15(dacValueFromSample(32767)) == 32704);
    CHECK(dacValueToQ15(dacValueFromSample(-32768)) == -32768);
    CHECK(dacValueToQ15(dacValueFromSample(1000000)) == 32704);     /* saturada */
}

int main(void) {
    testDacValueToQ15EnElCentroYLosExtremos();
    testDacValueToQ15SubeDeA64PorCuenta();
    testDacValueToQ15ConLaConversionDelPaso4();
    testCentroDaMediaEscala();
    testExtremoPositivoDaMaximo();
    testExtremoNegativoDaCero();
    testSaturaPorArriba();
    testSaturaPorAbajo();
    testElContadorNoCuentaLosValoresExactosEn0Y1023();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
