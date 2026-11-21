"""Tests de la síntesis aditiva del editor de armónicos (story 11, spec 3.3).

Lógica pura: estos tests no importan PySide6.
"""
import re
from pathlib import Path

import numpy as np
import pytest

from core.armonicos import Editor, build_table, harmonic_frequencies, spectrum

TABLES_C = Path(__file__).resolve().parents[2] / "firmware" / "src" / "tables.c"


def leer_tabla_firmware(nombre):
    """Muestras de una tabla de firmware/src/tables.c, tal como las genera gen_tables.py."""
    texto = TABLES_C.read_text()
    cuerpo = re.search(rf"int16_t {nombre}\[WAVE_SIZE\] = \{{(.*?)\}};", texto, re.S).group(1)
    return np.array([int(x) for x in re.findall(r"-?\d+", cuerpo)])


def test_senoide_es_la_fundamental_al_100_por_ciento():
    # Fundamental al 100 % y el resto en 0: la senoide, con el pico en 32767
    amplitudes = [1.0] + [0.0] * 29
    esperada = np.round(32767 * np.sin(2 * np.pi * np.arange(256) / 256))
    assert np.array_equal(build_table(amplitudes), esperada)


def test_fundamental_mas_segundo_da_picos_en_1_y_2_con_proporcion():
    # Fundamental al 100 % y segundo armónico al 50 %: el espectro tiene picos en 1 y 2, y el 2 mide la mitad del 1
    amplitudes = [1.0, 0.5] + [0.0] * 28
    espectro = spectrum(build_table(amplitudes))
    assert espectro[0] > 0
    assert espectro[1] > 0
    assert espectro[1] / espectro[0] == pytest.approx(0.5, rel=1e-3)


def test_varios_armonicos_dan_picos_solo_donde_hay_amplitud():
    # Armónicos 1, 3, 5 y 10 con amplitudes 1, 0.5, 0.25 y 0.8: cada pico mide su amplitud respecto de la fundamental
    ejemplo = {1: 1.0, 3: 0.5, 5: 0.25, 10: 0.8}
    amplitudes = [ejemplo.get(k, 0.0) for k in range(1, 31)]
    espectro = spectrum(build_table(amplitudes))
    for k, amplitud in ejemplo.items():
        assert espectro[k - 1] / espectro[0] == pytest.approx(amplitud, rel=1e-3)


def test_sin_picos_fuera_de_los_armonicos_configurados():
    # Los armónicos sin amplitud no tienen pico: quedan por debajo de 1 LSB de Q15 (spec 3.3)
    ejemplo = {1: 1.0, 4: 0.5, 7: 0.3, 20: 0.9}
    amplitudes = [ejemplo.get(k, 0.0) for k in range(1, 31)]
    espectro = spectrum(build_table(amplitudes))
    un_lsb_en_porcentaje = 100 / 32767
    for k in range(1, 31):
        if k not in ejemplo:
            assert espectro[k - 1] < un_lsb_en_porcentaje


def test_espectro_cae_en_bins_enteros_sin_fugas():
    # Un período repetido dos veces sigue siendo periódico con 256 muestras: su FFT de 512 no tiene energía
    # en los bins impares. Si la tabla no fuera periódica, esa energía aparecería como fuga
    amplitudes = [1.0, 0.5, 0.0, 0.25] + [0.0] * 26
    repetida = np.tile(build_table(amplitudes), 2)
    magnitudes = np.abs(np.fft.rfft(repetida.astype(np.float64))) / 256
    assert np.max(magnitudes[1::2]) < 1.0


def test_armonicos_son_2_a_30_ni_uno_mas():
    # Hay 30 armónicos (1 a 30): el espectro tiene 30 valores, el 30 aparece y un armónico 31 no se puede sintetizar
    espectro = spectrum(build_table([0.0] * 29 + [1.0]))
    assert len(espectro) == 30
    assert espectro[29] > 0
    with pytest.raises(ValueError):
        build_table([0.0] * 30 + [1.0])


def test_frecuencia_de_cada_armonico_es_k_por_la_nota():
    # Las ocho notas de la tabla de la spec 3.11: el armónico k de cada una está en k × su frecuencia
    notas = [("Do4", 261.63), ("Re4", 293.66), ("Mi4", 329.63), ("Fa4", 349.23),
             ("Sol4", 392.00), ("La4", 440.00), ("Si4", 493.88), ("Do5", 523.25)]
    for _, hz in notas:
        frecuencias = harmonic_frequencies(hz)
        assert len(frecuencias) == 30
        assert np.allclose(frecuencias, [k * hz for k in range(1, 31)])
    # Chequeo puntual: en La4 el armónico 30 cae en 13200 Hz
    assert harmonic_frequencies(440.00)[29] == pytest.approx(13200.0)


def test_cambiar_nota_cambia_solo_los_hz():
    # Al cambiar de nota cambian las frecuencias; las amplitudes y la tabla quedan iguales
    editor = Editor()
    editor.set_amplitude(1, 1.0)
    editor.set_amplitude(2, 0.5)
    amplitudes_antes = list(editor.amplitudes)
    tabla_antes = editor.table()
    for nota, hz in [("Do4", 261.63), ("La4", 440.00), ("Do5", 523.25)]:
        editor.set_note(nota)
        assert editor.amplitudes == amplitudes_antes
        assert np.array_equal(editor.table(), tabla_antes)
        assert editor.frequencies()[0] == pytest.approx(hz)
        assert editor.frequencies()[1] == pytest.approx(2 * hz)


def test_normalizacion_pone_el_pico_absoluto_en_32767():
    # En cualquier combinación, la muestra de mayor valor absoluto vale ±32767 (spec 3.3)
    combinaciones = [[1.0, 0.5], [0.3, 0.0, 0.7, 0.0, 0.0, 0.2], [0.0] * 9 + [1.0]]
    for amplitudes in combinaciones:
        tabla = build_table(amplitudes).astype(np.int64)
        assert np.max(np.abs(tabla)) == 32767


def test_cuantizacion_queda_dentro_de_int16():
    # Con 200 combinaciones al azar, la tabla es int16 y ninguna muestra sale de ±32767 (el -32768 no existe acá)
    rng = np.random.default_rng(7)
    for _ in range(200):
        amplitudes = rng.uniform(0, 1, size=30) * (rng.uniform(size=30) < 0.5)
        amplitudes[0] = rng.uniform(0.01, 1)
        tabla = build_table(amplitudes.tolist())
        assert tabla.dtype == np.int16
        assert tabla.min() >= -32767
        assert tabla.max() <= 32767


def test_senoide_y_cuadrada_coinciden_con_tables_c():
    # La senoide y la cuadrada (impares con 1/k) coinciden con las del firmware, con tolerancia de 1 LSB (spec 3.3)
    senoide = build_table([1.0] + [0.0] * 29).astype(np.int64)
    cuadrada = build_table([1 / k if k % 2 == 1 else 0.0 for k in range(1, 31)]).astype(np.int64)
    assert np.max(np.abs(senoide - leer_tabla_firmware("sineTable"))) <= 1
    assert np.max(np.abs(cuadrada - leer_tabla_firmware("squareTable"))) <= 1


def test_espectro_se_calcula_sobre_la_tabla_cuantizada():
    # El espectro sale de la tabla int16 que va a sonar, no de los controles: tiene que ser idéntico a spectrum(tabla)
    editor = Editor()
    for k, amplitud in [(1, 1.0), (2, 0.5), (3, 0.25)]:
        editor.set_amplitude(k, amplitud)
    assert np.array_equal(editor.spectrum(), spectrum(editor.table()))


def test_exporta_256_enteros_uno_por_linea_arrancando_en_cero(tmp_path):
    # El archivo es texto con 256 enteros, uno por línea, y la primera muestra es cero (spec 3.3)
    archivo = tmp_path / "onda.txt"
    Editor().export(archivo)
    lineas = archivo.read_text().splitlines()
    assert len(lineas) == 256
    muestras = [int(linea) for linea in lineas]
    assert muestras[0] == 0


def test_exporta_pico_32767_y_ninguna_muestra_fuera_de_rango(tmp_path):
    # Leído el archivo exportado: el mayor valor absoluto vale 32767 y ninguna muestra pasa de ±32767
    archivo = tmp_path / "onda.txt"
    editor = Editor()
    editor.set_amplitude(1, 1.0)
    editor.set_amplitude(2, 0.5)
    editor.export(archivo)
    muestras = np.array([int(linea) for linea in archivo.read_text().splitlines()])
    assert np.max(np.abs(muestras)) == 32767
    assert np.all(np.abs(muestras) <= 32767)


def test_onda_nula_no_exporta_y_devuelve_el_motivo(tmp_path):
    # Con todas las amplitudes en cero no se escribe archivo, y el error dice por qué (spec 3.3)
    archivo = tmp_path / "onda.txt"
    editor = Editor()
    editor.set_amplitude(1, 0.0)
    with pytest.raises(ValueError, match="amplitud"):
        editor.export(archivo)
    assert not archivo.exists()


def test_estado_inicial_es_fundamental_100_resto_0():
    # Al abrir: fundamental al 100 %, el resto en 0 (senoide) y nota La4 (spec 3.3)
    editor = Editor()
    assert editor.note == "La4"
    assert editor.amplitudes == [1.0] + [0.0] * 29
    assert editor.frequencies()[0] == pytest.approx(440.00)
