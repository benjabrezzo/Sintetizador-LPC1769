#!/usr/bin/env python3
"""Generador de tablas del sintetizador (spec 3.10).

Escribe firmware/src/tables.c y firmware/inc/tables.h. Se corre desde la raíz del repo:
    python3 firmware/tools/gen_tables.py

Por ahora genera la senoide (Bloque 1), los incrementos de las notas (Bloque 4) y las ondas cuadrada,
sierra y triangular (Bloque 6) la ventana Hann y los factores de giro de la FFT (Bloque 11). Las demás tablas nacen con su bloque.
"""
import math
from pathlib import Path

WAVE_SIZE = 256          # muestras por período
Q15_MAX = 32767          # amplitud máxima de una muestra Q15
FFT_SIZE = 512           # puntos de la FFT (spec 3.6)
MAX_HARMONIC = 30        # spec 3.3: 30 × 523,25 Hz = 15,7 kHz < 16 kHz (Nyquist a 32 kHz)

DAC_PCLK_HZ = 25_000_000              # PCLK del DAC: CCLK / 4
DAC_COUNTER_VALUE = 781               # DACCNTVAL, igual que firmware/inc/config.h
FS = DAC_PCLK_HZ / DAC_COUNTER_VALUE  # Fs real: 32 010,24 Hz
PHASE_WRAP = 1 << 24                  # la fase Q16.16 da la vuelta en 2^24 (spec 3.1)

# Escala de Do mayor de Do4 a Do5, tal cual la tabla de la spec 3.11
NOTES = [
    ("Do4", 261.63), ("Re4", 293.66), ("Mi4", 329.63), ("Fa4", 349.23),
    ("Sol4", 392.00), ("La4", 440.00), ("Si4", 493.88), ("Do5", 523.25),
]

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "firmware" / "inc" / "tables.h"
SOURCE = ROOT / "firmware" / "src" / "tables.c"

WARNING = "/* GENERADO por firmware/tools/gen_tables.py. No editar a mano. */\n"


def sineTable():
    """Un período completo de seno, en Q15 y redondeado al entero más cercano."""
    return [round(Q15_MAX * math.sin(2 * math.pi * n / WAVE_SIZE)) for n in range(WAVE_SIZE)]


def harmonicTable(harmonics):
    """Suma de senoides (spec 3.8). harmonics: lista de (k, amplitud) con el orden k del armónico.

    Se normaliza con el pico de la propia tabla, así que la muestra más grande vale exactamente Q15_MAX
    y el rizado de Gibbs de la suma no desborda."""
    raw = [sum(a * math.sin(2 * math.pi * k * n / WAVE_SIZE) for k, a in harmonics) for n in range(WAVE_SIZE)]
    peak = max(abs(v) for v in raw)
    return [round(Q15_MAX * v / peak) for v in raw]


def squareTable():
    """Cuadrada: armónicos impares con amplitud 1/k."""
    return harmonicTable([(k, 1 / k) for k in range(1, MAX_HARMONIC + 1, 2)])


def sawTable():
    """Diente de sierra: todos los armónicos con amplitud (-1)^(k+1)/k, así que la onda sube desde cero."""
    return harmonicTable([(k, (-1) ** (k + 1) / k) for k in range(1, MAX_HARMONIC + 1)])


def triangleTable():
    """Triangular: armónicos impares con amplitud (-1)^((k-1)/2)/k²."""
    return harmonicTable([(k, (-1) ** ((k - 1) // 2) / k ** 2) for k in range(1, MAX_HARMONIC + 1, 2)])


def hannWindow():
    """Ventana Hann periódica de FFT_SIZE puntos en Q15: w[n] = ½(1 − cos(2πn/N)). El máximo (1,0) cae en N/2
    y se satura a Q15_MAX."""
    return [min(Q15_MAX, round(32768 * 0.5 * (1 - math.cos(2 * math.pi * n / FFT_SIZE)))) for n in range(FFT_SIZE)]


def twiddleTable(trig):
    """Factores de giro W_N^k = cos(2πk/N) − j·sin(2πk/N) para k = 0 ... N/2 − 1, en Q15. trig es math.cos o
    math.sin: el seno se guarda positivo (el signo del giro lo pone la FFT). Se satura a Q15_MAX (cos 0 = 1,0)."""
    return [min(Q15_MAX, round(32768 * trig(2 * math.pi * k / FFT_SIZE))) for k in range(FFT_SIZE // 2)]


def noteSteps():
    """Incremento de fase por muestra de cada nota: ΔP = f · 2^24 / Fs, redondeado."""
    return [round(hz * PHASE_WRAP / FS) for _, hz in NOTES]


def renderHeader():
    return (
        WARNING
        + "#ifndef TABLES_H\n"
        + "#define TABLES_H\n\n"
        + "#include <stdint.h>\n\n"
        + f"#define WAVE_SIZE {WAVE_SIZE}   /* muestras por período */\n"
        + f"#define NUM_NOTES {len(NOTES)}     /* teclas de Do4 a Do5 (spec 3.11) */\n"
        + f"#define FFT_SIZE {FFT_SIZE}    /* puntos de la FFT (spec 3.6) */\n\n"
        + "/* Senoide de un período en Q15 (Flash) */\n"
        + "extern const int16_t sineTable[WAVE_SIZE];\n\n"
        + f"/* Cuadrada, sierra y triangular de un período en Q15, limitadas en banda a {MAX_HARMONIC} armónicos (Flash) */\n"
        + "extern const int16_t squareTable[WAVE_SIZE];\n"
        + "extern const int16_t sawTable[WAVE_SIZE];\n"
        + "extern const int16_t triangleTable[WAVE_SIZE];\n\n"
        + "/* Incremento de fase por muestra de cada nota (Q16.16, vuelta en 2^24),\n"
        + f" * con Fs = {DAC_PCLK_HZ // 1_000_000} MHz / {DAC_COUNTER_VALUE} (Flash) */\n"
        + "extern const uint32_t noteSteps[NUM_NOTES];\n\n"
        + "/* Ventana Hann periódica de FFT_SIZE puntos en Q15 (Flash) */\n"
        + "extern const int16_t hannWindow[FFT_SIZE];\n\n"
        + "/* Factores de giro de la FFT, W_N^k = cos(2πk/N) − j·sin(2πk/N) para k = 0 ... FFT_SIZE/2 − 1, en Q15:\n"
        + " * el coseno y el seno (positivo) de 2πk/N (Flash) */\n"
        + "extern const int16_t fftTwiddleCos[FFT_SIZE / 2];\n"
        + "extern const int16_t fftTwiddleSin[FFT_SIZE / 2];\n\n"
        + "#endif /* TABLES_H */\n"
    )


def renderWave(name, samples, size="WAVE_SIZE"):
    rows = []
    for i in range(0, len(samples), 8):
        rows.append("    " + ", ".join(f"{s}" for s in samples[i:i + 8]) + ",\n")
    return f"const int16_t {name}[{size}] = {{\n" + "".join(rows) + "};\n\n"


def renderSource(waves, steps):
    """waves: lista de (nombre del arreglo, muestras), en el orden en que se escriben."""
    noteRows = [f"    {step}u,   /* {name} {hz:.2f} Hz */\n" for step, (name, hz) in zip(steps, NOTES)]
    return (
        WARNING
        + '#include "tables.h"\n\n'
        + "".join(renderWave(name, samples) for name, samples in waves)
        + renderWave("hannWindow", hannWindow(), "FFT_SIZE")
        + renderWave("fftTwiddleCos", twiddleTable(math.cos), "FFT_SIZE / 2")
        + renderWave("fftTwiddleSin", twiddleTable(math.sin), "FFT_SIZE / 2")
        + "const uint32_t noteSteps[NUM_NOTES] = {\n"
        + "".join(noteRows)
        + "};\n"
    )


def main():
    waves = [
        ("sineTable", sineTable()),
        ("squareTable", squareTable()),
        ("sawTable", sawTable()),
        ("triangleTable", triangleTable()),
    ]
    HEADER.write_text(renderHeader())
    SOURCE.write_text(renderSource(waves, noteSteps()))
    print(f"Escritos {HEADER.relative_to(ROOT)} y {SOURCE.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
