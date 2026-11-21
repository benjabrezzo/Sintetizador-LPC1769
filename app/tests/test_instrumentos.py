"""Tests de los instrumentos precargados del editor (story 12, spec 3.3).

Lógica pura: estos tests no importan PySide6. Los valores esperados son la tabla de la spec 3.3, copiada a mano.
"""
import numpy as np
import pytest

from core import instrumentos
from core.armonicos import Editor

# Spec 3.3, tabla de instrumentos: amplitudes de los armónicos 1 a 30, en % enteros de la fundamental
AMPLITUDES_SPEC = {
    "Guitarra": [
        100, 40, 18, 6, 0, 3, 3, 3, 1, 0,
        1, 1, 1, 1, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ],
    "Piano": [
        100, 90, 75, 56, 36, 17, 0, 13, 20, 22,
        20, 15, 8, 0, 7, 11, 13, 12, 9, 5,
        0, 5, 8, 9, 9, 7, 4, 0, 3, 6,
    ],
    "Órgano": [
        100, 100, 100, 100, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ],
    "Flauta": [
        100, 20, 5, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ],
}


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_cada_instrumento_carga_sus_30_porcentajes_de_la_spec(nombre):
    editor = Editor()
    editor.load_instrument(nombre)
    assert [round(100 * amplitud) for amplitud in editor.amplitudes] == AMPLITUDES_SPEC[nombre]


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_espectro_de_cada_instrumento_respeta_las_proporciones(nombre):
    # Tolerancia 0,001 en la razón respecto de la fundamental (0,1 %), muy por encima del piso de cuantización (0,31 LSB)
    editor = Editor()
    editor.load_instrument(nombre)
    espectro = editor.spectrum()
    proporciones = espectro / espectro[0]
    esperadas = np.array(AMPLITUDES_SPEC[nombre]) / 100
    assert np.max(np.abs(proporciones - esperadas)) <= 0.001


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_mensaje_tiene_una_linea_por_pote_en_orden(nombre):
    lineas = instrumentos.ADSR_MESSAGES[nombre].splitlines()
    assert [linea.split(":")[0] for linea in lineas] == ["Attack", "Decay", "Sustain", "Release"]


def test_cada_instrumento_tiene_un_mensaje_distinto():
    mensajes = [instrumentos.ADSR_MESSAGES[nombre] for nombre in AMPLITUDES_SPEC]
    assert len(set(mensajes)) == len(AMPLITUDES_SPEC)


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_cargar_un_instrumento_no_cambia_la_nota(nombre):
    editor = Editor()
    editor.set_note("Sol4")
    editor.load_instrument(nombre)
    assert editor.note == "Sol4"


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_mover_un_control_despues_de_cargar_cambia_la_onda(nombre):
    editor = Editor()
    editor.load_instrument(nombre)
    tabla_antes = editor.table()
    editor.set_amplitude(2, 0.0)  # se apaga el armónico 2
    assert not np.array_equal(editor.table(), tabla_antes)
    assert editor.spectrum()[1] < 100 / 32767  # por debajo de 1 LSB


@pytest.mark.parametrize("nombre", list(AMPLITUDES_SPEC))
def test_despues_de_cargar_se_exporta_256_enteros_con_pico_32767(nombre, tmp_path):
    editor = Editor()
    editor.load_instrument(nombre)
    editor.set_note("Do5")  # cambiar la nota no cambia la onda
    archivo = tmp_path / "onda.txt"
    editor.export(archivo)
    muestras = np.array([int(linea) for linea in archivo.read_text().splitlines()])
    assert len(muestras) == 256
    assert muestras[0] == 0
    assert np.max(np.abs(muestras)) == 32767
