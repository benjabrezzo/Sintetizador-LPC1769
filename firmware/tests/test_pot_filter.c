/* Tests del filtro de las lecturas de los potenciómetros (Bloque 7: "filtrar o suavizar las lecturas ADC").
 * Las lecturas son de 12 bits (0 ... 4095) y llegan una por milisegundo (SysTick). */
#include <stdio.h>
#include <stdint.h>
#include "pot_filter.h"
#include "test_check.h"

/* Un filtro nuevo arranca en la primera lectura, no en 0: al encender, los parámetros no barren desde el mínimo
 * hasta la posición del pote. Se prueba en las dos puntas y en el medio: la primera actualización con la misma
 * lectura devuelve esa lectura. */
static void testUnFiltroNuevoArrancaEnLaPrimeraLectura(void) {
    const uint16_t readings[] = {0, 2048, 4095};

    for (uint32_t i = 0; i < sizeof readings / sizeof readings[0]; i++) {
        PotFilter f;
        potFilterInit(&f, readings[i]);

        CHECK(potFilterUpdate(&f, readings[i]) == readings[i]);
    }
}

/* Pasa la lectura de golpe de from a to y comprueba que el filtro la sigue: en ningún paso se pasa del destino ni
 * retrocede, y llega a 1 cuenta del destino en 150 ms o menos. Se miran 300 pasos, para ver también que se queda
 * ahí. */
static void checkStep(uint16_t from, uint16_t to) {
    PotFilter f;
    potFilterInit(&f, from);
    const int32_t direction = to > from ? 1 : -1;

    uint8_t overshot = 0;                     /* 1 = en algún paso se pasó del destino */
    uint8_t wentBack = 0;                     /* 1 = en algún paso se alejó del destino */
    uint32_t reachedStep = 0;                 /* primer paso a 1 cuenta del destino; 0 si no llegó */
    int32_t previous = from;
    for (uint32_t step = 1; step <= 300u; step++) {
        const int32_t value = potFilterUpdate(&f, to);
        if ((value - to) * direction > 0) {
            overshot = 1;
        }
        if ((value - previous) * direction < 0) {
            wentBack = 1;
        }
        if (reachedStep == 0 && value - to <= 1 && to - value <= 1) {
            reachedStep = step;
        }
        previous = value;
    }

    CHECK(!overshot);
    CHECK(!wentBack);
    CHECK(reachedStep != 0);
    CHECK(reachedStep <= 150u);
}

/* El pote girado de golpe de punta a punta, en los dos sentidos */
static void testSigueUnEscalonSinPasarse(void) {
    checkStep(0, 4095);
    checkStep(4095, 0);
}

/* Con el pote quieto, el ADC igual da unas cuentas de ruido. Una lectura que alterna 2048 + 8 y 2048 - 8 en cada ms
 * no tiene que pasar a la salida: se queda en 2048 ± 1 durante 1000 ms. */
static void testAtenuaElRuidoDePocasCuentas(void) {
    PotFilter f;
    potFilterInit(&f, 2048);

    uint16_t lowest = 2048;
    uint16_t highest = 2048;
    for (uint32_t step = 0; step < 1000u; step++) {
        const uint16_t reading = (step % 2u == 0) ? 2056 : 2040;
        const uint16_t value = potFilterUpdate(&f, reading);
        if (value < lowest) {
            lowest = value;
        }
        if (value > highest) {
            highest = value;
        }
    }

    CHECK(lowest >= 2047);
    CHECK(highest <= 2049);
}

int main(void) {
    testUnFiltroNuevoArrancaEnLaPrimeraLectura();
    testSigueUnEscalonSinPasarse();
    testAtenuaElRuidoDePocasCuentas();

    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
