"""Smoke test de la ventana del editor (story 11). Corre offscreen: no abre ventanas reales.

Lo visual (fuentes, estilo clásico, capturas) se verifica a mano; acá solo se comprueba que la ventana
arranca con su estado y que los controles se conectan con el editor.
"""
import gc
import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import numpy as np
import pytest
from PySide6.QtWidgets import QApplication, QFileDialog, QMessageBox

from core import instrumentos
from core.equalizer import eq_coefficients
from ui.editor_window import EditorWindow


@pytest.fixture(scope="module")
def qapp():
    # Al terminar el módulo se cierran y destruyen las ventanas con la QApplication todavía viva. Si quedan vivas
    # hasta el cierre del intérprete, Qt aborta con "pure virtual method called" (exit 134 o 139).
    app = QApplication.instance() or QApplication([])
    yield app
    for ventana in app.topLevelWidgets():
        ventana.close()
        ventana.deleteLater()
    app.processEvents()
    gc.collect()


def test_ventana_arranca_en_La4_con_30_controles(qapp):
    ventana = EditorWindow()
    assert ventana.selector_nota.count() == 8
    assert ventana.selector_nota.currentText() == "La4"
    assert len(ventana.sliders) == 30
    assert ventana.sliders[0].value() == 100
    assert all(s.value() == 0 for s in ventana.sliders[1:])


def test_cada_control_muestra_su_frecuencia_en_hz_y_cambia_con_la_nota(qapp):
    ventana = EditorWindow()
    ventana.selector_nota.setCurrentText("Do4")
    textos = [etiqueta.text() for etiqueta in ventana.etiquetas_hz]
    assert len(textos) == 30
    assert "523.26" in textos[1]      # armónico 2 de Do4: 2 × 261,63 Hz
    assert "7848.90" in textos[29]    # armónico 30 de Do4: 30 × 261,63 Hz
    ventana.selector_nota.setCurrentText("La4")
    assert "880.00" in ventana.etiquetas_hz[1].text()


def test_mover_una_amplitud_redibuja_la_onda_y_el_espectro(qapp):
    ventana = EditorWindow()
    onda_inicial = ventana.curva_onda.getData()[1].copy()
    ventana.sliders[1].setValue(50)  # segundo armónico al 50 %
    _, onda = ventana.curva_onda.getData()
    assert not np.array_equal(onda, onda_inicial)
    assert np.array_equal(onda, ventana.editor.table())
    frecuencias, valores = ventana.curva_espectro.getData()
    assert np.allclose(frecuencias[::2], np.arange(1, 31) * 440.00)  # La4: k × 440 Hz
    assert valores[3] / valores[1] == pytest.approx(0.5, rel=1e-3)   # el 2 mide la mitad del 1


def test_boton_exportar_escribe_el_archivo_y_avisa_si_la_onda_es_nula(qapp, tmp_path, monkeypatch, capsys):
    ventana = EditorWindow()
    destino = tmp_path / "onda.txt"
    monkeypatch.setattr(QFileDialog, "getSaveFileName", lambda *args, **kwargs: (str(destino), "Texto (*.txt)"))
    ventana.boton_exportar.click()
    assert len(destino.read_text().splitlines()) == 256

    # Onda nula: los gráficos quedan vacíos sin errores en consola, no se escribe archivo y el aviso dice por qué
    ventana.sliders[0].setValue(0)
    _, onda = ventana.curva_onda.getData()
    _, espectro = ventana.curva_espectro.getData()
    assert onda is None or len(onda) == 0
    assert espectro is None or len(espectro) == 0
    assert "Traceback" not in capsys.readouterr().err

    nula = tmp_path / "nula.txt"
    monkeypatch.setattr(QFileDialog, "getSaveFileName", lambda *args, **kwargs: (str(nula), "Texto (*.txt)"))
    avisos = []
    monkeypatch.setattr(QMessageBox, "warning", lambda *args, **kwargs: avisos.append(args))
    ventana.boton_exportar.click()
    assert not nula.exists()
    assert len(avisos) == 1


@pytest.mark.parametrize("nombre", list(instrumentos.AMPLITUDES))
def test_elegir_un_instrumento_mueve_los_30_controles_y_redibuja(qapp, nombre):
    ventana = EditorWindow()
    ventana.instrument_buttons[nombre].click()
    assert [slider.value() for slider in ventana.sliders] == instrumentos.AMPLITUDES[nombre]
    _, onda = ventana.curva_onda.getData()
    assert np.array_equal(onda, ventana.editor.table())
    _, valores = ventana.curva_espectro.getData()  # cada armónico: (x, 0) y (x, valor)
    proporciones = valores[1::2] / valores[1]
    assert np.allclose(proporciones, np.array(instrumentos.AMPLITUDES[nombre]) / 100, atol=0.001)


def test_elegir_un_instrumento_muestra_su_mensaje_y_otro_lo_cambia(qapp):
    ventana = EditorWindow()
    ventana.instrument_buttons["Guitarra"].click()
    assert ventana.adsr_message_label.text() == instrumentos.ADSR_MESSAGES["Guitarra"]
    ventana.instrument_buttons["Piano"].click()
    assert ventana.adsr_message_label.text() == instrumentos.ADSR_MESSAGES["Piano"]


def test_cargar_un_instrumento_en_la_ventana_no_cambia_la_nota(qapp):
    ventana = EditorWindow()
    ventana.selector_nota.setCurrentText("Do4")
    ventana.instrument_buttons["Piano"].click()
    assert ventana.selector_nota.currentText() == "Do4"
    assert ventana.editor.note == "Do4"



def test_ventana_tiene_selector_de_puerto_y_boton_enviar(qapp, monkeypatch):
    """Story 13: el selector lista los puertos serie y el botón "Enviar a la placa" arranca sin ningún mensaje."""
    monkeypatch.setattr("ui.editor_window.list_serial_ports", lambda: ["/dev/ttyUSB0", "/dev/ttyUSB1"])
    ventana = EditorWindow()
    assert [ventana.port_selector.itemText(i) for i in range(ventana.port_selector.count())] == [
        "/dev/ttyUSB0", "/dev/ttyUSB1"]
    assert ventana.send_button.text() == "Enviar a la placa"
    assert ventana.status_label.text() == ""


def test_boton_enviar_manda_la_tabla_del_editor(qapp, monkeypatch):
    """Story 13: el botón manda al puerto elegido la tabla que el editor tiene ahora, y el mensaje dice el resultado."""
    monkeypatch.setattr("ui.editor_window.list_serial_ports", lambda: ["/dev/ttyUSB0"])
    enviados = []

    def envio_que_confirma(port_name, table):
        enviados.append((port_name, table.copy()))
        return True

    monkeypatch.setattr("ui.editor_window.send_to_port", envio_que_confirma)
    ventana = EditorWindow()
    ventana.sliders[1].setValue(50)      # distinta de la senoide inicial: el test no pasa con cualquier tabla
    ventana.send_button.click()

    assert len(enviados) == 1
    puerto, tabla = enviados[0]
    assert puerto == "/dev/ttyUSB0"
    assert np.array_equal(tabla, ventana.editor.table())
    assert ventana.status_label.text().startswith("Onda enviada")

    # La placa no confirma: el mensaje lo dice
    monkeypatch.setattr("ui.editor_window.send_to_port", lambda port_name, table: False)
    ventana.send_button.click()
    assert "no confirmó" in ventana.status_label.text()


# Story 14: tres perillas del ecualizador (graves, medios y agudos)


def test_ventana_tiene_tres_perillas_de_ecualizador_de_menos_12_a_mas_6_db_en_0(qapp):
    ventana = EditorWindow()

    assert len(ventana.eq_dials) == 3
    for dial, label in zip(ventana.eq_dials, ventana.eq_value_labels):
        assert (dial.minimum(), dial.maximum()) == (-12, 6)
        assert dial.value() == 0                  # la placa arranca plana (spec 3.5)
        assert label.text() == "0 dB"


def test_soltar_una_perilla_envia_las_tres_bandas_una_sola_vez(qapp, monkeypatch):
    monkeypatch.setattr("ui.editor_window.list_serial_ports", lambda: ["/dev/ttyUSB0"])
    enviados = []

    def envio_que_confirma(port_name, coefficients):
        enviados.append((port_name, coefficients.copy()))
        return True

    monkeypatch.setattr("ui.editor_window.send_eq_to_port", envio_que_confirma)
    ventana = EditorWindow()
    medios = ventana.eq_dials[1]

    # Arrastrar la perilla no envía: el valor se actualiza al soltar (un envío por gesto)
    medios.setSliderDown(True)
    medios.setSliderPosition(5)
    medios.setSliderPosition(-8)
    medios.setSliderPosition(5)
    assert enviados == []
    medios.setSliderDown(False)

    assert len(enviados) == 1
    puerto, coefs = enviados[0]
    assert puerto == "/dev/ttyUSB0"
    assert np.array_equal(coefs, eq_coefficients(0, 5, 0))
    assert ventana.eq_value_labels[1].text() == "+5 dB"
    assert ventana.status_label.text().startswith("Ecualizador enviado")

    # El siguiente gesto, en otra perilla, lleva las tres bandas: la placa se resincroniza con lo que muestra la ventana
    graves = ventana.eq_dials[0]
    graves.setSliderDown(True)
    graves.setSliderPosition(-12)
    graves.setSliderDown(False)
    assert len(enviados) == 2
    assert np.array_equal(enviados[1][1], eq_coefficients(-12, 5, 0))


def test_si_la_placa_no_confirma_el_ecualizador_el_mensaje_lo_dice(qapp, monkeypatch):
    monkeypatch.setattr("ui.editor_window.list_serial_ports", lambda: ["/dev/ttyUSB0"])
    monkeypatch.setattr("ui.editor_window.send_eq_to_port", lambda port_name, coefficients: False)
    ventana = EditorWindow()

    ventana.eq_dials[2].setValue(6)

    assert ventana.status_label.text() == "La placa no confirmó el ecualizador"


def test_cambiar_una_perilla_sin_arrastrar_la_muestra_y_la_envia(qapp, monkeypatch):
    """Con la rueda o el teclado no hay arrastre (ni sliderMoved): valueChanged llega solo y alcanza para mostrar y enviar."""
    monkeypatch.setattr("ui.editor_window.list_serial_ports", lambda: ["/dev/ttyUSB0"])
    enviados = []

    def envio_que_confirma(port_name, coefficients):
        enviados.append(coefficients.copy())
        return True

    monkeypatch.setattr("ui.editor_window.send_eq_to_port", envio_que_confirma)
    ventana = EditorWindow()

    ventana.eq_dials[2].setValue(-3)

    assert ventana.eq_value_labels[2].text() == "-3 dB"
    assert len(enviados) == 1
    assert np.array_equal(enviados[0], eq_coefficients(0, 0, -3))
