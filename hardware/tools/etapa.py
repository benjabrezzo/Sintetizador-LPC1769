"""Nivel de salida de la etapa de volumen y LM386 (spec §3.7)."""

import math


def output_vrms(vpp_dac, r_series, r_pot, r_in_lm386, gain, r_out, r_load):
    """Vrms en la carga de un canal, con el volumen al máximo.

    Modelo ideal: el filtro pasa con ganancia 1 y el LM386 no recorta. Con el
    pote al máximo, el divisor es r_series contra (r_pot en paralelo con la
    entrada del LM386); luego ganancia y divisor r_out / r_load del canal.
    """
    r_down = r_pot * r_in_lm386 / (r_pot + r_in_lm386)
    vpp_pin3 = vpp_dac * r_down / (r_series + r_down)
    vpp_load = vpp_pin3 * gain * r_load / (r_load + r_out)
    return vpp_load / (2 * math.sqrt(2))
