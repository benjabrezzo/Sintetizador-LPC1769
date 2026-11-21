/* Mini-asserts de los tests de la PC (spec 7.6). CHECK cuenta la falla y sigue, para ver todas las fallas
 * de una corrida. Cada test_*.c es un programa: termina con 1 si failures > 0. */
#ifndef TEST_CHECK_H
#define TEST_CHECK_H

#include <stdio.h>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FALLA %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

#endif /* TEST_CHECK_H */
