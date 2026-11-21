"""Síntesis aditiva de un período de la onda por armónicos (story 11, spec 3.3).

Lógica pura en numpy: no importa PySide6.
"""
import numpy as np

from core.instrumentos import AMPLITUDES

WAVE_SIZE = 256     # muestras por período
FULL_SCALE = 32767   # máximo de una muestra Q15
MAX_HARMONIC = 30    # spec 3.3: armónicos 1 a 30


def build_table(amplitudes):
    """Un período de la onda en int16, con fase cero.

    amplitudes[k-1] es la amplitud (de 0 a 1) del armónico k: amplitudes[0] es la fundamental.
    """
    if len(amplitudes) > MAX_HARMONIC:
        raise ValueError(f"Hay {len(amplitudes)} amplitudes; el máximo es {MAX_HARMONIC} armónicos (spec 3.3)")
    if not np.any(amplitudes):
        raise ValueError("Todas las amplitudes están en cero: no hay onda que generar")
    n = np.arange(WAVE_SIZE)
    raw = np.zeros(WAVE_SIZE)
    for k, amplitude in enumerate(amplitudes, start=1):
        raw += amplitude * np.sin(2 * np.pi * k * n / WAVE_SIZE)
    peak = np.max(np.abs(raw))
    return np.round(FULL_SCALE * raw / peak).astype(np.int16)


def spectrum(table):
    """Amplitud de los armónicos 1 a 30 de una tabla int16, en % del pico de escala.

    El índice k-1 del resultado corresponde al armónico k.
    """
    magnitudes = np.abs(np.fft.rfft(table.astype(np.float64))) / (WAVE_SIZE / 2)
    return 100 * magnitudes[1:MAX_HARMONIC + 1] / FULL_SCALE


def harmonic_frequencies(fundamental_hz):
    """Frecuencia en Hz de los armónicos 1 a 30 de una fundamental: k × f0 (spec 3.3)."""
    return np.arange(1, MAX_HARMONIC + 1) * fundamental_hz


# Notas de las ocho teclas (spec 3.11), en Hz
NOTES = {
    "Do4": 261.63, "Re4": 293.66, "Mi4": 329.63, "Fa4": 349.23,
    "Sol4": 392.00, "La4": 440.00, "Si4": 493.88, "Do5": 523.25,
}


class Editor:
    """Estado del editor: nota elegida y amplitudes de los armónicos 1 a 30 (de 0 a 1)."""

    def __init__(self):
        self.note = "La4"
        self.amplitudes = [1.0] + [0.0] * (MAX_HARMONIC - 1)

    def set_note(self, note):
        self.note = note

    def set_amplitude(self, k, amplitude):
        self.amplitudes[k - 1] = amplitude

    def load_instrument(self, name):
        """Carga las 30 amplitudes de un instrumento (spec 3.3). No cambia la nota."""
        self.amplitudes = [pct / 100 for pct in AMPLITUDES[name]]

    def frequencies(self):
        return harmonic_frequencies(NOTES[self.note])

    def table(self):
        return build_table(self.amplitudes)

    def spectrum(self):
        return spectrum(self.table())

    def export(self, path):
        """Escribe la tabla int16 como texto: 256 enteros, uno por línea."""
        tabla = self.table()
        with open(path, "w") as archivo:
            for muestra in tabla:
                archivo.write(f"{int(muestra)}\n")
