/* Tests del parser de tramas UART (spec 3.14, story 13). Trama UPLOAD_WAVE: 0xA5 | 0x01 | LEN_L | LEN_H | 512 bytes
 * de payload | CHK, con CHK = complemento a dos de la suma de 8 bits de CMD, LEN_L, LEN_H y payload (docs/protocolo.md).
 * El parser recibe un byte por vez: la trama (517 B) no entra en el buffer circular de 256 B. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uart_protocol.h"
#include "tables.h"          /* WAVE_SIZE */
#include "test_check.h"

#define FRAME_BYTES  (4 + 2 * WAVE_SIZE + 1)    /* cabecera + payload + CHK = 517 */
#define CHK_INDEX    (FRAME_BYTES - 1)

#define EQ_FRAME_BYTES  (4 + UART_PAYLOAD_EQ_BYTES + 1)    /* cabecera + payload + CHK = 65 */
#define EQ_CHK_INDEX    (EQ_FRAME_BYTES - 1)

/* El parser recibe los dos buffers siempre. Los tests de una trama no miran el buffer de la otra: le pasan uno de descarte */
static int16_t waveScratch[WAVE_SIZE];
static int32_t eqScratch[UART_EQ_COEF_COUNT];

/* Arma una trama UPLOAD_WAVE con las muestras dadas, con el formato de docs/protocolo.md. El checksum se calcula
 * acá con la fórmula del documento, no con el código del firmware */
static void buildWaveFrame(const int16_t *samples, uint8_t *frame) {
    frame[0] = 0xA5;
    frame[1] = 0x01;
    frame[2] = 0x00;
    frame[3] = 0x02;       /* LEN = 512 */
    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        frame[4 + 2 * i] = (uint8_t)((uint16_t)samples[i] & 0xFF);
        frame[5 + 2 * i] = (uint8_t)((uint16_t)samples[i] >> 8);
    }
    uint32_t sum = 0;
    for (uint32_t i = 1; i < CHK_INDEX; i++) {
        sum += frame[i];
    }
    frame[CHK_INDEX] = (uint8_t)((0x100 - (sum & 0xFF)) & 0xFF);
}

/* Arma una trama SET_EQ con los coeficientes dados (15 int32_t, el byte bajo primero), con el formato de
 * docs/protocolo.md: 0xA5 | 0x04 | LEN = 60 | payload | CHK. El checksum se calcula con la fórmula del documento */
static void buildEqFrame(const int32_t *coefs, uint8_t *frame) {
    frame[0] = 0xA5;
    frame[1] = 0x04;
    frame[2] = 0x3C;
    frame[3] = 0x00;       /* LEN = 60 */
    for (uint32_t i = 0; i < UART_EQ_COEF_COUNT; i++) {
        const uint32_t word = (uint32_t)coefs[i];
        for (uint32_t b = 0; b < 4; b++) {
            frame[4 + 4 * i + b] = (uint8_t)((word >> (8 * b)) & 0xFF);
        }
    }
    uint32_t sum = 0;
    for (uint32_t i = 1; i < EQ_CHK_INDEX; i++) {
        sum += frame[i];
    }
    frame[EQ_CHK_INDEX] = (uint8_t)((0x100 - (sum & 0xFF)) & 0xFF);
}

/* Entrega bytes al parser de a uno y cuenta cuántas veces avisó que los coeficientes están listos */
static uint32_t feedEqBytes(UartParser *p, const uint8_t *bytes, uint32_t count, int32_t *eqBuf) {
    uint32_t ready = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (uartParserFeed(p, bytes[i], waveScratch, eqBuf) == UART_PARSER_EQ_READY) {
            ready++;
        }
    }
    return ready;
}

/* Entrega bytes al parser de a uno y cuenta cuántas veces avisó que la tabla está lista */
static uint32_t feedBytes(UartParser *p, const uint8_t *bytes, uint32_t count, int16_t *waveBuf) {
    uint32_t ready = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (uartParserFeed(p, bytes[i], waveBuf, eqScratch) == UART_PARSER_WAVE_READY) {
            ready++;
        }
    }
    return ready;
}

/* Una trama válida avisa una sola vez, al último byte, y el buffer de recepción queda igual a la tabla enviada.
 * Primero la trama del ejemplo de docs/protocolo.md (muestra 0 = 0x1234, el resto en cero, CHK = 0xB7 calculado a
 * mano), que ancla el armado de la trama; después una tabla con valores negativos y de todos los bytes */
static void testTramaValidaAvisaYElBufferQuedaIgualALaTablaEnviada(void) {
    int16_t samples[WAVE_SIZE] = {0};
    uint8_t frame[FRAME_BYTES];
    int16_t received[WAVE_SIZE];
    UartParser p;
    uartParserInit(&p);

    samples[0] = 0x1234;
    buildWaveFrame(samples, frame);
    CHECK(frame[CHK_INDEX] == 0xB7);
    memset(received, 0x55, sizeof received);
    CHECK(feedBytes(&p, frame, FRAME_BYTES - 1, received) == 0);     /* sin el CHK todavía no avisa */
    CHECK(feedBytes(&p, frame + CHK_INDEX, 1, received) == 1);
    CHECK(memcmp(received, samples, sizeof samples) == 0);

    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        samples[i] = (int16_t)((int32_t)i * 251 - 32000);
    }
    buildWaveFrame(samples, frame);
    memset(received, 0x55, sizeof received);
    CHECK(feedBytes(&p, frame, FRAME_BYTES - 1, received) == 0);
    CHECK(feedBytes(&p, frame + CHK_INDEX, 1, received) == 1);
    CHECK(memcmp(received, samples, sizeof samples) == 0);
}

/* Los bytes 0xA5 del payload son datos, no inicio de trama: una tabla donde todos los bytes valen 0xA5 (muestras
 * 0xA5A5) llega completa, con un solo aviso */
static void testTramaConA5DentroDelPayloadNoSeReinicia(void) {
    int16_t samples[WAVE_SIZE];
    uint8_t frame[FRAME_BYTES];
    int16_t received[WAVE_SIZE] = {0};
    UartParser p;
    uartParserInit(&p);

    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        samples[i] = (int16_t)0xA5A5;
    }
    buildWaveFrame(samples, frame);
    CHECK(feedBytes(&p, frame, FRAME_BYTES, received) == 1);
    CHECK(memcmp(received, samples, sizeof samples) == 0);
}

/* Los bytes llegan en tandas de cualquier tamaño (la ISR los junta y el loop los entrega cuando puede): el parser
 * conserva el estado entre llamados, no avisa a mitad de camino y avisa una sola vez, al último byte */
static void testTramaPartidaEnVariosLlamadosAvisaSoloAlFinal(void) {
    int16_t samples[WAVE_SIZE];
    uint8_t frame[FRAME_BYTES];
    int16_t received[WAVE_SIZE] = {0};
    UartParser p;
    uartParserInit(&p);

    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        samples[i] = (int16_t)((int32_t)i * 251 - 32000);
    }
    buildWaveFrame(samples, frame);

    CHECK(feedBytes(&p, frame, 2, received) == 0);                      /* 0xA5 y CMD */
    CHECK(feedBytes(&p, frame + 2, 99, received) == 0);                 /* LEN y el principio del payload */
    CHECK(feedBytes(&p, frame + 101, 300, received) == 0);
    CHECK(feedBytes(&p, frame + 401, FRAME_BYTES - 401 - 1, received) == 0);
    CHECK(feedBytes(&p, frame + CHK_INDEX, 1, received) == 1);
    CHECK(memcmp(received, samples, sizeof samples) == 0);
}

/* Un bit cambiado en el CHK, o en un byte del payload con el CHK intacto, hace que la trama no se acepte: no avisa
 * y la tabla activa no se toca (el aviso es lo que dispara el cambio de puntero) */
static void testChecksumMaloNoAvisa(void) {
    int16_t samples[WAVE_SIZE];
    uint8_t frame[FRAME_BYTES];
    int16_t received[WAVE_SIZE] = {0};
    UartParser p;
    uartParserInit(&p);

    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        samples[i] = (int16_t)((int32_t)i * 251 - 32000);
    }

    buildWaveFrame(samples, frame);
    frame[CHK_INDEX] ^= 0x01;                                   /* CHK equivocado */
    CHECK(feedBytes(&p, frame, FRAME_BYTES, received) == 0);

    buildWaveFrame(samples, frame);
    frame[4 + 100] ^= 0x80;                                     /* un bit del payload, CHK intacto */
    CHECK(feedBytes(&p, frame, FRAME_BYTES, received) == 0);

    buildWaveFrame(samples, frame);
    frame[1] ^= 0x01;                                           /* un bit de CMD, CHK intacto */
    CHECK(feedBytes(&p, frame, FRAME_BYTES, received) == 0);
}

/* Cabecera de UPLOAD_WAVE con el LEN dado, seguida de 600 bytes de relleno (0x11) y un CHK: el parser no tiene que
 * escribir ni uno en el buffer de recepción ni avisar. El relleno no usa 0xA5, que sería un inicio de trama */
static uint32_t feedFrameWithLen(UartParser *p, uint16_t len, int16_t *waveBuf) {
    const uint8_t header[4] = {0xA5, 0x01, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)};
    uint8_t filler[601];
    memset(filler, 0x11, sizeof filler);
    filler[600] = 0x00;
    return feedBytes(p, header, sizeof header, waveBuf) + feedBytes(p, filler, sizeof filler, waveBuf);
}

/* Una trama con LEN distinto de 512 se descarta apenas se lee el largo, sin tocar el buffer: con un LEN grande, el
 * parser escribiría fuera de él. Se prueban LEN = 0, 511, 513 y 0xFFFF */
static void testLenDistintoDe512SeDescartaSinEscribirElBuffer(void) {
    const uint16_t lens[] = {0, 511, 513, 0xFFFF};
    for (uint32_t k = 0; k < sizeof lens / sizeof lens[0]; k++) {
        int16_t received[WAVE_SIZE];
        int16_t untouched[WAVE_SIZE];
        UartParser p;
        uartParserInit(&p);
        memset(received, 0x55, sizeof received);
        memset(untouched, 0x55, sizeof untouched);

        CHECK(feedFrameWithLen(&p, lens[k], received) == 0);
        CHECK(memcmp(received, untouched, sizeof received) == 0);
    }
}

/* Una trama descartada no deja al parser trabado: ruido suelto, una trama con el checksum malo y otra con LEN
 * erróneo, una detrás de otra, y después una trama válida. Esa llega completa, con un solo aviso */
static void testDespuesDeUnaTramaInvalidaAceptaLaSiguienteValida(void) {
    int16_t badSamples[WAVE_SIZE];
    int16_t goodSamples[WAVE_SIZE];
    uint8_t badFrame[FRAME_BYTES];
    uint8_t goodFrame[FRAME_BYTES];
    int16_t received[WAVE_SIZE] = {0};
    UartParser p;
    uartParserInit(&p);

    for (uint32_t i = 0; i < WAVE_SIZE; i++) {
        badSamples[i] = (int16_t)i;
        goodSamples[i] = (int16_t)(1000 - 7 * (int32_t)i);
    }
    buildWaveFrame(badSamples, badFrame);
    badFrame[CHK_INDEX] ^= 0xFF;
    buildWaveFrame(goodSamples, goodFrame);

    const uint8_t noise[] = {0x00, 0x11, 0xFF, 0x7E};
    CHECK(feedBytes(&p, noise, sizeof noise, received) == 0);
    CHECK(feedBytes(&p, badFrame, FRAME_BYTES, received) == 0);
    CHECK(feedFrameWithLen(&p, 513, received) == 0);
    CHECK(feedBytes(&p, goodFrame, FRAME_BYTES, received) == 1);
    CHECK(memcmp(received, goodSamples, sizeof goodSamples) == 0);
}

/* Una trama SET_EQ válida avisa una sola vez, al último byte, y el buffer de coeficientes queda igual a lo enviado (15
 * int32 en Q2.30, el byte bajo primero). Primero el ejemplo de docs/protocolo.md con b0 del low-shelf = 0x12345678 y el
 * resto en cero (CHK = 0xAC, calculado a mano), después la identidad de las tres bandas (CHK = 0x00) y por último
 * valores negativos, extremos de int32 y bytes iguales al inicio de trama (0xA5). La trama no toca el buffer de la onda */
static void testSetEqValidaAvisaYElBufferQuedaIgualALosCoeficientesEnviados(void) {
    int32_t example[UART_EQ_COEF_COUNT] = {0};
    int32_t identity[UART_EQ_COEF_COUNT] = {0};
    const int32_t mixed[UART_EQ_COEF_COUNT] = {
        INT32_MIN, -1, INT32_MAX, -2, 0x0123ABCD, 1, -1073741824, 0x7F00FF00, -65536, 255, 256, -256, 0xA5,
        -0x5A5A5A5B, 2000000000,
    };
    example[0] = 0x12345678;
    for (uint32_t band = 0; band < 3; band++) {
        identity[5 * band] = 1 << 30;      /* b0 = 1,0 en Q2.30 */
    }
    const int32_t *sets[3] = {example, identity, mixed};

    for (uint32_t s = 0; s < 3; s++) {
        uint8_t frame[EQ_FRAME_BYTES];
        int32_t received[UART_EQ_COEF_COUNT];
        int16_t untouchedWave[WAVE_SIZE];
        UartParser p;
        uartParserInit(&p);

        buildEqFrame(sets[s], frame);
        if (s == 0) {
            CHECK(frame[EQ_CHK_INDEX] == 0xAC);
        }
        if (s == 1) {
            CHECK(frame[EQ_CHK_INDEX] == 0x00);
        }
        memset(received, 0x55, sizeof received);
        memset(waveScratch, 0x55, sizeof waveScratch);
        memset(untouchedWave, 0x55, sizeof untouchedWave);
        CHECK(feedEqBytes(&p, frame, EQ_FRAME_BYTES - 1, received) == 0);     /* sin el CHK todavía no avisa */
        CHECK(feedEqBytes(&p, frame + EQ_CHK_INDEX, 1, received) == 1);
        CHECK(memcmp(received, sets[s], sizeof received) == 0);
        CHECK(memcmp(waveScratch, untouchedWave, sizeof untouchedWave) == 0);
    }
}

/* Una trama SET_EQ con el checksum incorrecto no avisa: ni con el CHK equivocado ni con un bit cambiado en el payload
 * (en el primer byte, en uno del medio y en el último, con bits distintos), con el CHK intacto. Un bit de CMD no se
 * prueba acá: otro CMD se descarta antes del checksum, porque el LEN de SET_EQ no le corresponde (tests de CMD y LEN) */
static void testSetEqConChecksumMaloNoAvisa(void) {
    int32_t coefs[UART_EQ_COEF_COUNT];
    uint8_t frame[EQ_FRAME_BYTES];
    int32_t received[UART_EQ_COEF_COUNT] = {0};
    UartParser p;
    uartParserInit(&p);

    for (uint32_t i = 0; i < UART_EQ_COEF_COUNT; i++) {
        coefs[i] = (int32_t)((int64_t)i * 123456789 - 800000000);
    }

    buildEqFrame(coefs, frame);
    frame[EQ_CHK_INDEX] ^= 0x01;                                /* CHK equivocado */
    CHECK(feedEqBytes(&p, frame, EQ_FRAME_BYTES, received) == 0);

    buildEqFrame(coefs, frame);
    frame[4] ^= 0x01;                                           /* un bit del primer byte del payload, CHK intacto */
    CHECK(feedEqBytes(&p, frame, EQ_FRAME_BYTES, received) == 0);

    buildEqFrame(coefs, frame);
    frame[4 + 30] ^= 0x10;                                      /* un bit de un byte del medio */
    CHECK(feedEqBytes(&p, frame, EQ_FRAME_BYTES, received) == 0);

    buildEqFrame(coefs, frame);
    frame[4 + UART_PAYLOAD_EQ_BYTES - 1] ^= 0x80;               /* un bit del último byte del payload */
    CHECK(feedEqBytes(&p, frame, EQ_FRAME_BYTES, received) == 0);
}

/* Cabecera con el CMD y el LEN dados, seguida de 600 bytes de relleno (0x11) y un CHK: el parser no tiene que escribir
 * ni uno en los buffers de recepción ni avisar. Cuenta los avisos de cualquiera de los dos comandos. El relleno no usa
 * 0xA5, que sería un inicio de trama */
static uint32_t feedFrameWithCmdAndLen(UartParser *p, uint8_t cmd, uint16_t len, int16_t *waveBuf, int32_t *eqBuf) {
    const uint8_t header[4] = {0xA5, cmd, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)};
    uint8_t filler[601];
    uint32_t ready = 0;
    memset(filler, 0x11, sizeof filler);
    filler[600] = 0x00;
    for (uint32_t i = 0; i < sizeof header; i++) {
        ready += (uartParserFeed(p, header[i], waveBuf, eqBuf) != UART_PARSER_NONE);
    }
    for (uint32_t i = 0; i < sizeof filler; i++) {
        ready += (uartParserFeed(p, filler[i], waveBuf, eqBuf) != UART_PARSER_NONE);
    }
    return ready;
}

/* Un LEN que no le corresponde al CMD se descarta apenas se lee, sin tocar ningún buffer: UPLOAD_WAVE con el LEN de
 * SET_EQ (60), y SET_EQ con 0, 59, 61, con el LEN de UPLOAD_WAVE (512) y con 0xFFFF. Con SET_EQ y LEN = 512 el parser
 * escribiría 512 bytes en un buffer de 60, así que el buffer de coeficientes de la prueba lleva 640 bytes de guarda
 * detrás para ver si se pasa */
static void testLenQueNoCorrespondeAlCmdSeDescartaSinEscribirLosBuffers(void) {
    typedef struct {
        int32_t coefs[UART_EQ_COEF_COUNT];
        int32_t guard[160];
    } EqRxWithGuard;
    static const struct { uint8_t cmd; uint16_t len; } cases[] = {
        {0x01, 60}, {0x04, 0}, {0x04, 59}, {0x04, 61}, {0x04, 512}, {0x04, 0xFFFF},
    };
    for (uint32_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        int16_t wave[WAVE_SIZE];
        int16_t untouchedWave[WAVE_SIZE];
        EqRxWithGuard eq;
        EqRxWithGuard untouchedEq;
        UartParser p;
        uartParserInit(&p);
        memset(wave, 0x55, sizeof wave);
        memset(untouchedWave, 0x55, sizeof untouchedWave);
        memset(&eq, 0x55, sizeof eq);
        memset(&untouchedEq, 0x55, sizeof untouchedEq);

        CHECK(feedFrameWithCmdAndLen(&p, cases[k].cmd, cases[k].len, wave, eq.coefs) == 0);
        CHECK(memcmp(wave, untouchedWave, sizeof wave) == 0);
        CHECK(memcmp(&eq, &untouchedEq, sizeof eq) == 0);
    }
}

/* Una trama con un CMD desconocido se descarta aunque llegue completa y con el checksum correcto: no avisa ni escribe
 * ningún buffer. Se prueban CMD sin comando (0x00, 0x02, 0x03, 0x05, 0xFF) y el ACK (0x06), que es de la placa a la PC y
 * nunca debería llegar, cada uno con el LEN de SET_EQ (60), el de UPLOAD_WAVE (512) y LEN = 0. El payload es de 0x11 (no
 * es un inicio de trama) y el CHK se calcula con la fórmula de docs/protocolo.md */
static void testCmdDesconocidoSeDescartaAunqueLaTramaSeaValida(void) {
    typedef struct {
        int32_t coefs[UART_EQ_COEF_COUNT];
        int32_t guard[160];
    } EqRxWithGuard;
    static const uint8_t cmds[] = {0x00, 0x02, 0x03, 0x05, 0x06, 0xFF};
    static const uint16_t lens[] = {UART_PAYLOAD_EQ_BYTES, UART_PAYLOAD_WAVE_BYTES, 0};
    for (uint32_t c = 0; c < sizeof cmds / sizeof cmds[0]; c++) {
        for (uint32_t l = 0; l < sizeof lens / sizeof lens[0]; l++) {
            uint8_t frame[4 + 2 * WAVE_SIZE + 1];
            const uint32_t frameBytes = 4u + lens[l] + 1u;
            int16_t wave[WAVE_SIZE];
            int16_t untouchedWave[WAVE_SIZE];
            EqRxWithGuard eq;
            EqRxWithGuard untouchedEq;
            UartParser p;
            uartParserInit(&p);
            memset(wave, 0x55, sizeof wave);
            memset(untouchedWave, 0x55, sizeof untouchedWave);
            memset(&eq, 0x55, sizeof eq);
            memset(&untouchedEq, 0x55, sizeof untouchedEq);

            frame[0] = 0xA5;
            frame[1] = cmds[c];
            frame[2] = (uint8_t)(lens[l] & 0xFF);
            frame[3] = (uint8_t)(lens[l] >> 8);
            memset(frame + 4, 0x11, lens[l]);
            uint32_t sum = 0;
            for (uint32_t i = 1; i < frameBytes - 1; i++) {
                sum += frame[i];
            }
            frame[frameBytes - 1] = (uint8_t)((0x100 - (sum & 0xFF)) & 0xFF);

            uint32_t ready = 0;
            for (uint32_t i = 0; i < frameBytes; i++) {
                ready += (uartParserFeed(&p, frame[i], wave, eq.coefs) != UART_PARSER_NONE);
            }
            CHECK(ready == 0);
            CHECK(memcmp(wave, untouchedWave, sizeof wave) == 0);
            CHECK(memcmp(&eq, &untouchedEq, sizeof eq) == 0);
        }
    }
}

int main(void) {
    testTramaValidaAvisaYElBufferQuedaIgualALaTablaEnviada();
    testTramaConA5DentroDelPayloadNoSeReinicia();
    testTramaPartidaEnVariosLlamadosAvisaSoloAlFinal();
    testChecksumMaloNoAvisa();
    testLenDistintoDe512SeDescartaSinEscribirElBuffer();
    testDespuesDeUnaTramaInvalidaAceptaLaSiguienteValida();
    testSetEqValidaAvisaYElBufferQuedaIgualALosCoeficientesEnviados();
    testSetEqConChecksumMaloNoAvisa();
    testLenQueNoCorrespondeAlCmdSeDescartaSinEscribirLosBuffers();
    testCmdDesconocidoSeDescartaAunqueLaTramaSeaValida();

    if (failures > 0) {
        printf("%d fallas\n", failures);
        return 1;
    }
    printf("OK\n");
    return 0;
}
