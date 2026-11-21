"""Respuesta del filtro Sallen-Key de ganancia unitaria (spec §3.7)."""

import math


def attenuation_db(r1, r2, c1, c2, f):
    """Ganancia en dB a la frecuencia f (Hz).

    c1 es el capacitor de realimentación (a la salida del LM358) y c2 el que
    va a GND. H(s) = 1 / (s² r1 r2 c1 c2 + s c2 (r1 + r2) + 1).
    """
    s = 2j * math.pi * f
    h = 1 / (s * s * r1 * r2 * c1 * c2 + s * c2 * (r1 + r2) + 1)
    return 20 * math.log10(abs(h))
