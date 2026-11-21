#include <cr_section_macros.h>
#include "audio_out.h"
#include "config.h"
#include "tables.h"
#include "dac_level.h"
#include "block_cursor.h"
#include "oscillator.h"
#include "voice.h"
#include "wave_select.h"      /* identificadores de onda */
#include "mixer.h"
#include "equalizer.h"
#include "cycle_counter.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

/* Doble buffer de palabras para DACR: dos mitades de AUDIO_BLOCK_SIZE (2 ms cada una), en AHB (RAM2)
 * para que el DMA no compita con la CPU por RamLoc32. El DMA lee una mitad mientras la CPU llena
 * la otra. El DMA escribe DACR entero, así que BIAS queda en 0 (700 µA). */
__BSS(RAM2) static uint32_t outBuf[2][AUDIO_BLOCK_SIZE];

/* Una LLI por mitad, enlazadas en círculo: al terminar una mitad el DMA arranca la otra sin
 * intervención. También van en AHB (RAM2). Observado en la placa: con la LLI en RamLoc32 y arrancando
 * sin depurador, el canal cargaba ceros al terminar la primera pasada y la salida quedaba plana;
 * con el depurador conectado funcionaba. Con la LLI en RAM2 circula en ambos casos. */
__BSS(RAM2) static GPDMA_LLI_T dacLli[2];

/* Qué mitad llenar (lógica pura, probada en la PC) */
static BlockCursor cursor;

/* Las voces (lógica pura, probada en la PC). Empiezan libres (audioOutInit inicializa sus envolventes). La ISR del
 * DMA lee el nivel de la envolvente y avanza la fase de las que no están libres; SysTick las arranca, las suelta y
 * avanza sus envolventes. Como SysTick no puede interrumpir a la ISR pero la ISR sí puede interrumpir a SysTick,
 * SysTick deshabilita la IRQ del DMA mientras escribe las voces (audioIsrLock), así la ISR las ve completas. No
 * necesitan volatile: la ISR las lee en funciones de otro módulo y las barreras de compilador de audioIsrLock y
 * audioIsrUnlock fijan el orden de las escrituras. */
static Voice voices[NUM_VOICES];

/* Etapa y nivel de la envolvente de cada voz, para el depurador (ver audio_out.h). Copias: la fuente de verdad son
 * las voces. Global a propósito, para leerlas en la vista Expressions con la CPU suspendida */
volatile uint8_t envStage[NUM_VOICES] = {0};
volatile int16_t envLevel[NUM_VOICES] = {0};

/* La tabla de cada onda, indexada por el identificador de onda de la voz (wave_select.h). Es un arreglo de
 * punteros en RAM, no const: la quinta posición (Python) se cambia entre bloques de audio (spec 3.8). Hasta
 * que llegue la onda de Python (story 13) esa posición apunta a la senoide, así que ni un identificador que
 * se colara ahí lee un puntero nulo. */
static const int16_t *waveTables[NUM_WAVES] = {
    [WAVE_SQUARE]   = squareTable,
    [WAVE_SAW]      = sawTable,
    [WAVE_TRIANGLE] = triangleTable,
    [WAVE_SINE]     = sineTable,
    [WAVE_PYTHON]   = sineTable,
};

/* Buffers de la quinta onda (spec 3.10, en RamLoc32): uno suena, a través de waveTables[WAVE_PYTHON], y el otro recibe la
 * onda que llega por UART. Al activarla se intercambian, así nunca se escribe sobre la tabla que está sonando (spec 3.8).
 * Mientras no llegue ninguna, la quinta posición sigue en la senoide y los dos están libres. Solo los toca el loop principal
 * (la ISR del DMA lee la tabla activa por waveTables). */
static int16_t pythonWaveBuf[2][WAVE_SIZE];
static uint8_t pythonWaveRx = 0;      /* índice del buffer que recibe */

/* Ecualizador (spec 3.5). Su estado y sus coeficientes los toca solo la ISR del DMA. El loop principal deja los
 * coeficientes nuevos en uno de dos juegos y publica su puntero; la ISR lo toma al empezar un bloque, así nunca cambian
 * a mitad de un bloque ni se leen a medio escribir. El loop escribe siempre el juego que no está publicado (alterna), y
 * solo la ISR limpia el puntero: el loop no puede interrumpirla */
static Equalizer equalizer;
static EqBandCoefs eqCoefSets[2][EQ_NUM_BANDS];
static uint8_t eqNextSet = 0;                                 /* juego donde escribe el loop principal */
static const EqBandCoefs *volatile eqPendingCoefs = 0;        /* juego publicado que la ISR todavía no tomó */

/* Captura para el análisis espectral (spec 3.6): las muestras que van al DAC (ya saturadas, llevadas a Q15 con
 * dacValueToQ15), FFT_SIZE seguidas. La ISR las copia bloque a bloque mientras captureCount < FFT_SIZE y, cuando se
 * llena, deja de escribir: el buffer queda quieto hasta que el loop principal lo libera con audioOutCaptureRelease. Es
 * un handshake sin bloqueos: la ISR solo sube captureCount (y solo si no está lleno) y el loop solo lo baja a 0 (y solo
 * si está lleno), así que nunca se pisan. Está en RamLoc32 (1 KB). Un cuadro son 8 bloques: FFT_SIZE es múltiplo de
 * AUDIO_BLOCK_SIZE, así que la captura siempre se llena con bloques completos. */
_Static_assert(FFT_SIZE % AUDIO_BLOCK_SIZE == 0, "la captura se llena con bloques completos");
static int16_t captureBuf[FFT_SIZE];
static volatile uint32_t captureCount = 0;

/* Bloques que la ISR terminó tarde (spec 7.1). Global a propósito, para leerla en la vista Expressions
 * con la CPU suspendida */
volatile uint32_t audioUnderruns = 0;

/* Muestras recortadas por la mezcla (spec 3.2). Solo la escribe la ISR, una vez por bloque */
volatile uint32_t audioSaturations = 0;

/* Duración de la ISR (spec 3.12, Bloques 5 y 10): máximo de ciclos por bloque según las voces activas.
 * Global a propósito, para leerlo en la vista Expressions con la CPU suspendida */
volatile uint32_t audioIsrMaxCycles[NUM_VOICES + 1] = {0};

/* Anota la duración de un bloque con las lecturas de cycleCounterNow al empezar y al terminar (ciclos de CPU, módulo
 * 2^32, así que la resta sin signo da la duración aunque la cuenta dé la vuelta). Vale con ISR de cualquier duración
 * hasta unos 43 s. Antes se medía con SysTick, que da la vuelta cada 1 ms: una ISR más larga se medía mal (story 15). Se
 * llama con el pin ya bajo, fuera de lo que se mide. */
static void recordIsrCycles(uint32_t startCycles, uint32_t endCycles) {
    const uint32_t elapsed = endCycles - startCycles;
    uint32_t sounding = 0;
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        sounding += (voices[v].env.stage != ENV_IDLE);   /* SysTick no puede cambiarlas durante la ISR */
    }
    if (elapsed > audioIsrMaxCycles[sounding]) {
        audioIsrMaxCycles[sounding] = elapsed;
    }
}

/* Llena la próxima mitad con la mezcla de las voces activas (pasos 1 y 2 de la cadena de niveles), la pasa por el
 * ecualizador (paso 3) y la convierte al formato de DACR (paso 4 + desplazamiento a bits 15:6); copia además cada
 * muestra a la captura de la FFT mientras no esté llena. Si el loop principal
 * dejó coeficientes nuevos, se aplican al empezar el bloque. Las saturaciones del bloque se cuentan en una variable
 * local y se suman al global al final, así no se pasa un puntero a volatile */
static void fillNextBlock(void) {
    const uint8_t half = blockCursorNext(&cursor);
    uint32_t blockSaturations = 0;
    const EqBandCoefs *newCoefs = eqPendingCoefs;
    if (newCoefs != 0) {
        equalizerSetCoefs(&equalizer, newCoefs);
        eqPendingCoefs = 0;
    }
    const uint32_t captured = captureCount;               /* solo esta ISR lo sube; el loop lo baja cuando está lleno */
    const uint8_t capturing = captured < FFT_SIZE;
    for (uint32_t i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        const int32_t mixed = mixNextSample(voices, NUM_VOICES, waveTables);
        const int32_t y = equalizerProcess(&equalizer, mixed);     /* entre la mezcla y la saturación */
        const uint32_t dacValue = dacValueFromSampleCounted(y, &blockSaturations);
        outBuf[half][i] = DAC_VALUE(dacValue);
        if (capturing) {
            captureBuf[captured + i] = dacValueToQ15(dacValue);
        }
    }
    if (capturing) {
        captureCount = captured + AUDIO_BLOCK_SIZE;       /* publicado después de copiar el bloque entero */
    }
    audioSaturations += blockSaturations;
}

static void configMeasurePin(void) {
    GPIO_SetDir(MEASURE_PORT, 1u << MEASURE_PIN, GPIO_OUTPUT);
    GPIO_ClearPins(MEASURE_PORT, 1u << MEASURE_PIN);
}

static void configDac(void) {
    DAC_Init();                                   /* P0.26 como AOUT y PCLK = CCLK/4 = 25 MHz */
    DAC_SetDMATimeOut(DAC_COUNTER_VALUE);         /* Fs = 25 MHz / 781 */

    const DAC_CONVERTER_CFG_T dacCfg = {ENABLE, ENABLE, ENABLE};   /* doubleBuffer, dmaCounter, dmaRequest */
    DAC_ConfigDAConverterControl(&dacCfg);
}

static Status configDma(void) {
    GPDMA_Init();

    /* El primer tramo usa los registros del canal (mitad 0) y los siguientes estas LLI: tienen que
     * coincidir. Con el bit I, cada mitad que termina dispara la interrupción de fin de transferencia */
    const uint32_t control = GPDMA_DMACCxControl_TransferSize(AUDIO_BLOCK_SIZE)
                           | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                           | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                           | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
                           | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
                           | GPDMA_DMACCxControl_SI
                           | GPDMA_DMACCxControl_I;
    for (uint32_t i = 0; i < 2; i++) {
        dacLli[i].srcAddr = (uint32_t)outBuf[i];
        dacLli[i].dstAddr = (uint32_t)&LPC_DAC->DACR;   /* la dirección del registro, no su valor */
        dacLli[i].nextLLI = (uint32_t)&dacLli[1 - i];   /* la otra mitad */
        dacLli[i].control = control;
    }

    GPDMA_Channel_CFG_T dmaCfg = {0};
    dmaCfg.channelNum    = (GPDMA_CH)DAC_DMA_CHANNEL;
    dmaCfg.transferSize  = AUDIO_BLOCK_SIZE;
    dmaCfg.type          = GPDMA_M2P;
    dmaCfg.srcMemAddr    = (uint32_t)outBuf[0];
    dmaCfg.dstConn       = GPDMA_DAC;             /* en M2P el destino sale de la tabla del driver: DACR */
    dmaCfg.src.width     = GPDMA_WORD;
    dmaCfg.src.burst     = GPDMA_BSIZE_1;
    dmaCfg.src.increment = ENABLE;
    dmaCfg.dst.width     = GPDMA_WORD;
    dmaCfg.dst.burst     = GPDMA_BSIZE_1;
    dmaCfg.dst.increment = DISABLE;
    dmaCfg.intTC         = ENABLE;
    dmaCfg.intErr        = DISABLE;
    dmaCfg.linkedList    = (uint32_t)&dacLli[1];  /* arranca con la mitad 0 y sigue con la 1 */

    if (GPDMA_SetupChannel(&dmaCfg) == ERROR) {
        return ERROR;
    }
    NVIC_SetPriority(DMA_IRQn, DMA_IRQ_PRIORITY); /* spec 3.12: la más prioritaria */
    NVIC_EnableIRQ(DMA_IRQn);
    return SUCCESS;
}

/* Fin de una mitad: llena la que acaba de terminar mientras el DMA reproduce la otra */
void DMA_IRQHandler(void) {
    const GPDMA_CH ch = (GPDMA_CH)DAC_DMA_CHANNEL;

    /* Un solo vector para todos los canales: se revisa el bit de cada uno */
    if (GPDMA_IntGetStatus(GPDMA_INTTC, ch) == SET) {
        /* Se limpia al entrar: si vuelve a activarse durante la generación, el DMA ya terminó la otra mitad */
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, ch);
        GPIO_SetPins(MEASURE_PORT, 1u << MEASURE_PIN);
        const uint32_t startCycles = cycleCounterNow();

        fillNextBlock();

        /* spec 7.1: si el canal ya terminó la otra mitad, la generación llegó tarde. No se limpia el
         * flag: la ISR vuelve a entrar y llena la mitad que acaba de terminar, sin contarla dos veces. */
        if (GPDMA_IntGetStatus(GPDMA_INTTC, ch) == SET) {
            audioUnderruns++;
        }
        const uint32_t endCycles = cycleCounterNow();
        GPIO_ClearPins(MEASURE_PORT, 1u << MEASURE_PIN);
        recordIsrCycles(startCycles, endCycles);
    }
}

/* Bloqueo de la ISR del audio para escribir las voces. Las voces pueden estar sonando (la reutilizada, la que se
 * apaga, la más antigua) y la ISR del DMA (prioridad 0) puede interrumpir a SysTick entre dos escrituras
 * cualesquiera: la vería a medio escribir. Se deshabilita su IRQ mientras se escriben, así las ve completas sin
 * importar cuántos campos tengan. Si el DMA termina una mitad en ese lapso, la interrupción queda pendiente y
 * entra al habilitar: se atrasa unos ciclos, contra los 2 ms de un bloque.
 * DSB e ISB hacen que la baja ya valga antes de escribir. Las barreras de compilador impiden que mueva las
 * escrituras de las voces fuera del lapso (NVIC_DisableIRQ y NVIC_EnableIRQ solo ordenan lo volátil). Solo las
 * llama SysTick y no se anidan: cada función de este módulo toma y suelta el bloqueo una vez. */
static inline void audioIsrLock(void) {
    NVIC_DisableIRQ(DMA_IRQn);
    __DSB();
    __ISB();
    __asm volatile ("" ::: "memory");
}

static inline void audioIsrUnlock(void) {
    __asm volatile ("" ::: "memory");
    NVIC_EnableIRQ(DMA_IRQn);
}

/* Copia la etapa y el nivel de cada envolvente a las variables del depurador. Se llama desde SysTick, que es quien
 * escribe las envolventes, así que lee un estado completo sin bloquear la ISR (la ISR no las escribe) */
static void publishEnvelopes(void) {
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        envStage[v] = (uint8_t)voices[v].env.stage;
        envLevel[v] = voices[v].env.level;
    }
}

int16_t *audioOutPythonWaveRxBuffer(void) {
    return pythonWaveBuf[pythonWaveRx];
}

void audioOutPythonWaveActivate(void) {
    /* Sin audioIsrLock, a propósito. El puntero es una escritura de 32 bits, atómica en el Cortex-M3, y la ISR del DMA
     * (prioridad 0) no puede ser interrumpida por el loop principal: lee waveTables bloque a bloque, así que el cambio
     * siempre cae entre dos bloques. Además, audioIsrLock lo toma solo SysTick y no se anida: SysTick puede interrumpir
     * al loop principal y su audioIsrUnlock reactivaría la IRQ del DMA a mitad de esta función */
    waveTables[WAVE_PYTHON] = pythonWaveBuf[pythonWaveRx];
    __asm volatile ("" ::: "memory");     /* el puntero nuevo se escribe antes de que main marque la onda como cargada */
    pythonWaveRx ^= 1;                    /* el buffer que sonaba pasa a recibir la próxima onda */
}

void audioOutEqApply(const int32_t coefs[EQ_NUM_BANDS * 5]) {
    EqBandCoefs *set = eqCoefSets[eqNextSet];
    for (uint32_t band = 0; band < EQ_NUM_BANDS; band++) {
        set[band].b0 = coefs[5 * band];
        set[band].b1 = coefs[5 * band + 1];
        set[band].b2 = coefs[5 * band + 2];
        set[band].a1 = coefs[5 * band + 3];
        set[band].a2 = coefs[5 * band + 4];
    }
    __asm volatile ("" ::: "memory");     /* el juego se escribe completo antes de publicar el puntero */
    eqPendingCoefs = set;
    eqNextSet ^= 1;
}

const int16_t *audioOutCapture(void) {
    if (captureCount < FFT_SIZE) {
        return 0;
    }
    __asm volatile ("" ::: "memory");     /* las muestras se leen después de ver el contador lleno */
    return captureBuf;
}

void audioOutCaptureRelease(void) {
    __asm volatile ("" ::: "memory");     /* las lecturas del buffer terminan antes de reabrir la captura */
    captureCount = 0;
}

const Voice *audioOutVoices(void) {
    return voices;
}

void audioOutVoiceStart(uint8_t voice, uint8_t note, uint8_t wave) {
    if (voice >= NUM_VOICES || note >= NUM_NOTES || wave >= NUM_WAVES) {   /* fuera de rango, se leería fuera de voices, noteSteps o waveTables */
        return;
    }
    audioIsrLock();
    voiceStart(&voices[voice], noteSteps[note], wave);   /* voz libre: fase 0 y nivel 0; si suena, sigue desde su fase y nivel */
    audioIsrUnlock();
    publishEnvelopes();
}

void audioOutVoiceRelease(uint8_t voice) {
    if (voice >= NUM_VOICES) {
        return;
    }
    audioIsrLock();
    envNoteOff(&voices[voice].env);               /* la voz sigue sonando mientras se apaga: queda libre al terminar */
    audioIsrUnlock();
    publishEnvelopes();
}

void audioOutEnvelopeStep(const EnvParams *params) {
    audioIsrLock();
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        envStep(&voices[v].env, params);
    }
    audioIsrUnlock();
    publishEnvelopes();
}

Status audioOutInit(void) {
    for (uint32_t v = 0; v < NUM_VOICES; v++) {
        envInit(&voices[v].env);                  /* libres, sin depender de que ENV_IDLE valga 0 */
    }
    publishEnvelopes();
    blockCursorInit(&cursor);
    equalizerInit(&equalizer);                    /* plano (0 dB) antes de precargar los dos bloques */
    fillNextBlock();                           /* precargado: mitad 0 (la lee el DMA primero), en silencio */
    fillNextBlock();                              /* y mitad 1 */
    captureCount = 0;                             /* los dos bloques precargados son silencio: la captura empieza con la ISR */
    configMeasurePin();
    configDac();
    if (configDma() == ERROR) {
        return ERROR;
    }
    GPDMA_ChannelStart((GPDMA_CH)DAC_DMA_CHANNEL);
    return SUCCESS;
}
