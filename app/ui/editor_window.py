"""Ventana del editor de armónicos (story 11, spec 3.3).

Única parte de la app que importa PySide6: la lógica vive en core/armonicos.py.
"""
import sys

import numpy as np
import pyqtgraph as pg
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication, QComboBox, QDial, QFileDialog, QHBoxLayout, QLabel, QMessageBox, QPushButton, QSlider,
    QVBoxLayout, QWidget,
)

from core.armonicos import FULL_SCALE, MAX_HARMONIC, NOTES, WAVE_SIZE, Editor
from core.board_link import list_serial_ports, send_eq_to_port, send_to_port
from core.equalizer import MAX_GAIN_DB, MIN_GAIN_DB, eq_coefficients
from core.instrumentos import ADSR_MESSAGES, AMPLITUDES


class EditorWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.editor = Editor()

        self.selector_nota = QComboBox()
        self.selector_nota.addItems(list(NOTES))
        self.selector_nota.setCurrentText(self.editor.note)
        self.boton_exportar = QPushButton("Exportar")
        # Envío a la placa (story 13): el puerto se elige o se escribe a mano, y el mensaje muestra el resultado
        self.port_selector = QComboBox()
        self.port_selector.setEditable(True)
        self.port_selector.addItems(list_serial_ports())
        self.send_button = QPushButton("Enviar a la placa")
        self.status_label = QLabel()
        # Un botón por instrumento: cargar es una acción, así que no hay un "instrumento actual" que pueda quedar viejo
        self.instrument_buttons = {nombre: QPushButton(nombre) for nombre in AMPLITUDES}
        # Zona fija con el mensaje de ajuste de la ADSR: vacía hasta elegir un instrumento
        self.adsr_message_label = QLabel()
        self.adsr_message_label.setWordWrap(True)

        # Una fila por armónico: nombre, frecuencia en Hz y slider de 0 a 100 %.
        # El índice k-1 de sliders y etiquetas_hz corresponde al armónico k
        self.sliders = []
        self.etiquetas_hz = []
        filas = QVBoxLayout()
        for k in range(1, MAX_HARMONIC + 1):
            slider = QSlider(Qt.Horizontal)
            slider.setRange(0, 100)
            slider.setValue(round(self.editor.amplitudes[k - 1] * 100))
            etiqueta = QLabel()
            self.sliders.append(slider)
            self.etiquetas_hz.append(etiqueta)
            fila = QHBoxLayout()
            fila.addWidget(QLabel(f"Armónico {k}"))
            fila.addWidget(etiqueta)
            fila.addWidget(slider)
            filas.addLayout(fila)

        # Izquierda: nota y armónicos. Derecha: la onda y su espectro
        izquierda = QVBoxLayout()
        izquierda.addWidget(self.selector_nota)
        fila_instrumentos = QHBoxLayout()
        for boton in self.instrument_buttons.values():
            fila_instrumentos.addWidget(boton)
        izquierda.addLayout(fila_instrumentos)
        izquierda.addWidget(self.adsr_message_label)
        izquierda.addLayout(filas)
        izquierda.addWidget(self.boton_exportar)
        fila_envio = QHBoxLayout()
        fila_envio.addWidget(self.port_selector)
        fila_envio.addWidget(self.send_button)
        izquierda.addLayout(fila_envio)
        izquierda.addWidget(self.status_label)

        self.grafico_onda = pg.PlotWidget(background="w", title="Un período")
        self.grafico_onda.setYRange(-FULL_SCALE, FULL_SCALE)
        self.curva_onda = self.grafico_onda.plot(pen=pg.mkPen("k", width=2))

        self.grafico_espectro = pg.PlotWidget(background="w", title="Espectro (% del pico de escala)")
        self.grafico_espectro.setLabel("bottom", "Frecuencia", units="Hz")
        self.grafico_espectro.getAxis("bottom").enableAutoSIPrefix(False)  # en Hz, no en kHz (spec 3.3)
        self.grafico_espectro.setYRange(0, 100)
        self.curva_espectro = self.grafico_espectro.plot(pen=pg.mkPen("b", width=2), connect="pairs")

        # Ecualizador (story 14, spec 3.5): una perilla por banda, de −12 a +6 dB, que arrancan en 0 como la placa. Con
        # el seguimiento apagado, valueChanged llega al soltar la perilla: un envío por gesto, no uno por cada paso
        self.eq_dials = []
        self.eq_value_labels = []
        fila_ecualizador = QHBoxLayout()
        for banda, nombre in enumerate(("Graves (200 Hz)", "Medios (1 kHz)", "Agudos (4 kHz)")):
            dial = QDial()
            dial.setRange(int(MIN_GAIN_DB), int(MAX_GAIN_DB))
            dial.setValue(0)
            dial.setNotchesVisible(True)
            dial.setTracking(False)
            etiqueta_valor = QLabel(self._eq_text(0))
            etiqueta_valor.setAlignment(Qt.AlignCenter)
            etiqueta_nombre = QLabel(nombre)
            etiqueta_nombre.setAlignment(Qt.AlignCenter)
            columna = QVBoxLayout()
            columna.addWidget(etiqueta_nombre)
            columna.addWidget(dial)
            columna.addWidget(etiqueta_valor)
            fila_ecualizador.addLayout(columna)
            dial.sliderMoved.connect(lambda valor, banda=banda: self._show_eq_gain(banda, valor))
            dial.valueChanged.connect(lambda valor, banda=banda: self._on_eq_dial_changed(banda, valor))
            self.eq_dials.append(dial)
            self.eq_value_labels.append(etiqueta_valor)

        derecha = QVBoxLayout()
        derecha.addWidget(self.grafico_onda)
        derecha.addWidget(self.grafico_espectro)
        derecha.addLayout(fila_ecualizador)

        raiz = QHBoxLayout(self)
        raiz.addLayout(izquierda)
        raiz.addLayout(derecha)

        for k, slider in enumerate(self.sliders, start=1):
            slider.valueChanged.connect(lambda valor, k=k: self._al_mover_amplitud(k, valor))
        self.selector_nota.currentTextChanged.connect(self._al_cambiar_nota)
        self.boton_exportar.clicked.connect(self._exportar)
        self.send_button.clicked.connect(self._send_to_board)
        for nombre, boton in self.instrument_buttons.items():
            boton.clicked.connect(lambda _=False, nombre=nombre: self._load_instrument(nombre))
        self._actualizar_etiquetas()
        self._refrescar()

    def _exportar(self):
        ruta, _ = QFileDialog.getSaveFileName(self, "Exportar tabla", "onda.txt", "Texto (*.txt)")
        if not ruta:
            return
        try:
            self.editor.export(ruta)
        except ValueError as motivo:
            QMessageBox.warning(self, "No se exporta", str(motivo))

    def _send_to_board(self):
        """Envía la tabla del editor al puerto elegido y muestra si la placa confirmó (story 13, spec 3.14)."""
        if send_to_port(self.port_selector.currentText(), self.editor.table()):
            self.status_label.setText("Onda enviada: la placa confirmó la recepción")
        else:
            self.status_label.setText("La placa no confirmó la recepción")

    @staticmethod
    def _eq_text(gain_db):
        return f"{gain_db:+d} dB" if gain_db else "0 dB"

    def _show_eq_gain(self, band, gain_db):
        """Muestra la ganancia de una banda mientras se arrastra su perilla, sin enviar nada."""
        self.eq_value_labels[band].setText(self._eq_text(gain_db))

    def _on_eq_dial_changed(self, band, gain_db):
        """Una perilla soltó su valor: se muestra y se envían a la placa las tres bandas (story 14, spec 3.14)."""
        self._show_eq_gain(band, gain_db)
        coefficients = eq_coefficients(*(dial.value() for dial in self.eq_dials))
        if send_eq_to_port(self.port_selector.currentText(), coefficients):
            self.status_label.setText("Ecualizador enviado: la placa confirmó la recepción")
        else:
            self.status_label.setText("La placa no confirmó el ecualizador")

    def _al_mover_amplitud(self, k, valor):
        self.editor.set_amplitude(k, valor / 100)
        self._refrescar()

    def _al_cambiar_nota(self, nota):
        self.editor.set_note(nota)
        self._actualizar_etiquetas()
        self._refrescar()

    def _load_instrument(self, nombre):
        """Carga las 30 amplitudes del instrumento y redibuja una sola vez (spec 3.3)."""
        self.editor.load_instrument(nombre)
        for slider, amplitud in zip(self.sliders, self.editor.amplitudes):
            # Sin bloquear, cada valueChanged redibujaría la onda: 30 veces por carga
            slider.blockSignals(True)
            slider.setValue(round(amplitud * 100))
            slider.blockSignals(False)
        self.adsr_message_label.setText(ADSR_MESSAGES[nombre])
        self._refrescar()

    def _refrescar(self):
        """Redibuja la onda y el espectro con lo que devuelve el editor."""
        try:
            tabla = self.editor.table()
        except ValueError:
            # Onda nula: no hay período que dibujar, así que los gráficos quedan vacíos
            self.curva_onda.setData([], [])
            self.curva_espectro.setData([], [])
            return
        self.curva_onda.setData(np.arange(WAVE_SIZE), tabla)
        espectro = self.editor.spectrum()
        frecuencias = self.editor.frequencies()
        # Cada armónico es una línea vertical de 0 a su valor: pares de puntos (x, 0) y (x, valor)
        valores = np.column_stack([np.zeros_like(espectro), espectro]).ravel()
        self.curva_espectro.setData(np.repeat(frecuencias, 2), valores)

    def _actualizar_etiquetas(self):
        for etiqueta, hz in zip(self.etiquetas_hz, self.editor.frequencies()):
            etiqueta.setText(f"{hz:.2f} Hz")


def aplicar_estilo_clasico(app):
    """Estilo Windows de Qt con su paleta, forzados para que no los pise el tema claro u oscuro de Plasma (spec 3.3)."""
    app.setStyle("Windows")
    app.setPalette(app.style().standardPalette())


def main():
    app = QApplication(sys.argv)
    aplicar_estilo_clasico(app)
    ventana = EditorWindow()
    ventana.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
