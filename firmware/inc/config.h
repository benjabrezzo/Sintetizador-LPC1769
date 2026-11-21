/* Configuración del sintetizador: pines, tiempos y prioridades (constitución, principio 9).
 * Solo constantes: lo puede incluir cualquier módulo, también los de lógica. */
#ifndef CONFIG_H
#define CONFIG_H

/* LED de la placa (prueba de vida del Bloque 0, spec 3.13) */
#define LED_PORT            0        /* P0.22 */
#define LED_PIN             22
#define LED_HALF_PERIOD_MS  500      /* alterna cada 500 ms: período de 1 s */

/* Audio (spec 3.1). El contador del DAC corre con PCLK = CCLK/4 = 25 MHz: Fs = 25 MHz / 781 = 32,01 kHz */
#define DAC_COUNTER_VALUE   781      /* valor de DACCNTVAL */
#define DAC_DMA_CHANNEL     0        /* canal del GPDMA que alimenta el DAC (M2P) */
#define AUDIO_BLOCK_SIZE    64       /* muestras por bloque: 64 / 32,01 kHz = 2 ms */

/* Voces (spec 3.2) */
#define NUM_VOICES          4

/* Potenciómetros de la ADSR (spec 3.4): lecturas de 12 bits del ADC. Las primeras y las últimas POT_END_MARGIN
 * cuentas dan el extremo, para que el giro real del pote llegue a los dos extremos de cada parámetro */
#define ADC_MAX_READING      4095
#define POT_END_MARGIN       50

/* Filtro de los potenciómetros (Bloque 7): EMA de primer orden con α = 1 / 2^POT_FILTER_SHIFT. Con una lectura por
 * ms, α = 1/16 da τ ≈ 16 ms: un giro de golpe de punta a punta se sigue en unos 130 ms */
#define POT_FILTER_SHIFT     4

/* Canales del ADC de los potenciómetros (spec 3.13), convertidos en modo burst. AD0.3 no: es P0.26, la salida del DAC */
#define POT_ATTACK_CHANNEL   0        /* AD0.0, P0.23 */
#define POT_DECAY_CHANNEL    1        /* AD0.1, P0.24 */
#define POT_SUSTAIN_CHANNEL  2        /* AD0.2, P0.25 */
#define POT_RELEASE_CHANNEL  4        /* AD0.4, P1.30 */

/* Frecuencia de conversión del ADC (el máximo del LPC1769). Con PCLK = CCLK/4 = 25 MHz, ADC_Init deja el reloj del
 * ADC en 12,5 MHz (máximo 13 MHz): 65 ciclos por conversión, unos 21 µs para recorrer los cuatro canales */
#define ADC_RATE_HZ          200000

/* Teclas (spec 3.11): una tecla cambia de estado cuando lleva DEBOUNCE_MS ms estable (lecturas de SysTick cada 1 ms) */
#define DEBOUNCE_MS         10

/* Teclas (spec 3.11 y 3.13): ocho pulsadores en P2.0 a P2.7, activos en bajo con el pull-up interno.
 * La tecla 1 (Do4) está en P2.0 y la 8 (Do5), en P2.7. */
#define KEYS_PORT           2
#define KEYS_FIRST_PIN      0        /* P2.0; las ocho teclas ocupan pines consecutivos */

/* Selección de onda (spec 3.11 y 3.13): el pulsador de EINT1 está en P2.11. Una pulsación solo se acepta si pasaron
 * WAVE_LOCKOUT_MS desde la última aceptada y, además, el pulsador se soltó (DEBOUNCE_MS ms seguidos en alto) */
#define WAVE_LOCKOUT_MS     150
#define WAVE_BUTTON_PORT    2        /* P2.11 = EINT1, con pull-up interno; flanco descendente */
#define WAVE_BUTTON_PIN     11

/* UART0 (spec 3.14): el buffer circular de recepción tiene que ser potencia de 2, para que los contadores libres de
 * ring_buffer den la vuelta junto con los índices */
#define UART_RX_BUFFER_SIZE 256
#define UART_BAUD_RATE      115200   /* 8N1, sin control de flujo */

/* Pin de medición del Bloque 2 (spec 3.13): sube mientras la ISR del DMA genera un bloque */
#define MEASURE_PORT        2        /* P2.8 */
#define MEASURE_PIN         8

/* Matriz WS2812B (spec 3.15 y Bloque 12): DIN en P0.0 por 330 Ω. El Timer2 pide una transferencia de DMA cada 400 ns
 * (MAT2.0: PCLK de 25 MHz con PR = 0 y MR0 = 9 da 25 MHz / 10 = 2,5 MHz) y el canal 1 copia los bytes del buffer a
 * FIOPIN0, con FIOMASK0 = 0xFE para que solo cambie P0.0 */
#define MATRIX_PORT          0        /* P0.0 */
#define MATRIX_PIN           0
#define MATRIX_DMA_CHANNEL   1        /* GPDMA canal 1 (el 0 es el del audio) */
#define MATRIX_TIMER_MATCH   9        /* valor de MR0 del Timer2 */
#define MATRIX_RESET_CYCLES  10000    /* ciclos de CPU en bajo entre cuadros: 100 µs con CCLK = 100 MHz (el reinicio pide más de 50 µs) */

/* Patrón de prueba de la matriz (story 16): el loop principal lo dibuja según matrixPattern, que se escribe desde la vista
 * Expressions. Se reenvía cada MATRIX_FRAME_MS (25 FPS) y los patrones con paso avanzan cada MATRIX_STEP_MS */
#define MATRIX_PATTERN_OFF      0     /* no transmite: P0.0 queda en bajo. Es el arranque, por si la matriz está sin alimentar */
#define MATRIX_PATTERN_PIXEL    1     /* un píxel rojo recorre la matriz en orden de filas */
#define MATRIX_PATTERN_ROW      2     /* una fila roja recorre la matriz */
#define MATRIX_PATTERN_COLUMN   3     /* una columna roja recorre la matriz */
#define MATRIX_PATTERN_COLORS   4     /* toda la matriz en rojo, azul, verde, amarillo y morado, uno por paso */
#define MATRIX_PATTERN_WHITE    5     /* los 64 píxeles en blanco: la prueba de consumo */
#define MATRIX_PATTERN_BLACK    6     /* transmite los 64 píxeles apagados */
#define MATRIX_PATTERN_SPECTRUM 7     /* barras del espectro (story 17): una columna por banda, del color de la onda seleccionada (story 18) */
#define MATRIX_FRAME_MS         40
#define MATRIX_STEP_MS          500

/* Caída de las barras del espectro en pantalla (spec 3.6, Bloque 13): niveles que baja una barra por cuadro de pantalla
 * (40 ms). Con 1, de 8 a 0 tarda 320 ms. Se ajusta en el laboratorio; la subida es siempre inmediata */
#define BARS_FALL               1

/* Prioridades de interrupción (spec 3.12): un número menor es más prioritario */
#define DMA_IRQ_PRIORITY      0      /* audio: si se atrasa, hay un corte audible */
#define SYSTICK_IRQ_PRIORITY  1      /* cada 1 ms: potes, ADSR y teclas */
#define UART_IRQ_PRIORITY     2      /* recepción de la app: copia bytes al buffer circular, sin procesarlos */
#define EINT1_IRQ_PRIORITY    4      /* selección de onda: evento humano, sin urgencia */

#endif /* CONFIG_H */
