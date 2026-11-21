"""Coeficientes del ecualizador de tres bandas (story 14, spec 3.5).

Lógica pura en numpy: no importa PySide6. La app calcula los coeficientes y la placa solo los aplica, así que el LPC1769
no necesita cálculo en punto flotante.
"""
import numpy as np

Q2_30_ONE = 1 << 30                  # 1,0 en Q2.30
FS = 25_000_000 / 781                # Hz: frecuencia de muestreo real de la placa (spec 3.1)

LOW_SHELF_HZ = 200.0                 # spec 3.5
PEAKING_HZ = 1000.0
HIGH_SHELF_HZ = 4000.0
SHELF_Q = 1 / np.sqrt(2)             # factor Q de los shelves (spec 3.5)
PEAKING_Q = 1.0                      # factor Q del peaking

MIN_GAIN_DB = -12.0                  # spec 3.5: por encima de +6 dB una nota sola ya saturaría
MAX_GAIN_DB = 6.0


def to_q2_30(value):
    """Un valor real en Q2.30 (int). Falla con ValueError si no entra: Q2.30 cubre de −2 a 2, sin incluir el 2."""
    raw = round(value * Q2_30_ONE)
    if not -(1 << 31) <= raw <= (1 << 31) - 1:
        raise ValueError(f"El coeficiente {value} no entra en Q2.30 (va de -2 a 2, sin incluir el 2)")
    return raw


def _rbj_coefficients(kind, f0, gain_db, q):
    """(b0, b1, b2, a1, a2) en coma flotante, normalizados con a0 = 1, del Audio EQ Cookbook (RBJ)."""
    amp = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0 / FS
    cos_w0 = np.cos(w0)
    alpha = np.sin(w0) / (2 * q)
    shelf_term = 2 * np.sqrt(amp) * alpha
    if kind == "low_shelf":
        b0 = amp * ((amp + 1) - (amp - 1) * cos_w0 + shelf_term)
        b1 = 2 * amp * ((amp - 1) - (amp + 1) * cos_w0)
        b2 = amp * ((amp + 1) - (amp - 1) * cos_w0 - shelf_term)
        a0 = (amp + 1) + (amp - 1) * cos_w0 + shelf_term
        a1 = -2 * ((amp - 1) + (amp + 1) * cos_w0)
        a2 = (amp + 1) + (amp - 1) * cos_w0 - shelf_term
    elif kind == "high_shelf":
        b0 = amp * ((amp + 1) + (amp - 1) * cos_w0 + shelf_term)
        b1 = -2 * amp * ((amp - 1) + (amp + 1) * cos_w0)
        b2 = amp * ((amp + 1) + (amp - 1) * cos_w0 - shelf_term)
        a0 = (amp + 1) - (amp - 1) * cos_w0 + shelf_term
        a1 = 2 * ((amp - 1) - (amp + 1) * cos_w0)
        a2 = (amp + 1) - (amp - 1) * cos_w0 - shelf_term
    else:   # peaking
        b0 = 1 + alpha * amp
        b1 = -2 * cos_w0
        b2 = 1 - alpha * amp
        a0 = 1 + alpha / amp
        a1 = -2 * cos_w0
        a2 = 1 - alpha / amp
    return b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0


def _band_coefficients(kind, f0, gain_db, q):
    """Los 5 coeficientes Q2.30 de una banda. Con 0 dB, la identidad trivial (b0 = 1,0 y el resto en cero)."""
    if not MIN_GAIN_DB <= gain_db <= MAX_GAIN_DB:
        raise ValueError(f"La ganancia {gain_db} dB está fuera de rango: va de {MIN_GAIN_DB:g} a {MAX_GAIN_DB:g} dB (spec 3.5)")
    if gain_db == 0:
        return [Q2_30_ONE, 0, 0, 0, 0]
    return [to_q2_30(c) for c in _rbj_coefficients(kind, f0, gain_db, q)]


def eq_coefficients(low_db, mid_db, high_db):
    """Los 15 coeficientes Q2.30 (int32) de las tres bandas, en el orden de docs/protocolo.md.

    Orden: low-shelf (200 Hz), peaking (1 kHz) y high-shelf (4 kHz); en cada banda, b0, b1, b2, a1, a2.
    """
    coefficients = (
        _band_coefficients("low_shelf", LOW_SHELF_HZ, low_db, SHELF_Q)
        + _band_coefficients("peaking", PEAKING_HZ, mid_db, PEAKING_Q)
        + _band_coefficients("high_shelf", HIGH_SHELF_HZ, high_db, SHELF_Q)
    )
    return np.array(coefficients, dtype=np.int32)
