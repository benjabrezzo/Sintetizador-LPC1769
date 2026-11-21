"""Instrumentos precargados del editor (story 12, spec 3.3). Datos puros: no importa PySide6.
"""

# Spec 3.3, tabla de instrumentos: amplitudes de los armónicos 1 a 30, en % enteros de la fundamental
AMPLITUDES = {
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

# Spec 3.3: mensaje de ajuste de la ADSR, una línea por pote
ADSR_MESSAGES = {
    "Guitarra": (
        "Attack: al mínimo (≈ 2 ms)\n"
        "Decay: tres cuartos del recorrido (≈ 1500 ms)\n"
        "Sustain: al mínimo (nivel 0 %)\n"
        "Release: un décimo del recorrido (≈ 200 ms)"
    ),
    "Piano": (
        "Attack: al mínimo (≈ 2 ms)\n"
        "Decay: al máximo (2000 ms)\n"
        "Sustain: al mínimo (nivel 0 %)\n"
        "Release: un sexto del recorrido (≈ 300 ms)"
    ),
    "Órgano": (
        "Attack: al mínimo (≈ 2 ms)\n"
        "Decay: en cualquier posición (con Sustain al 100 % no suena distinto)\n"
        "Sustain: al máximo (nivel 100 %)\n"
        "Release: al mínimo (≈ 2 ms)"
    ),
    "Flauta": (
        "Attack: un poco más del mínimo (≈ 100 ms)\n"
        "Decay: un décimo del recorrido (≈ 200 ms)\n"
        "Sustain: cuatro quintos del recorrido (nivel 80 %)\n"
        "Release: un décimo del recorrido (≈ 150 ms)"
    ),
}
