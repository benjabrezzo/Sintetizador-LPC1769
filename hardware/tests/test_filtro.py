import sys
from pathlib import Path

# El módulo vive en hardware/tools y no es un paquete: se agrega al path.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

import etapa
import filtro

R = 8200.0
C1 = 2.2e-9
C2 = 1e-9
F_IMAGEN = 32000.0

# Componentes montados en el filtro, medidos con el multímetro (story 4).
R1_MEDIDA = 8040.0
R2_MEDIDA = 8030.0
C1_MEDIDO = 2.34e-9
C2_MEDIDO = 1.034e-9


def test_filtro_nominal_atenua_15_5_dB_a_32kHz():
    # Esperado de la story 4, criterio 1: diseño −15,5 dB.
    assert abs(filtro.attenuation_db(R, R, C1, C2, F_IMAGEN) - (-15.5)) <= 0.05


def test_filtro_peor_caso_5pct_10pct_atenua_12_9_dB():
    # Story 4, criterio 1: resistencias al 5 % y capacitores al 10 %,
    # en el sentido que sube fc (R y C por debajo): ≈ −12,9 dB.
    r = R * 0.95
    c1 = C1 * 0.90
    c2 = C2 * 0.90
    assert abs(filtro.attenuation_db(r, r, c1, c2, F_IMAGEN) - (-12.9)) <= 0.05


def test_filtro_montado_atenua_al_menos_12_dB_a_32kHz():
    # Story 4, criterio 1: con los valores verificados de los componentes
    # montados, al menos 12 dB de atenuación a 32 kHz.
    atenuacion = filtro.attenuation_db(R1_MEDIDA, R2_MEDIDA, C1_MEDIDO, C2_MEDIDO, F_IMAGEN)
    assert atenuacion <= -12.0


def test_salida_16_ohm_al_maximo_esta_entre_0_12_y_0_21_Vrms():
    # Story 4, criterio 2: rango 0,12 a 0,21 Vrms por canal (diseño ≈ 0,17),
    # con la senoide de escala completa de 3,36 a 3,40 Vpp del DAC.
    for vpp_dac in (3.36, 3.40):
        vrms = etapa.output_vrms(
            vpp_dac,
            r_series=150e3,
            r_pot=10e3,
            r_in_lm386=50e3,
            gain=20,
            r_out=100.0,
            r_load=16.0,
        )
        assert 0.12 <= vrms <= 0.21
