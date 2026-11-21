/* Test de ejemplo de la lógica del firmware en la PC (spec 7.6).
 * Todavía no hay módulos de lógica: este test solo comprueba el arnés
 * (mini-asserts, Makefile y código de salida). */
#include <stdio.h>
#include "test_check.h"

static void testSumaBasica(void) {
    CHECK(2 + 2 == 4);
}

int main(void) {
    testSumaBasica();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
