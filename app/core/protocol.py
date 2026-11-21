"""Tramas UART entre la app y la placa (stories 13 y 14, spec 3.14, docs/protocolo.md).

Lógica pura: no importa PySide6 ni abre puertos.
"""
import numpy as np

FRAME_START = 0xA5
CMD_UPLOAD_WAVE = 0x01
CMD_SET_EQ = 0x04


def checksum(data):
    """Complemento a dos de la suma de 8 bits de data (CMD, LEN_L, LEN_H y payload): con el CHK incluido, la suma da 0."""
    return (-sum(data)) & 0xFF


def build_upload_wave_frame(table):
    """Trama UPLOAD_WAVE para una tabla de 256 muestras int16: 0xA5 | CMD | LEN_L | LEN_H | payload | CHK."""
    payload = np.asarray(table).astype("<i2").tobytes()      # little-endian, el byte bajo primero
    body = bytes([CMD_UPLOAD_WAVE]) + len(payload).to_bytes(2, "little") + payload
    return bytes([FRAME_START]) + body + bytes([checksum(body)])


def build_set_eq_frame(coefficients):
    """Trama SET_EQ para los 15 coeficientes Q2.30 del ecualizador (int32, en el orden de docs/protocolo.md)."""
    payload = np.asarray(coefficients).astype("<i4").tobytes()      # little-endian, el byte bajo primero
    body = bytes([CMD_SET_EQ]) + len(payload).to_bytes(2, "little") + payload
    return bytes([FRAME_START]) + body + bytes([checksum(body)])
