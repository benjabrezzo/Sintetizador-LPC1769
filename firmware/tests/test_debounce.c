/* Tests del antirrebote de las teclas (spec 3.11, Bloque 6).
 * Una tecla cambia de estado tras 10 ms estable y ese cambio genera la pulsación o la liberación.
 * Cada llamada a debounceUpdate es una lectura de SysTick (1 ms). Bit k = tecla k + 1. */
#include <stdio.h>
#include <stdint.h>
#include "debounce.h"
#include "test_check.h"

/* Sin teclas apretadas, ninguna lectura genera pulsaciones ni liberaciones */
static void testSinTeclasApretadasNoGeneraEventos(void) {
    Debounce d;
    debounceInit(&d);

    for (uint32_t ms = 0; ms < 100; ms++) {
        uint8_t pressed = 0xFF;      /* valores basura: la función tiene que escribirlos */
        uint8_t released = 0xFF;
        debounceUpdate(&d, 0x00, &pressed, &released);
        CHECK(pressed == 0);
        CHECK(released == 0);
    }
}

/* La tecla 1 (bit 0) apretada y estable: la primera lectura apretada es el instante 0 y la pulsación sale
 * 10 ms después, en la lectura número 11, ni antes ni después, y una sola vez aunque se la siga manteniendo.
 * 10 ms es el valor de la spec 3.11 */
static void testTeclaEstable10msGeneraUnaUnicaPulsacion(void) {
    Debounce d;
    debounceInit(&d);

    for (uint32_t reading = 1; reading <= 50; reading++) {
        uint8_t pressed = 0xFF;
        uint8_t released = 0xFF;
        debounceUpdate(&d, 0x01, &pressed, &released);
        CHECK(pressed == (reading == 11 ? 0x01 : 0x00));
        CHECK(released == 0);
    }
}

/* Un rebote al apretar dura menos de 10 ms: la tecla se lee apretada entre 1 y 10 lecturas seguidas y
 * vuelve a soltarse. Ninguna longitud genera eventos, ni durante el rebote ni después */
static void testReboteDeMenosDe10msAlApretarNoGeneraNada(void) {
    for (uint32_t bounceLength = 1; bounceLength <= 10; bounceLength++) {
        Debounce d;
        debounceInit(&d);

        for (uint32_t reading = 0; reading < bounceLength + 20; reading++) {
            uint8_t pressed = 0xFF;
            uint8_t released = 0xFF;
            debounceUpdate(&d, reading < bounceLength ? 0x01 : 0x00, &pressed, &released);
            CHECK(pressed == 0);
            CHECK(released == 0);
        }
    }
}

/* Con la tecla 1 ya apretada y estable, la primera lectura suelta es el instante 0 y la liberación sale
 * 10 ms después, en la lectura número 11, ni antes ni después, y una sola vez. Nunca hay una pulsación */
static void testSoltarConTeclaEstable10msGeneraUnaUnicaLiberacion(void) {
    Debounce d;
    debounceInit(&d);
    for (uint32_t reading = 0; reading < 30; reading++) {      /* apretada y estable */
        uint8_t pressed;
        uint8_t released;
        debounceUpdate(&d, 0x01, &pressed, &released);
    }

    for (uint32_t reading = 1; reading <= 50; reading++) {
        uint8_t pressed = 0xFF;
        uint8_t released = 0xFF;
        debounceUpdate(&d, 0x00, &pressed, &released);
        CHECK(pressed == 0);
        CHECK(released == (reading == 11 ? 0x01 : 0x00));
    }
}

/* Con la tecla 1 apretada y estable, un rebote al soltar dura menos de 10 ms: se lee suelta entre 1 y 10
 * lecturas seguidas y vuelve a apretarse. Ninguna longitud genera eventos, ni durante el rebote ni después */
static void testReboteDeMenosDe10msAlSoltarNoGeneraNada(void) {
    for (uint32_t bounceLength = 1; bounceLength <= 10; bounceLength++) {
        Debounce d;
        debounceInit(&d);
        for (uint32_t reading = 0; reading < 30; reading++) {  /* apretada y estable */
            uint8_t pressed;
            uint8_t released;
            debounceUpdate(&d, 0x01, &pressed, &released);
        }

        for (uint32_t reading = 0; reading < bounceLength + 20; reading++) {
            uint8_t pressed = 0xFF;
            uint8_t released = 0xFF;
            debounceUpdate(&d, reading < bounceLength ? 0x00 : 0x01, &pressed, &released);
            CHECK(pressed == 0);
            CHECK(released == 0);
        }
    }
}

/* Un rebote real alterna varias veces. Patrón de 6 lecturas en un estado y 1 en el otro, repetido: en total
 * hay mucho más de 10 lecturas distintas, pero nunca 10 ms seguidos, así que no genera eventos. Se prueba al
 * apretar (la tecla parte suelta) y al soltar (parte apretada y estable). Si el antirrebote acumulara las
 * lecturas distintas en lugar de pedirlas consecutivas, dispararía una activación falsa */
static void testReboteQueAlternaSinCompletar10msSeguidosNoGeneraNada(void) {
    for (uint32_t startPressed = 0; startPressed <= 1; startPressed++) {
        Debounce d;
        debounceInit(&d);
        for (uint32_t reading = 0; reading < 30; reading++) {  /* estado inicial estable */
            uint8_t pressed;
            uint8_t released;
            debounceUpdate(&d, startPressed ? 0x01 : 0x00, &pressed, &released);
        }

        for (uint32_t reading = 0; reading < 100; reading++) {
            const uint8_t differs = (reading % 7) < 6;         /* 6 lecturas distintas y 1 igual */
            const uint8_t raw = (differs != startPressed) ? 0x01 : 0x00;
            uint8_t pressed = 0xFF;
            uint8_t released = 0xFF;
            debounceUpdate(&d, raw, &pressed, &released);
            CHECK(pressed == 0);
            CHECK(released == 0);
        }
    }
}

/* Las ocho teclas se filtran cada una por su cuenta. La tecla k + 1 (bit k) se aprieta en la lectura k y se
 * suelta en la 60 + 2k, así que sus tiempos no coinciden con los de ninguna otra. Como la primera lectura
 * distinta es el instante 0, cada evento sale 10 lecturas después de ese cambio (la lectura número 11). Una
 * sola tecla, la 4 (bit 3), rebota todo el tiempo sin completar 10 ms seguidos: no tiene que generar nada
 * ni alterar a las otras */
#define BOUNCING_KEY  3

static void testLasOchoTeclasSeFiltranDeFormaIndependiente(void) {
    Debounce d;
    debounceInit(&d);

    for (uint32_t reading = 0; reading < 120; reading++) {
        uint8_t raw = 0;
        uint8_t expectedPressed = 0;
        uint8_t expectedReleased = 0;
        for (uint32_t k = 0; k < 8; k++) {
            const uint32_t pressAt = k;
            const uint32_t releaseAt = 60 + 2 * k;
            if (k == BOUNCING_KEY) {
                raw |= ((reading % 7) < 6) ? (1u << k) : 0u;   /* 6 lecturas apretada, 1 suelta */
                continue;
            }
            if (reading >= pressAt && reading < releaseAt) {
                raw |= (uint8_t)(1u << k);
            }
            if (reading == pressAt + 10) {
                expectedPressed |= (uint8_t)(1u << k);
            }
            if (reading == releaseAt + 10) {
                expectedReleased |= (uint8_t)(1u << k);
            }
        }

        uint8_t pressed = 0xFF;
        uint8_t released = 0xFF;
        debounceUpdate(&d, raw, &pressed, &released);
        CHECK(pressed == expectedPressed);
        CHECK(released == expectedReleased);
    }
}

int main(void) {
    testSinTeclasApretadasNoGeneraEventos();
    testTeclaEstable10msGeneraUnaUnicaPulsacion();
    testReboteDeMenosDe10msAlApretarNoGeneraNada();
    testSoltarConTeclaEstable10msGeneraUnaUnicaLiberacion();
    testReboteDeMenosDe10msAlSoltarNoGeneraNada();
    testReboteQueAlternaSinCompletar10msSeguidosNoGeneraNada();
    testLasOchoTeclasSeFiltranDeFormaIndependiente();
    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
