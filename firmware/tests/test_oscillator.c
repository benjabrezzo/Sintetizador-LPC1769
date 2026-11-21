/* Tests del oscilador DDS (spec 3.1, Bloque 4).
 * Afinación: con f = ΔP · Fs / 2^24, el incremento de cada nota da su frecuencia de la tabla de la
 * sección 3.11 con menos de 1 cent de error.
 * Vuelta del acumulador: al pasar por 2^24, el índice de la tabla sigue sin saltos. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "config.h"
#include "tables.h"
#include "oscillator.h"
#include "test_check.h"

#define DAC_PCLK_HZ  25000000.0      /* PCLK del DAC: CCLK / 4 (DAC_Init) */
#define PHASE_WRAP   16777216.0      /* 2^24: vuelta de la fase (spec 3.1) */

/* Escala de Do mayor de Do4 a Do5, tal cual la tabla de la sección 3.11 */
static const double expectedHz[8] = {
    261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25
};

static void testCadaNotaDaSuFrecuenciaConMenosDeUnCent(void) {
    CHECK(NUM_NOTES == 8);                        /* las 8 teclas de la sección 3.11 */
    if (NUM_NOTES != 8) {
        return;
    }

    const double fs = DAC_PCLK_HZ / DAC_COUNTER_VALUE;   /* 25 MHz / 781 = 32 010,24 Hz */
    for (uint32_t n = 0; n < 8; n++) {
        const double f = noteSteps[n] * fs / PHASE_WRAP;
        const double cents = 1200.0 * log2(f / expectedHz[n]);
        if (fabs(cents) >= 1.0) {
            printf("nota %u: %.4f Hz, esperado %.2f Hz (%+.3f cents)\n",
                   (unsigned)n, f, expectedHz[n], cents);
        }
        CHECK(fabs(cents) < 1.0);
    }
}

/* Tabla rampa: cada muestra vale su índice, así la salida del oscilador muestra qué índice leyó */
static int16_t ramp[WAVE_SIZE];

static void buildRamp(void) {
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        ramp[i] = (int16_t)i;
    }
}

/* 4 bloques = 256 muestras: con Do5 la fase cruza 2^24 unas 4 veces, y hay 3 bordes de bloque */
#define BLOCKS_TO_CHECK 4

/* Lo que hará la ISR del DMA: un bloque de muestras seguidas con el mismo oscilador */
static void renderBlock(Oscillator *osc, int16_t *dst) {
    for (uint32_t i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        dst[i] = oscNextSample(osc, ramp);
    }
}

/* Do5 (523,25 Hz) avanza 523,25 · 256 / 32 010,24 ≈ 4,18 posiciones de la tabla por muestra: entre
 * dos muestras seguidas el índice sube 4 o 5 (módulo 256), también al dar la vuelta y entre bloques. */
static void testLaVueltaDelAcumuladorNoSaltaIndices(void) {
    Oscillator osc;
    oscInit(&osc, noteSteps[NUM_NOTES - 1]);      /* Do5: el incremento mayor */

    int16_t out[BLOCKS_TO_CHECK * AUDIO_BLOCK_SIZE];
    for (uint32_t b = 0; b < BLOCKS_TO_CHECK; b++) {
        renderBlock(&osc, &out[b * AUDIO_BLOCK_SIZE]);
    }

    uint32_t wraps = 0;
    for (uint32_t i = 0; i < BLOCKS_TO_CHECK * AUDIO_BLOCK_SIZE; i++) {
        CHECK(out[i] >= 0 && out[i] < WAVE_SIZE);     /* índice válido de la tabla */
        if (i == 0) {
            continue;
        }
        const int jump = (out[i] - out[i - 1] + WAVE_SIZE) % WAVE_SIZE;
        CHECK(jump == 4 || jump == 5);
        if (out[i] < out[i - 1]) {
            wraps++;                                  /* la fase pasó por 2^24 */
        }
    }
    CHECK(wraps > 0);                                 /* el recorrido de verdad dio la vuelta */
}

int main(void) {
    buildRamp();
    testCadaNotaDaSuFrecuenciaConMenosDeUnCent();
    testLaVueltaDelAcumuladorNoSaltaIndices();

    if (failures) {
        printf("%d falla(s)\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
