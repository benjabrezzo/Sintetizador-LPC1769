"""Tests de los coeficientes del ecualizador de la app (story 14, spec 3.5, docs/protocolo.md).

Los valores esperados salen de la definición del formato Q2.30 y de la curva teórica de cada banda (RBJ, Fs = 25 MHz / 781,
Q = 1/√2 en los shelves y 1 en el peaking), no del código de la app.
"""
import numpy as np
import pytest

from core.equalizer import eq_coefficients, to_q2_30

Q2_30_ONE = 1 << 30      # 1,0 en Q2.30
FS = 25_000_000 / 781    # Hz: frecuencia de muestreo real de la placa (spec 3.1)
TOLERANCE_DB = 1.0       # tolerancia de la respuesta (story 14)

# Ganancia esperada de cada banda sola (las otras en 0 dB), con la banda en −12 y en +6 dB, en las frecuencias donde la
# curva teórica de la banda vale lo indicado: la ganancia G lejos de la rodilla o en el centro, G/2 en la frecuencia del
# shelf, 0 dB fuera de la banda y un punto del flanco que fija el Q. Son los mismos valores de los tests de C del
# ecualizador (firmware/tests/test_equalizer.c). Banda 0: low-shelf, 1: peaking, 2: high-shelf. (frecuencia en Hz, dB)
ANCHORS = {
    0: {-12: [(40, -11.97), (133, -9.71), (200, -6.00), (5000, 0.0)],
        6: [(40, 5.99), (133, 4.98), (200, 3.00), (5000, 0.0)]},
    1: {-12: [(100, 0.0), (500, -3.96), (1000, -12.00), (10000, 0.0)],
        6: [(100, 0.0), (500, 1.88), (1000, 6.00), (10000, 0.0)]},
    2: {-12: [(500, 0.0), (4000, -6.00), (6000, -10.15), (12000, -11.99)],
        6: [(500, 0.0), (4000, 3.00), (6000, 5.19), (12000, 5.99)]},
}


def response_db(band, freq):
    """Ganancia en dB, a la frecuencia freq (Hz), de una banda de 5 coeficientes Q2.30 (b0, b1, b2, a1, a2; a0 = 1)."""
    b0, b1, b2, a1, a2 = np.asarray(band, dtype=np.float64) / Q2_30_ONE
    z1 = np.exp(-1j * 2 * np.pi * freq / FS)
    return 20 * np.log10(abs((b0 + b1 * z1 + b2 * z1 ** 2) / (1 + a1 * z1 + a2 * z1 ** 2)))


def test_0_db_da_coeficientes_de_identidad():
    coefs = eq_coefficients(0, 0, 0)

    # Cada banda: b0 = 1,0 en Q2.30 (0x40000000) y el resto en cero. Orden: low-shelf, peaking, high-shelf (docs/protocolo.md)
    assert coefs.dtype == np.int32
    assert coefs.tolist() == [Q2_30_ONE, 0, 0, 0, 0] * 3


@pytest.mark.parametrize("band", [0, 1, 2])
@pytest.mark.parametrize("gain_db", [-12, 6])
def test_cada_banda_cumple_su_ganancia_en_los_extremos(band, gain_db):
    gains = [0, 0, 0]
    gains[band] = gain_db
    coefs = eq_coefficients(*gains)

    for freq, expected_db in ANCHORS[band][gain_db]:
        measured_db = response_db(coefs[5 * band:5 * band + 5], freq)
        assert measured_db == pytest.approx(expected_db, abs=TOLERANCE_DB), f"{freq} Hz"


def band_coefficients_over_grid():
    """(banda, ganancia, coeficientes reales de esa banda) para las 3 bandas y las 19 ganancias de −12 a +6 dB."""
    for band in range(3):
        for gain_db in range(-12, 7):
            gains = [0, 0, 0]
            gains[band] = gain_db
            coefs = eq_coefficients(*gains)[5 * band:5 * band + 5]
            yield band, gain_db, coefs.astype(np.float64) / Q2_30_ONE


def test_toda_la_grilla_entra_en_q2_30_y_es_estable():
    for band, gain_db, values in band_coefficients_over_grid():
        # Q2.30 cubre de −2 a 2 sin incluir el 2
        assert np.all(values >= -2.0) and np.all(values < 2.0), f"banda {band}, {gain_db} dB"
        # Estable: los dos polos (raíces de z² + a1·z + a2) dentro del círculo unidad
        a1, a2 = values[3], values[4]
        assert np.max(np.abs(np.roots([1.0, a1, a2]))) < 1.0, f"banda {band}, {gain_db} dB"


def test_el_caso_mas_justo_es_el_low_shelf_a_mas_6_db():
    # Story 14: con esos Q y Fs = 32,01 kHz, el coeficiente más cercano al límite de Q2.30 es el a1 del low-shelf a +6 dB,
    # con |a1| = 1,953 contra el límite de 2. a1 es el cuarto coeficiente de la banda (índice 3)
    largest = max(
        ((abs(value), band, gain_db, index)
         for band, gain_db, values in band_coefficients_over_grid()
         for index, value in enumerate(values)),
    )
    magnitude, band, gain_db, index = largest

    assert (band, gain_db, index) == (0, 6, 3)
    assert magnitude == pytest.approx(1.953, abs=0.001)
    assert magnitude < 2.0


@pytest.mark.parametrize("gains", [(-12.5, 0, 0), (0, 6.5, 0), (0, 0, 7), (-13, 0, 0), (0, 0, -12.01)])
def test_una_ganancia_fuera_de_rango_se_rechaza(gains):
    # Spec 3.5: cada banda va de −12 a +6 dB; por encima de +6 dB una nota sola ya saturaría
    with pytest.raises(ValueError):
        eq_coefficients(*gains)


def test_un_coeficiente_que_no_entra_en_q2_30_se_rechaza():
    # Q2.30 cubre de −2 a 2 sin incluir el 2: el menor entero es −2^31 y el mayor, 2^31 − 1
    assert to_q2_30(-2.0) == -(1 << 31)
    assert to_q2_30(1.0) == 1 << 30
    assert to_q2_30((2 ** 31 - 1) / 2 ** 30) == 2 ** 31 - 1
    for value in (2.0, 2.5, -2.0000001, -3.0):
        with pytest.raises(ValueError):
            to_q2_30(value)
