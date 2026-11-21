#include "spectrum.h"

#define FFT_STAGES  9                    /* log2(FFT_SIZE) */

/* Una senoide de amplitud A centrada en un bin da en la FFT A·N/2; la Hann la deja en la mitad (ganancia coherente
 * 0,5) y el escalado de 1/2 por etapa divide por N: queda A/4. Este factor la lleva de vuelta a A (spec 3.6) */
#define MAGNITUDE_GAIN  4

#define LEVEL_1_MAGNITUDE  128           /* 1/256 del fondo de escala (32768): primer nivel de la escala (spec 3.6) */

/* Parte real e imaginaria de trabajo (1 KB cada una) */
static int16_t re[FFT_SIZE];
static int16_t im[FFT_SIZE];

/* Invierte el orden de los FFT_STAGES bits de n: la FFT de decimación en el tiempo pide la entrada en ese orden */
static uint32_t bitReverse(uint32_t n) {
    uint32_t r = 0;
    for (uint32_t b = 0; b < FFT_STAGES; b++) {
        r = (r << 1) | ((n >> b) & 1u);
    }
    return r;
}

/* Raíz cuadrada entera por aproximaciones de a un bit: floor(√x) */
static uint32_t isqrt(uint32_t x) {
    uint32_t root = 0;
    for (uint32_t bit = 1u << 15; bit != 0; bit >>= 1) {
        const uint32_t trial = root | bit;
        if (trial * trial <= x) {
            root = trial;
        }
    }
    return root;
}

void spectrumMagnitudes(const int16_t samples[FFT_SIZE], uint16_t magnitudes[SPECTRUM_BINS]) {
    /* Ventana Hann y reordenamiento por bit-reverse */
    for (uint32_t n = 0; n < FFT_SIZE; n++) {
        const uint32_t r = bitReverse(n);
        re[r] = (int16_t)(((int32_t)samples[n] * hannWindow[n]) >> 15);
        im[r] = 0;
    }

    /* Mariposas: en cada etapa, el escalado de 1/2 evita el desborde (|a ± W·b| ≤ 2 · 32767 en 32 bits). El +1 antes
     * del desplazamiento redondea en vez de truncar: el truncado sesga hacia abajo y deja un ruido de hasta −62 dB
     * en los bins bajos (medido en la story 15), contra −78 dB redondeando.
     * W = cos − j·sin, así que W·b = (b.re·cos + b.im·sin) + j·(b.im·cos − b.re·sin) */
    for (uint32_t size = 2; size <= FFT_SIZE; size <<= 1) {
        const uint32_t half = size / 2;
        const uint32_t twiddleStep = FFT_SIZE / size;
        for (uint32_t start = 0; start < FFT_SIZE; start += size) {
            for (uint32_t j = 0; j < half; j++) {
                const int32_t cosW = fftTwiddleCos[j * twiddleStep];
                const int32_t sinW = fftTwiddleSin[j * twiddleStep];
                const uint32_t a = start + j;
                const uint32_t b = a + half;
                const int32_t tRe = ((int32_t)re[b] * cosW + (int32_t)im[b] * sinW) >> 15;
                const int32_t tIm = ((int32_t)im[b] * cosW - (int32_t)re[b] * sinW) >> 15;
                const int32_t aRe = re[a];
                const int32_t aIm = im[a];
                re[a] = (int16_t)((aRe + tRe + 1) >> 1);
                im[a] = (int16_t)((aIm + tIm + 1) >> 1);
                re[b] = (int16_t)((aRe - tRe + 1) >> 1);
                im[b] = (int16_t)((aIm - tIm + 1) >> 1);
            }
        }
    }

    for (uint32_t k = 0; k < SPECTRUM_BINS; k++) {
        const int32_t x = re[k];
        const int32_t y = im[k];
        magnitudes[k] = (uint16_t)(MAGNITUDE_GAIN * isqrt((uint32_t)(x * x + y * y)));
    }
}

void spectrumBands(const uint16_t magnitudes[SPECTRUM_BINS], uint16_t bands[SPECTRUM_NUM_BANDS]) {
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        uint16_t peak = 0;
        for (uint32_t k = 1u << b; k < (2u << b); k++) {      /* bins 2^b ... 2^(b+1) − 1; el bin 0 nunca entra */
            if (magnitudes[k] > peak) {
                peak = magnitudes[k];
            }
        }
        bands[b] = peak;
    }
}

/* Un nivel por cada umbral que la magnitud alcanza: 128, 256, 512 ... 16384 (cada uno, el doble del anterior) */
static uint8_t levelOf(uint16_t magnitude) {
    uint8_t level = 0;
    uint32_t threshold = LEVEL_1_MAGNITUDE;
    while (level < SPECTRUM_MAX_LEVEL && magnitude >= threshold) {
        level++;
        threshold <<= 1;
    }
    return level;
}

void spectrumLevels(const uint16_t bands[SPECTRUM_NUM_BANDS], uint8_t levels[SPECTRUM_NUM_BANDS]) {
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        const uint8_t target = levelOf(bands[b]);
        if (target >= levels[b]) {
            levels[b] = target;             /* sube de inmediato */
        } else {
            levels[b]--;                    /* baja de a un nivel por cuadro */
        }
    }
}

uint32_t spectrumPeakBin(const uint16_t magnitudes[SPECTRUM_BINS]) {
    uint32_t peak = 0;
    uint16_t peakMagnitude = 0;
    for (uint32_t k = 1; k < SPECTRUM_BINS; k++) {          /* el bin 0 (continua) no cuenta */
        if (magnitudes[k] > peakMagnitude) {                 /* estrictamente mayor: con empate gana el más bajo */
            peak = k;
            peakMagnitude = magnitudes[k];
        }
    }
    return peak;
}
