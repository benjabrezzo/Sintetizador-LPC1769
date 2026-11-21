"""Envío de una onda y de los coeficientes del ecualizador a la placa por el puerto serie (stories 13 y 14,
spec 3.14, docs/protocolo.md).

Lógica pura: no importa PySide6. El puerto es cualquier objeto con write y read (serial.Serial o uno falso).
"""
import serial
from serial.tools import list_ports

from core.protocol import build_set_eq_frame, build_upload_wave_frame

BAUD_RATE = 115200       # 8N1, sin control de flujo (docs/protocolo.md)
ACK_TIMEOUT_S = 1.0      # una trama tarda unos 45 ms en el cable: un segundo sobra para el ACK

# Respuesta de la placa cuando aceptó la trama (docs/protocolo.md): 0xA5 | ACK | LEN = 0 | CHK
ACK_FRAME = b"\xA5\x06\x00\x00\xFA"


def send_wave(port, table):
    """Envía la tabla y espera el ACK. Devuelve True solo si llegó exactamente el ACK.

    La espera la acota el timeout del puerto: si vence, read devuelve menos bytes de los pedidos.
    """
    port.write(build_upload_wave_frame(table))
    return port.read(len(ACK_FRAME)) == ACK_FRAME


def send_eq(port, coefficients):
    """Envía los 15 coeficientes del ecualizador y espera el ACK. Devuelve True solo si llegó exactamente el ACK."""
    port.write(build_set_eq_frame(coefficients))
    return port.read(len(ACK_FRAME)) == ACK_FRAME


def list_serial_ports():
    """Nombres de los puertos serie del sistema, p. ej. /dev/ttyUSB0 (el adaptador USB-TTL)."""
    return sorted(port.device for port in list_ports.comports())


def send_to_port(port_name, table):
    """Abre el puerto, envía la tabla, espera el ACK y cierra. Devuelve True si la placa confirmó."""
    with serial.Serial(port_name, BAUD_RATE, timeout=ACK_TIMEOUT_S) as port:
        return send_wave(port, table)


def send_eq_to_port(port_name, coefficients):
    """Abre el puerto, envía los coeficientes, espera el ACK y cierra. Devuelve True si la placa confirmó."""
    with serial.Serial(port_name, BAUD_RATE, timeout=ACK_TIMEOUT_S) as port:
        return send_eq(port, coefficients)
