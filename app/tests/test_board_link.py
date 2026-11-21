"""Tests del envío de una onda a la placa (story 13, spec 3.14, docs/protocolo.md).

Usan un puerto falso con la misma interfaz que serial.Serial (write y read). El ACK esperado sale de
docs/protocolo.md: A5 06 00 00 FA.
"""
import numpy as np

from core.board_link import send_eq, send_wave
from core.equalizer import eq_coefficients
from core.protocol import build_set_eq_frame, build_upload_wave_frame

ACK = b"\xA5\x06\x00\x00\xFA"


class FakePort:
    """Puerto falso: guarda lo que se escribe y contesta con los bytes dados. Si no queda nada que leer, read
    devuelve lo que haya (vacío), como serial.Serial.read cuando se cumple el timeout."""

    def __init__(self, response=b""):
        self.written = b""
        self.pending = response

    def write(self, data):
        self.written += data
        return len(data)

    def read(self, size):
        data, self.pending = self.pending[:size], self.pending[size:]
        return data


def make_table():
    return (np.arange(256, dtype=np.int32) * 251 - 32000).astype(np.int16)


def test_envio_ok_cuando_llega_el_ack():
    table = make_table()
    port = FakePort(response=ACK)

    assert send_wave(port, table) is True
    assert port.written == build_upload_wave_frame(table)


def test_envio_falla_por_timeout_si_no_hay_respuesta():
    table = make_table()
    port = FakePort(response=b"")      # la placa no contesta: read devuelve vacío al vencer el timeout

    assert send_wave(port, table) is False
    assert port.written == build_upload_wave_frame(table)     # la trama sí salió


def test_una_respuesta_que_no_es_ack_no_cuenta_como_exito():
    table = make_table()
    nack_like = b"\xA5\x15\x01\x00\x01\xE9"      # con forma de NACK: la placa no envía NACK, pero no es un ACK
    garbage = b"\x00\x01\x02\x03\x04"
    partial_ack = ACK[:3]                          # el ACK cortado a la mitad

    for response in (nack_like, garbage, partial_ack):
        assert send_wave(FakePort(response=response), table) is False


# Story 14: SET_EQ por el mismo camino que la onda, con el mismo criterio de ACK


def test_envio_de_set_eq_ok_cuando_llega_el_ack():
    coefs = eq_coefficients(-12, 6, -3)
    port = FakePort(response=ACK)

    assert send_eq(port, coefs) is True
    assert port.written == build_set_eq_frame(coefs)


def test_envio_de_set_eq_falla_por_timeout_si_no_hay_respuesta():
    coefs = eq_coefficients(0, 3, 0)
    port = FakePort(response=b"")      # la placa no contesta: read devuelve vacío al vencer el timeout

    assert send_eq(port, coefs) is False
    assert port.written == build_set_eq_frame(coefs)     # la trama sí salió


def test_una_respuesta_que_no_es_ack_a_set_eq_no_cuenta_como_exito():
    coefs = eq_coefficients(6, 0, -12)

    assert send_eq(FakePort(response=b"\xA5\x06\x00\x00\xFB"), coefs) is False     # CHK equivocado
    assert send_eq(FakePort(response=b"\xA5\x06\x00"), coefs) is False                # ACK cortado
