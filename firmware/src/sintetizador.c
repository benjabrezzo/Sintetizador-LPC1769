#include <stdio.h>
#include "config.h"
#include "adc_input.h"
#include "audio_out.h"
#include "button_input.h"
#include "cycle_counter.h"
#include "debounce.h"
#include "envelope.h"
#include "led_bars.h"
#include "led_encoder.h"
#include "led_matrix.h"
#include "led_patterns.h"
#include "pot_filter.h"
#include "spectrum.h"
#include "uart_link.h"
#include "uart_protocol.h"
#include "voice.h"
#include "wave_select.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_systick.h"

/* Diagnóstico por semihosting: el printf se envía al depurador con un BKPT. Sin depurador
 * ese BKPT cuelga el arranque, así que solo se imprime si C_DEBUGEN está activo.
 * Acceso directo a CoreDebug->DHCSR: ningún driver de la lista lo cubre (spec 3.9). */
static int isDebuggerAttached(void) {
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

/* Milisegundos transcurridos desde el último cambio del LED. Solo la usa la ISR. */
static uint32_t msSinceToggle = 0;

/* Antirrebote y asignación de voces (lógica pura, probada en la PC). Solo los toca SysTick. */
static Debounce keyDebounce;
static VoiceAllocator voiceAlloc;

/* Parámetros de la envolvente (spec 3.4), globales para las cuatro voces. SysTick los recalcula desde los potes en
 * cada ms, antes de avanzar las envolventes: nunca se usan con los ceros del arranque. Solo los toca SysTick. */
static EnvParams envParams;

/* Un filtro por potenciómetro, en el orden de PotIndex. Arrancan en la primera lectura (main) y solo los toca
 * SysTick. */
static PotFilter potFilters[POT_COUNT];

/* Pulsaciones detectadas por cada tecla (la tecla k + 1 es el elemento k), acumuladas desde el arranque. Es
 * evidencia para la story 7: sin osciloscopio no se oye un re-disparo por rebote, pero si se aprieta 20 veces
 * una tecla y el contador marca 20, el antirrebote no generó pulsaciones de más. Global a propósito, para
 * leerla en la vista Expressions con la CPU suspendida. Solo la escribe SysTick. */
volatile uint32_t keyPressCount[DEBOUNCE_NUM_KEYS] = {0};

/* Selección de onda con EINT1 (lógica pura, probada en la PC). La ISR de EINT1 (prioridad 4) acepta las
 * pulsaciones y SysTick (prioridad 1) lee el pulsador para rearmarlo: SysTick puede interrumpir a EINT1, pero
 * no al revés. No necesita volatile: los dos la tocan solo en funciones de otro módulo (wave_select.c). */
static WaveSelect waveSelect;

/* Milisegundos desde el arranque, para la ventana de 150 ms del pulsador (spec 3.11). Los incrementa SysTick; da
 * la vuelta a los ~49 días y waveSelect lo tolera (lo prueban los tests de la PC). */
static volatile uint32_t msTicks = 0;

/* La onda activa: el identificador (wave_select.h) que toman las notas que arrancan ahora. Solo la escribe la
 * ISR de EINT1, cuando acepta una pulsación; SysTick la lee al arrancar cada nota. Global a propósito, para
 * leerla en la vista Expressions con la CPU suspendida. */
volatile uint8_t activeWave = WAVE_SINE;

/* Pulsaciones del pulsador de EINT1 que la lógica aceptó, acumuladas desde el arranque. Es evidencia para la
 * story 8: los flancos ignorados (rebote, pulsación mantenida, a menos de 150 ms) no cuentan, así que si se
 * aprieta 20 veces y marca 20, ningún rebote sumó de más. Se lee igual que activeWave. Solo la escribe EINT1. */
volatile uint32_t waveButtonCount = 0;

/* 1 desde que llegó la primera onda válida de Python: la rotación de EINT1 incluye la quinta posición (spec 3.8).
 * Mientras valga 0, la rotación la saltea. La escribe el loop principal, después de activar la onda (audio_out), y la
 * lee la ISR de EINT1, así que la quinta nunca se elige antes de tener una tabla. Global a propósito, para leerla en
 * la vista Expressions con la CPU suspendida. */
volatile uint8_t pythonWaveLoaded = 0;

/* Parser de las tramas que llegan por UART (lógica pura, probada en la PC). Solo lo toca el loop principal */
static UartParser uartParser;

/* Coeficientes de SET_EQ en recepción (15 int32 en Q2.30, orden de docs/protocolo.md). No son los que usa el audio: con
 * una trama válida, audioOutEqApply los copia a uno de sus dos juegos. Solo los toca el loop principal */
static int32_t eqRxBuf[UART_EQ_COEF_COUNT];

/* Lee las ocho teclas, las filtra y aplica los eventos a las voces (spec 3.2 y 3.11). Las liberaciones van
 * antes que las pulsaciones: si en el mismo milisegundo se suelta una tecla y se aprieta otra, la voz soltada ya
 * está en Release cuando la nueva elige voz, y puede tomarla si no hay una libre, en vez de reemplazar de balde a
 * una apretada. Una tecla que perdió su voz (la tomó otra) no tiene nada que soltar: voiceAllocRelease devuelve
 * VOICE_NONE. */
static void applyKeyEvents(void) {
    uint8_t pressed;
    uint8_t released;
    debounceUpdate(&keyDebounce, buttonInputRead(), &pressed, &released);

    for (uint8_t key = 0; key < DEBOUNCE_NUM_KEYS; key++) {
        if (released & (1u << key)) {
            const uint8_t voice = voiceAllocRelease(&voiceAlloc, key, audioOutVoices());
            if (voice != VOICE_NONE) {
                audioOutVoiceRelease(voice);
            }
        }
    }
    for (uint8_t key = 0; key < DEBOUNCE_NUM_KEYS; key++) {
        if (pressed & (1u << key)) {
            keyPressCount[key]++;
            const VoiceAssignment assignment = voiceAllocPress(&voiceAlloc, key, audioOutVoices());
            /* La onda se fija acá, al arrancar la nota: un cambio de activeWave no toca las voces que suenan.
             * Si la voz suena (la reutilizada, la que se apagaba o la más antigua), la nota sigue desde su fase y
             * su nivel */
            audioOutVoiceStart(assignment.voice, assignment.note, activeWave);
        }
    }
}

/* Lee los cuatro potes (la última conversión del burst), filtra cada lectura y la convierte en su parámetro de la
 * envolvente (spec 3.4) */
static void updateEnvParams(void) {
    uint16_t readings[POT_COUNT];
    adcInputRead(readings);
    for (uint32_t i = 0; i < POT_COUNT; i++) {
        readings[i] = potFilterUpdate(&potFilters[i], readings[i]);
    }
    envParams.attackMs = envMapTimeMs(readings[POT_ATTACK]);
    envParams.decayMs = envMapTimeMs(readings[POT_DECAY]);
    envParams.sustainLevel = envMapSustainLevel(readings[POT_SUSTAIN]);
    envParams.releaseMs = envMapTimeMs(readings[POT_RELEASE]);
}

/* SysTick cada 1 ms: cuenta el tiempo, alterna el LED cada LED_HALF_PERIOD_MS, lee el pulsador de onda
 * (para rearmarlo al soltar), actualiza los parámetros desde los potes, avanza las envolventes y atiende las
 * teclas. Los parámetros van antes que las envolventes: cada paso usa la posición actual de los potes. Las
 * envolventes van antes que las teclas: el asignador ve el estado al día (un Release que acaba de terminar ya es
 * una voz libre) y una nota apretada en este ms empieza a contar en el siguiente. */
void SysTick_Handler(void) {
    msTicks++;
    msSinceToggle++;
    if (msSinceToggle >= LED_HALF_PERIOD_MS) {
        msSinceToggle = 0;
        GPIO_TogglePins(LED_PORT, 1u << LED_PIN);
    }
    waveSelectSample(&waveSelect, buttonInputWaveRead());
    updateEnvParams();
    audioOutEnvelopeStep(&envParams);
    applyKeyEvents();
}

/* Flanco descendente de P2.11 (spec 3.11). waveSelect decide si cuenta como pulsación: pasaron 150 ms desde la
 * última aceptada y el pulsador se soltó desde entonces. El flag se limpia al entrar, antes de decidir: un
 * flanco que llegue durante la ISR queda pendiente y se atiende después, en vez de borrarse sin haberlo visto.
 * Se limpia también cuando el flanco se ignora, o la interrupción volvería a dispararse. */
void EINT1_IRQHandler(void) {
    EXTI_ClearFlag(EXTI_EINT1);
    if (waveSelectPress(&waveSelect, msTicks, pythonWaveLoaded)) {
        waveButtonCount++;
        activeWave = waveSelectCurrent(&waveSelect);
    }
}

/* Análisis espectral (spec 3.6; story 15). Lo corre solo el loop principal, después del parser. Todas estas variables son
 * globales a propósito, para leerlas en la vista Expressions con la CPU suspendida (el audio sigue por la ISR del DMA):
 *  - spectrumPeak: bin de mayor magnitud del último cuadro (1 a 255; 0 si no hay pico). Cada bin mide Fs / 512 = 62,5 Hz.
 *  - spectrumBand[b]: magnitud de la banda b + 1 (Q15; 1,0 = 32768 a fondo de escala) y spectrumLevel[b], su altura de 0 a 8.
 *  - spectrumFrames: cuadros analizados desde el arranque; spectrumFps, cuadros por segundo promedio desde el arranque
 *    (spectrumFrames · 1000 / msTicks).
 *  - spectrumLastCycles y spectrumMaxCycles: ciclos de CPU del último análisis y su máximo, medidos con cycleCounterNow desde
 *    que se toma la captura hasta que se publica el resultado. Incluyen el tiempo que le quitan la ISR del DMA y SysTick
 *    (es la latencia real, no el costo del análisis solo). Vale mientras el análisis dure menos de 43 s. */
volatile uint32_t spectrumPeak = 0;
volatile uint16_t spectrumBand[SPECTRUM_NUM_BANDS] = {0};
volatile uint8_t spectrumLevel[SPECTRUM_NUM_BANDS] = {0};
volatile uint32_t spectrumFrames = 0;
volatile uint32_t spectrumFps = 0;
volatile uint32_t spectrumLastCycles = 0;
volatile uint32_t spectrumMaxCycles = 0;

/* Magnitudes del último cuadro y niveles con el suavizado (spectrumLevels los recibe y los devuelve). Solo los toca el loop
 * principal. Las variables de arriba son copias para el depurador, como envStage y envLevel */
static uint16_t spectrumMagnitudeBuf[SPECTRUM_BINS];
static uint8_t spectrumLevelState[SPECTRUM_NUM_BANDS];

/* Si la captura de la ISR ya está llena, la analiza: ventana, FFT y magnitudes, bandas, niveles y bin del pico. La captura
 * se libera apenas spectrumMagnitudes copió las muestras a su buffer de trabajo, así la ISR ya puede llenar el cuadro
 * siguiente mientras se calculan las bandas. Sin captura lista no hace nada. */
static void analyzeSpectrum(void) {
    const int16_t *capture = audioOutCapture();
    if (capture == 0) {
        return;
    }
    const uint32_t startCycles = cycleCounterNow();

    uint16_t bands[SPECTRUM_NUM_BANDS];
    spectrumMagnitudes(capture, spectrumMagnitudeBuf);
    audioOutCaptureRelease();
    spectrumBands(spectrumMagnitudeBuf, bands);
    spectrumLevels(bands, spectrumLevelState);

    spectrumPeak = spectrumPeakBin(spectrumMagnitudeBuf);
    for (uint32_t b = 0; b < SPECTRUM_NUM_BANDS; b++) {
        spectrumBand[b] = bands[b];
        spectrumLevel[b] = spectrumLevelState[b];
    }

    const uint32_t elapsed = cycleCounterNow() - startCycles;
    spectrumLastCycles = elapsed;
    if (elapsed > spectrumMaxCycles) {
        spectrumMaxCycles = elapsed;
    }
    spectrumFrames++;
    const uint32_t ms = msTicks;
    if (ms != 0) {
        spectrumFps = spectrumFrames * 1000u / ms;
    }
}

/* Matriz de 8×8 (spec 3.6; story 16). Las escribe y lee solo el loop principal, salvo matrixPattern, que se cambia desde la
 * vista Expressions con la CPU suspendida. Todas son globales a propósito, para verlas ahí:
 *  - matrixPattern: patrón de prueba o barras del espectro (MATRIX_PATTERN_* de config.h; story 17: poner 7). Arranca en MATRIX_PATTERN_OFF (0), que no transmite: con
 *    la matriz sin alimentar, DIN solo tolera 0,5 V (spec 3.15). Se enciende la matriz y después se elige el patrón. Un
 *    valor fuera de la lista tampoco transmite.
 *  - matrixStep: paso actual de los patrones que avanzan (píxel, fila, columna y colores); sube 1 cada MATRIX_STEP_MS.
 *  - matrixFrames: cuadros que ledMatrixSend aceptó desde el arranque.
 *  - matrixSendMaxCycles: máximo de ciclos de CPU de una llamada a ledMatrixSend que inició un envío, o sea casi toda la
 *    codificación de la superficie. Incluye el tiempo que le quitan la ISR del DMA y SysTick (es la latencia real). Vale
 *    mientras dure menos de 43 s. */
volatile uint8_t matrixPattern = MATRIX_PATTERN_OFF;
volatile uint32_t matrixStep = 0;
volatile uint32_t matrixFrames = 0;
volatile uint32_t matrixSendMaxCycles = 0;

/* Superficie que se dibuja y se envía en cada cuadro, y los instantes (en ms de SysTick) del último cuadro aceptado y del
 * último avance del paso. Solo los toca el loop principal */
static LedSurface matrixSurface;
static uint32_t matrixLastFrameMs = 0;
static uint32_t matrixLastStepMs = 0;

/* Altura de cada barra tal como se muestra en pantalla (patrón del espectro): el nivel del análisis con la etapa de caída
 * de ledBarsFall (spec 3.6, Bloque 13). Solo se actualiza cuando ledMatrixSend acepta el cuadro, así que baja BARS_FALL
 * niveles por cuadro de pantalla aunque haya que reintentar el envío varias vueltas del loop */
static uint8_t barsShown[LED_BARS_COUNT] = {0};

/* Cada MATRIX_FRAME_MS dibuja el patrón elegido y lo manda a la matriz. Sin esperas: si ledMatrixSend no acepta el envío (hay
 * uno en curso o todavía no pasó el bajo de reinicio), el cuadro sigue pendiente y se reintenta en la próxima vuelta. El
 * Timer2 y el DMA transmiten solos mientras el loop sigue con el análisis y el parser */
static void updateMatrix(void) {
    /* Sin patrón válido no se transmite. ledMatrixBusy se consulta igual: al terminar el último envío apaga el Timer2 */
    const uint8_t pattern = matrixPattern;
    if (pattern == MATRIX_PATTERN_OFF || pattern > MATRIX_PATTERN_SPECTRUM) {
        (void)ledMatrixBusy();
        return;
    }

    const uint32_t now = msTicks;
    if ((uint32_t)(now - matrixLastFrameMs) < MATRIX_FRAME_MS) {
        return;
    }
    if ((uint32_t)(now - matrixLastStepMs) >= MATRIX_STEP_MS) {
        matrixLastStepMs = now;
        matrixStep++;
    }

    ledSurfaceInit(&matrixSurface);
    uint8_t barsNext[LED_BARS_COUNT] = {0};     /* alturas de este cuadro: se guardan en barsShown solo si se acepta el envío */
    switch (pattern) {
        case MATRIX_PATTERN_PIXEL:
            ledPatternPixel(&matrixSurface, matrixStep);
            break;
        case MATRIX_PATTERN_ROW:
            ledPatternRow(&matrixSurface, matrixStep);
            break;
        case MATRIX_PATTERN_COLUMN:
            ledPatternColumn(&matrixSurface, matrixStep);
            break;
        case MATRIX_PATTERN_COLORS:
            ledPatternColor(&matrixSurface, matrixStep);
            break;
        case MATRIX_PATTERN_WHITE:
            ledPatternWhite(&matrixSurface);
            break;
        case MATRIX_PATTERN_SPECTRUM:
            uint8_t levels[LED_BARS_COUNT];
            for (uint32_t b = 0; b < LED_BARS_COUNT; b++) {
                levels[b] = spectrumLevel[b];       /* los escribe solo analyzeSpectrum, en este mismo loop */
                barsNext[b] = barsShown[b];
            }
            ledBarsFall(barsNext, levels);
            /* Color de la onda seleccionada, la de las próximas notas (story 18): activeWave se lee una sola vez por cuadro, así que
             * todas las barras cambian juntas aunque EINT1 la escriba en medio del dibujo */
            ledBarsDraw(&matrixSurface, barsNext, ledWaveColor(activeWave));
            break;
        case MATRIX_PATTERN_BLACK:
        default:
            break;                      /* superficie apagada */
    }

    const uint32_t startCycles = cycleCounterNow();
    if (ledMatrixSend(&matrixSurface)) {
        const uint32_t elapsed = cycleCounterNow() - startCycles;
        if (elapsed > matrixSendMaxCycles) {
            matrixSendMaxCycles = elapsed;
        }
        matrixFrames++;
        matrixLastFrameMs = now;
        if (pattern == MATRIX_PATTERN_SPECTRUM) {
            for (uint32_t b = 0; b < LED_BARS_COUNT; b++) {
                barsShown[b] = barsNext[b];
            }
        }
    }
}

int main(void) {
    if (isDebuggerAttached()) {
        printf("Sintetizador LPC1769: reloj de %lu Hz\n", (unsigned long)SystemCoreClock);
    }

    GPIO_SetDir(LED_PORT, 1u << LED_PIN, GPIO_OUTPUT);

    cycleCounterInit();             /* antes del audio: la ISR del DMA mide sus ciclos desde el primer bloque */
    if (audioOutInit() == ERROR && isDebuggerAttached()) {
        printf("Error: no se pudo configurar el canal DMA del DAC\n");
    }
    ledMatrixInit();                /* después de audioOutInit (no repite GPDMA_Init, que resetearía el canal 0) y de cycleCounterInit */

    /* Antes de habilitar SysTick: arranca sin teclas apretadas, con las cuatro voces libres (en silencio), en
     * senoide y con los filtros de los potes en la posición real de cada uno. EINT1 se habilita al final, con la
     * selección de onda ya lista para atender un flanco */
    buttonInputInit();
    debounceInit(&keyDebounce);
    voiceAllocInit(&voiceAlloc);
    adcInputInit();
    uint16_t firstReadings[POT_COUNT];
    adcInputRead(firstReadings);
    for (uint32_t i = 0; i < POT_COUNT; i++) {
        potFilterInit(&potFilters[i], firstReadings[i]);
    }
    waveSelectInit(&waveSelect);
    activeWave = waveSelectCurrent(&waveSelect);
    buttonInputWaveInit();
    uartParserInit(&uartParser);
    uartLinkInit();

    SYSTICK_InternalInit(1);        /* 1 ms */
    /* El driver no fija la prioridad y quedaría en 0, igual que el DMA (spec 3.12 pide 1) */
    NVIC_SetPriority(SysTick_IRQn, SYSTICK_IRQ_PRIORITY);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    /* Loop principal (spec 3.12 y 5): primero el parser, después el análisis espectral y al final la matriz (updateMatrix:
     * dibuja el patrón de prueba y lo manda con Timer2 + DMA, sin esperar). Saca los bytes que la ISR de la UART dejó en el buffer circular y se los pasa al parser,
     * que escribe la onda o los coeficientes del ecualizador en su buffer de recepción. Con una onda válida la activa, la
     * deja disponible para EINT1 y responde ACK; con unos coeficientes válidos los encola para el próximo bloque de audio
     * y responde ACK. Una trama inválida no hace nada. __WFI duerme hasta la próxima interrupción (SysTick,
     * como mucho en 1 ms), así que un byte que llegue justo antes de dormir se atiende enseguida */
    while (1) {
        uint8_t byte;
        while (uartLinkReadByte(&byte)) {
            const uint8_t parsed = uartParserFeed(&uartParser, byte, audioOutPythonWaveRxBuffer(), eqRxBuf);
            if (parsed == UART_PARSER_WAVE_READY) {
                audioOutPythonWaveActivate();
                pythonWaveLoaded = 1;
                uartLinkSend(uartAckFrame, UART_ACK_FRAME_BYTES);
            } else if (parsed == UART_PARSER_EQ_READY) {
                audioOutEqApply(eqRxBuf);
                uartLinkSend(uartAckFrame, UART_ACK_FRAME_BYTES);
            }
        }
        analyzeSpectrum();
        updateMatrix();
        __WFI();
    }
    return 0;
}
