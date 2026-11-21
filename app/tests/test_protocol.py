"""Tests de las tramas UART de la app (story 13, spec 3.14, docs/protocolo.md).

Los valores esperados salen de los ejemplos calculados a mano en docs/protocolo.md, no del código de la app.
"""
import numpy as np

from core.equalizer import eq_coefficients
from core.protocol import build_set_eq_frame, build_upload_wave_frame


def test_trama_de_tabla_en_cero_tiene_cabecera_largo_y_checksum():
    frame = build_upload_wave_frame(np.zeros(256, dtype=np.int16))

    # 0xA5, CMD = 0x01, LEN = 512 (0x0200 en little-endian), 512 bytes de payload y CHK = 0xFD
    assert frame == b"\xA5\x01\x00\x02" + b"\x00" * 512 + b"\xFD"
    assert len(frame) == 517


def test_muestras_se_codifican_int16_little_endian():
    table = np.zeros(256, dtype=np.int16)
    table[0] = 0x1234
    table[1] = -2         # 0xFFFE
    table[2] = -32768     # 0x8000
    table[255] = 32767    # 0x7FFF
    frame = build_upload_wave_frame(table)

    payload = frame[4:-1]
    assert payload[:6] == b"\x34\x12\xFE\xFF\x00\x80"
    assert payload[-2:] == b"\xFF\x7F"

    # Ejemplo de docs/protocolo.md: muestra 0 = 0x1234 y el resto en cero, CHK = 0xB7 calculado a mano
    solo_la_muestra_0 = np.zeros(256, dtype=np.int16)
    solo_la_muestra_0[0] = 0x1234
    assert build_upload_wave_frame(solo_la_muestra_0)[-1] == 0xB7


def test_suma_de_cmd_a_chk_da_cero_modulo_256():
    rng = np.random.default_rng(13)
    for _ in range(20):
        table = rng.integers(-32768, 32768, size=256).astype(np.int16)
        frame = build_upload_wave_frame(table)
        assert frame[0] == 0xA5
        assert sum(frame[1:]) % 256 == 0


# Story 14: trama SET_EQ (docs/protocolo.md): 0xA5 | 0x04 | LEN = 60 | 15 coeficientes int32 little-endian | CHK


def test_trama_set_eq_de_la_identidad_tiene_cabecera_largo_y_checksum():
    frame = build_set_eq_frame(eq_coefficients(0, 0, 0))

    # Ejemplo de docs/protocolo.md: A5 04 3C 00 | (00 00 00 40, 00 × 16) × 3 | 00. LEN = 60 es 0x3C 0x00 y la suma
    # 0x04 + 0x3C + 0x00 + 3 × 0x40 = 0x100 da 0 en 8 bits
    assert frame == b"\xA5\x04\x3C\x00" + (b"\x00\x00\x00\x40" + b"\x00" * 16) * 3 + b"\x00"
    assert len(frame) == 65


def test_coeficientes_se_codifican_int32_little_endian_en_el_orden_del_protocolo():
    coefs = np.zeros(15, dtype=np.int32)
    coefs[0] = 0x12345678        # b0 del low-shelf
    coefs[3] = -2                # a1 del low-shelf: 0xFFFFFFFE
    coefs[5] = 1                 # b0 del peaking
    coefs[14] = -(2 ** 31)       # a2 del high-shelf: 0x80000000
    payload = build_set_eq_frame(coefs)[4:-1]

    assert payload[0:4] == b"\x78\x56\x34\x12"
    assert payload[12:16] == b"\xFE\xFF\xFF\xFF"
    assert payload[20:24] == b"\x01\x00\x00\x00"
    assert payload[56:60] == b"\x00\x00\x00\x80"

    # Ejemplo de docs/protocolo.md: b0 del low-shelf en 0x12345678 y el resto en cero, CHK = 0xAC calculado a mano
    solo_b0 = np.zeros(15, dtype=np.int32)
    solo_b0[0] = 0x12345678
    assert build_set_eq_frame(solo_b0)[-1] == 0xAC


def test_suma_de_cmd_a_chk_de_set_eq_da_cero_modulo_256():
    rng = np.random.default_rng(14)
    for _ in range(20):
        coefs = rng.integers(-(2 ** 31), 2 ** 31, size=15).astype(np.int32)
        frame = build_set_eq_frame(coefs)
        assert frame[0] == 0xA5
        assert frame[1] == 0x04
        assert sum(frame[1:]) % 256 == 0
