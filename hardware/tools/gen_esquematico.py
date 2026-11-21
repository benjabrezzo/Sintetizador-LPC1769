#!/usr/bin/env python3
"""Genera hardware/kicad/sintetizador.kicad_sch (esquemático completo del sintetizador).

Es el mismo circuito que el README (controles, LPC1769, UART y etapa analógica de audio) y que
hardware/etapa-audio.md (referencias R1..R6, C1..C7, P1, U1, U2, J1 y nodos X, F, P5 y S).

Uso:  python3 hardware/tools/gen_esquematico.py
Después se abre hardware/kicad/sintetizador.kicad_pro con KiCad 9 o 10 (el archivo se puede seguir
editando a mano; este script solo sirve para regenerarlo desde cero).

Los símbolos salen de las librerías oficiales de KiCad (/usr/share/kicad/symbols en Fedora,
/usr/share/kicad/symbols también en Arch/CachyOS); se copian dentro del esquemático, así que el
archivo resultante no depende de las librerías. Los únicos símbolos propios son la placa LPCXpresso
y el adaptador USB-TTL.
"""
import json
import math
import os
import random
import sys

SYMDIR = os.environ.get("KICAD_SYMBOL_DIR", "/usr/share/kicad/symbols")
OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "kicad")
PROJECT = "sintetizador"

rng = random.Random(1769)


def uid():
    return "%08x-%04x-%04x-%04x-%012x" % (rng.getrandbits(32), rng.getrandbits(16),
                                          0x4000 | rng.getrandbits(12), 0x8000 | rng.getrandbits(14),
                                          rng.getrandbits(48))


def fmt(v):
    s = ("%.4f" % v).rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


# ---------------------------------------------------------------- lector de librerías de KiCad
def parse(s):
    i, n, stack = 0, len(s), [[]]
    while i < n:
        c = s[i]
        if c == "(":
            stack.append([])
            i += 1
        elif c == ")":
            x = stack.pop()
            stack[-1].append(x)
            i += 1
        elif c == '"':
            j = i + 1
            while s[j] != '"':
                j += 2 if s[j] == "\\" else 1
            stack[-1].append(("S", s[i + 1:j]))
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and not s[j].isspace() and s[j] not in "()":
                j += 1
            stack[-1].append(s[i:j])
            i = j
    return stack[0][0]


def find(node, name):
    return [c for c in node if isinstance(c, list) and c and c[0] == name]


def emit(node, depth=0):
    if isinstance(node, tuple):
        return '"' + node[1] + '"'
    if not isinstance(node, list):
        return node
    if not any(isinstance(c, list) for c in node):
        return "(" + " ".join(emit(c) for c in node) + ")"
    head = [emit(c) for c in node if not isinstance(c, list)]
    out = "(" + " ".join(head)
    for c in node:
        if isinstance(c, list):
            out += "\n" + "\t" * (depth + 1) + emit(c, depth + 1)
    return out + "\n" + "\t" * depth + ")"


_libs = {}


def lib_symbol(libname, name, alias=None):
    """Devuelve (texto para lib_symbols, pines) de un símbolo de las librerías de KiCad.
    pines: {número: (unidad, x, y, tipo)} con x, y en coordenadas de biblioteca (y hacia arriba)."""
    if libname not in _libs:
        _libs[libname] = parse(open(os.path.join(SYMDIR, libname + ".kicad_sym")).read())
    sym = [s for s in find(_libs[libname], "symbol") if s[1][1] == name][0]
    ext = find(sym, "extends")
    if ext:  # LM358 extiende a LM2904: se usa el símbolo base con otro nombre
        base = ext[0][1][1]
        return lib_symbol_renamed(libname, base, name, find(sym, "property"))
    return lib_symbol_renamed(libname, name, name)


def lib_symbol_renamed(libname, src, dst, override=None):
    if libname not in _libs:
        _libs[libname] = parse(open(os.path.join(SYMDIR, libname + ".kicad_sym")).read())
    sym = [s for s in find(_libs[libname], "symbol") if s[1][1] == src][0]
    for ov in override or []:  # propiedades propias del símbolo derivado (Value, Datasheet...)
        for i, c in enumerate(sym):
            if isinstance(c, list) and c and c[0] == "property" and c[1][1] == ov[1][1]:
                sym[i] = ov
    pins = {}
    for sub in find(sym, "symbol"):
        unit = int(sub[1][1].rsplit("_", 2)[1])
        for p in find(sub, "pin"):
            at = find(p, "at")[0]
            num = find(p, "number")[0][1][1]
            pins[num] = (unit, float(at[1]), float(at[2]), p[1])
    text = emit(sym).replace(src, dst)
    # los nombres de las sub-unidades llevan el nombre sin prefijo; el principal, con prefijo
    text = text.replace('(symbol "%s"' % dst, '(symbol "%s:%s"' % (libname, dst), 1)
    return text, pins


# ---------------------------------------------------------------- símbolos propios
def box_symbol(libid, ref, value, width, height, pins, name_hide=False):
    """Caja con pines. Origen en la esquina superior izquierda de la caja (coordenadas en pantalla,
    y hacia abajo). pins: [(lado, número, nombre, tipo, pos)] con lado 'L','R','T','B' y pos = distancia
    en mm desde la esquina (en y para L y R, en x para T y B). Longitud de pin: 5,08 mm."""
    L = 5.08
    out = []
    pinmap = {}
    for side, num, name, etype, pos in pins:
        if side == "L":
            sx, sy, ang = -L, pos, 0
        elif side == "R":
            sx, sy, ang = width + L, pos, 180
        elif side == "T":
            sx, sy, ang = pos, -L, 270
        else:
            sx, sy, ang = pos, height + L, 90
        pinmap[num] = (1, sx, -sy, ang)
        out.append('\t\t\t(pin %s line (at %s %s %d) (length %s)\n'
                   '\t\t\t\t(name "%s" (effects (font (size 1.27 1.27))))\n'
                   '\t\t\t\t(number "%s" (effects (font (size 1.27 1.27))))\n\t\t\t)'
                   % (etype, fmt(sx), fmt(-sy), ang, fmt(L), name, num))
    name = libid.split(":")[1]
    text = (
        '\t\t(symbol "%s"\n'
        '\t\t\t(pin_numbers (hide yes))\n'
        '\t\t\t(pin_names (offset 1.016))\n'
        '\t\t\t(exclude_from_sim no) (in_bom yes) (on_board yes)\n'
        '\t\t\t(property "Reference" "%s" (at 0 2.54 0) (effects (font (size 1.27 1.27)) (justify left)))\n'
        '\t\t\t(property "Value" "%s" (at %s %s 0) (effects (font (size 1.27 1.27)) (justify left)))\n'
        '\t\t\t(property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))\n'
        '\t\t\t(property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))\n'
        '\t\t\t(symbol "%s_0_1"\n'
        '\t\t\t\t(rectangle (start 0 0) (end %s %s) (stroke (width 0.254) (type default)) (fill (type background)))\n'
        '\t\t\t)\n'
        '\t\t\t(symbol "%s_1_1"\n%s\n\t\t\t)\n'
        '\t\t)' % (libid, ref, value, fmt(0), fmt(-2.54 - 0), name, fmt(width), fmt(-height), name, "\n".join(out)))
    return text, pinmap


# ---------------------------------------------------------------- esquemático
class Sym:
    def __init__(self, sch, libid, ref, value, x, y, rot, mirror, unit, pins, ref_pos, val_pos,
                 show_value=True, hide_ref=False):
        self.sch, self.libid, self.ref, self.x, self.y = sch, libid, ref, x, y
        self.rot, self.mirror, self.unit, self.pins = rot, mirror, unit, pins
        self.value, self.ref_pos, self.val_pos = value, ref_pos, val_pos
        self.show_value, self.hide_ref = show_value, hide_ref

    def pin(self, num):
        _, px, py, _a = self.pins[num][:4]
        sx, sy = px, -py                       # a coordenadas de pantalla (y hacia abajo)
        if self.mirror == "x":
            sy = -sy
        elif self.mirror == "y":
            sx = -sx
        th = math.radians(self.rot)
        c, s = round(math.cos(th)), round(math.sin(th))
        return (round(self.x + sx * c + sy * s, 4), round(self.y - sx * s + sy * c, 4))


class Sch:
    def __init__(self):
        self.body, self.libs, self.libpins, self.refcount = [], {}, {}, {}
        self.root = uid()
        self.pwr = 0

    def use_lib(self, libid, text, pins):
        if libid not in self.libs:
            self.libs[libid] = text
            self.libpins[libid] = pins

    def add_lib(self, libname, name, alias=None):
        libid = "%s:%s" % (libname, alias or name)
        if libid not in self.libs:
            text, pins = lib_symbol(libname, name) if not alias else lib_symbol_renamed(libname, name, alias)
            self.use_lib(libid, "\t\t" + text.replace("\n", "\n\t\t"), pins)
        return libid

    def place(self, libid, ref, value, x, y, rot=0, mirror=None, unit=1, ref_pos=None, val_pos=None,
              show_value=True, hide_ref=False, justify="left"):
        pins = self.libpins[libid]
        s = Sym(self, libid, ref, value, x, y, rot, mirror, unit, pins, ref_pos, val_pos, show_value, hide_ref)
        up = {n: p for n, p in pins.items() if p[0] in (0, unit)}
        mir = " (mirror %s)" % mirror if mirror else ""
        rp = ref_pos or (x + 2.54, y - 1.27)
        vp = val_pos or (x + 2.54, y + 1.27)
        jf = "" if justify == "center" else " (justify %s)" % justify
        eff = lambda h: "(effects (font (size 1.27 1.27))%s%s)" % (jf, " (hide yes)" if h else "")
        fa = 90 if rot in (90, 270) else 0
        lines = [
            '\t(symbol (lib_id "%s") (at %s %s %d)%s (unit %d)' % (libid, fmt(x), fmt(y), rot, mir, unit),
            '\t\t(exclude_from_sim no) (in_bom %s) (on_board %s) (dnp no)' %
            (("no", "no") if libid.startswith("power:") else ("yes", "yes")),
            '\t\t(uuid "%s")' % uid(),
            '\t\t(property "Reference" "%s" (at %s %s %d) %s)' % (ref, fmt(rp[0]), fmt(rp[1]), fa, eff(hide_ref)),
            '\t\t(property "Value" "%s" (at %s %s %d) %s)' % (value, fmt(vp[0]), fmt(vp[1]), fa, eff(not show_value)),
            '\t\t(property "Footprint" "" (at %s %s 0) (effects (font (size 1.27 1.27)) (hide yes)))' % (fmt(x), fmt(y)),
            '\t\t(property "Datasheet" "" (at %s %s 0) (effects (font (size 1.27 1.27)) (hide yes)))' % (fmt(x), fmt(y)),
        ]
        for n in up:
            lines.append('\t\t(pin "%s" (uuid "%s"))' % (n, uid()))
        lines.append('\t\t(instances (project "%s" (path "/%s" (reference "%s") (unit %d))))' %
                     (PROJECT, self.root, ref, unit))
        lines.append("\t)")
        self.body.append("\n".join(lines))
        return s

    def power(self, name, x, y, value=None, show=True):
        libid = self.add_lib("power", name)
        self.pwr += 1
        return self.place(libid, "#PWR%03d" % self.pwr, value or name, x, y, hide_ref=True,
                          show_value=show, ref_pos=(x, y), val_pos=(x + 1.8, y - 2.0 if name != "GND" else y + 3.5))

    def flag(self, x, y):
        libid = self.add_lib("power", "PWR_FLAG")
        self.pwr += 1
        return self.place(libid, "#FLG%03d" % self.pwr, "PWR_FLAG", x, y, hide_ref=True, show_value=False)

    def flag_left(self, x, y):
        libid = self.add_lib("power", "PWR_FLAG")
        self.pwr += 1
        return self.place(libid, "#FLG%03d" % self.pwr, "PWR_FLAG", x, y, rot=90, hide_ref=True, show_value=False)

    def wire(self, *pts):
        for a, b in zip(pts, pts[1:]):
            self.body.append('\t(wire (pts (xy %s %s) (xy %s %s)) (stroke (width 0) (type default)) (uuid "%s"))' %
                             (fmt(a[0]), fmt(a[1]), fmt(b[0]), fmt(b[1]), uid()))

    def junction(self, p):
        self.body.append('\t(junction (at %s %s) (diameter 0) (color 0 0 0 0) (uuid "%s"))' %
                         (fmt(p[0]), fmt(p[1]), uid()))

    def nc(self, p):
        self.body.append('\t(no_connect (at %s %s) (uuid "%s"))' % (fmt(p[0]), fmt(p[1]), uid()))

    def label(self, text, p, rot=0, justify="left bottom"):
        self.body.append('\t(label "%s" (at %s %s %d) (effects (font (size 1.27 1.27)) (justify %s)) (uuid "%s"))' %
                         (text, fmt(p[0]), fmt(p[1]), rot, justify, uid()))

    def text(self, text, p, size=1.27, bold=False, justify="left", italic=False):
        self.body.append('\t(text "%s" (exclude_from_sim no) (at %s %s 0) (effects (font (size %s %s)%s%s) '
                         '(justify %s)) (uuid "%s"))' %
                         (text.replace("\n", "\\n"), fmt(p[0]), fmt(p[1]), fmt(size), fmt(size),
                          " (bold yes)" if bold else "", " (italic yes)" if italic else "", justify, uid()))

    def frame(self, p1, p2, title=None):
        self.body.append('\t(rectangle (start %s %s) (end %s %s) (stroke (width 0.254) (type dash)) '
                         '(fill (type none)) (uuid "%s"))' % (fmt(p1[0]), fmt(p1[1]), fmt(p2[0]), fmt(p2[1]), uid()))
        if title:
            self.text(title, (p1[0] + 2.54, p1[1] + 3.5), 1.8, bold=True)

    def write(self, path, title_block):
        out = ['(kicad_sch', '\t(version 20250114)', '\t(generator "eeschema")', '\t(generator_version "9.0")',
               '\t(uuid "%s")' % self.root, '\t(paper "A3")', title_block, '\t(lib_symbols']
        out += list(self.libs.values())
        out.append("\t)")
        out += self.body
        out += ['\t(sheet_instances (path "/" (page "1")))', "\t(embedded_fonts no)", ")"]
        open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")


def main():
    S = Sch()
    # ----------------------------------------------------------------- símbolos
    R = S.add_lib("Device", "R")
    C = S.add_lib("Device", "C")
    CP = S.add_lib("Device", "C_Polarized")
    POT = S.add_lib("Device", "R_Potentiometer")
    SW = S.add_lib("Switch", "SW_Push")
    OPA = S.add_lib("Amplifier_Operational", "LM358")   # extiende a LM2904
    AMP = S.add_lib("Amplifier_Audio", "LM386")
    JACK = S.add_lib("Connector_Audio", "AudioJack3")
    CONN2 = S.add_lib("Connector_Generic", "Conn_01x02")
    TP = S.add_lib("Connector", "TestPoint")

    # Placa LPCXpresso LPC1769: solo los pines que se usan. La y de cada pin es la y absoluta en la hoja.
    X0, Y0 = 106.68, 30.48           # esquina superior izquierda de la caja
    W = 40.64
    key_y = [40.64 + 7.62 * i for i in range(9)]
    pot_y = [129.54 + 17.78 * i for i in range(4)]
    tx_y, rx_y, tp_y = 139.7, 147.32, 160.02
    aout_y = 76.2
    din_y = 111.76
    H = 195.58 - Y0
    key_pin = ["P2.0", "P2.1", "P2.2", "P2.3", "P2.4", "P2.5", "P2.6", "P2.7", "P2.11"]
    key_nm = ["P2.0/Do4", "P2.1/Re4", "P2.2/Mi4", "P2.3/Fa4", "P2.4/Sol4", "P2.5/La4", "P2.6/Si4", "P2.7/Do5",
              "P2.11/EINT1"]
    ad_pin = ["P0.23", "P0.24", "P0.25", "P1.30"]
    ad_nm = ["P0.23/AD0.0", "P0.24/AD0.1", "P0.25/AD0.2", "P1.30/AD0.4"]
    pins = []
    for k, n, y in zip(key_pin, key_nm, key_y):
        pins.append(("L", k, n, "bidirectional", y - Y0))
    pins += [("L", "P0.2", "P0.2/TXD0", "output", tx_y - Y0),
             ("L", "P0.3", "P0.3/RXD0", "input", rx_y - Y0),
             ("L", "P2.8", "P2.8/ISR", "output", tp_y - Y0),
             ("R", "P0.26", "P0.26/AOUT", "output", aout_y - Y0),
             ("R", "P0.0", "P0.0/DIN", "output", din_y - Y0)]
    for k, n, y in zip(ad_pin, ad_nm, pot_y):
        pins.append(("R", k, n, "input", y - Y0))
    pins += [("T", "3V3", "3V3", "power_out", W / 2 + 0.0),
             ("B", "GND", "GND", "power_in", W / 2 + 0.0)]
    text, mpins = box_symbol("Sintetizador:LPCXpresso_LPC1769", "U3", "LPCXpresso LPC1769", W, H, pins)
    S.use_lib("Sintetizador:LPCXpresso_LPC1769", text, mpins)
    mcu = S.place("Sintetizador:LPCXpresso_LPC1769", "U3", "LPCXpresso LPC1769", X0, Y0,
                  ref_pos=(X0 + W / 2 - 3.0, Y0 - 9.5), val_pos=(X0 + W / 2 - 3.0, Y0 - 7.0), justify="right")

    # Adaptador USB-TTL: VCC, RX, TX y GND, todos a la derecha
    UX, UY = 38.1, tx_y - 12.7
    text, apins = box_symbol("Sintetizador:USB_TTL_3V3", "J3", "Adaptador USB-TTL 3,3 V", 25.4, 33.02, [
        ("R", "VCC", "VCC", "power_in", tx_y - 7.62 - UY),
        ("R", "RX", "RX", "input", tx_y - UY),
        ("R", "TX", "TX", "output", rx_y - UY),
        ("R", "GND", "GND", "power_in", rx_y + 7.62 - UY)])
    S.use_lib("Sintetizador:USB_TTL_3V3", text, apins)
    ttl = S.place("Sintetizador:USB_TTL_3V3", "J3", "Adaptador USB-TTL 3,3 V", UX, UY,
                  ref_pos=(UX, UY - 3.5), val_pos=(UX, UY - 1.2))

    # ----------------------------------------------------------------- teclas (SW1..SW9)
    note = ["Do4", "Re4", "Mi4", "Fa4", "Sol4", "La4", "Si4", "Do5"]
    sx = 76.2
    prev = None
    for i, y in enumerate(key_y):
        sw = S.place(SW, "SW%d" % (i + 1), note[i] if i < 8 else "Onda", sx, y,
                     ref_pos=(sx - 2.2, y - 3.4), val_pos=(sx + 2.2, y - 3.4))
        S.wire(sw.pin("2"), mcu.pin(key_pin[i]))
        pt = sw.pin("1")
        if prev:
            S.wire(prev, pt)
            S.junction(prev)
        prev = pt
    S.wire(prev, (prev[0], prev[1] + 5.08))
    S.power("GND", prev[0], prev[1] + 5.08, show=False)

    # ----------------------------------------------------------------- UART y punto de medición
    S.wire(ttl.pin("RX"), mcu.pin("P0.2"))
    S.wire(ttl.pin("TX"), mcu.pin("P0.3"))
    S.nc(ttl.pin("VCC"))
    S.power("GND", *ttl.pin("GND"), show=False)
    tp = S.place(TP, "TP1", "ISR de audio", 88.9, tp_y, rot=90, ref_pos=(80, tp_y - 3.0), val_pos=(80, tp_y + 3.0),
                 justify="right")
    S.wire(tp.pin("1"), mcu.pin("P2.8"))
    S.text("TX del adaptador → RXD0 (P0.3)\nRX del adaptador → TXD0 (P0.2)\nVCC sin conectar: cada lado se\nalimenta por su propio USB",
           (33.02, tp_y + 5.5), 1.27)

    # ----------------------------------------------------------------- alimentación de la placa
    S.power("+3.3V", *mcu.pin("3V3"), value="+3V3")
    S.power("GND", *mcu.pin("GND"), show=False)

    # ----------------------------------------------------------------- potenciómetros (RV1..RV4), a la derecha
    adsr = ["Attack", "Decay", "Sustain", "Release"]
    AX = mcu.pin("P0.26")[0]
    potx = AX + 15.24
    for i, y in enumerate(pot_y):
        pot = S.place(POT, "RV%d" % (i + 1), "10k · %s" % adsr[i], potx, y, rot=180,
                      ref_pos=(potx + 3.0, y - 1.27), val_pos=(potx + 3.0, y + 1.27), justify="right")
        S.power("+3.3V", *pot.pin("3"), value="+3V3")
        S.power("GND", *pot.pin("1"), show=False)
        S.wire(pot.pin("2"), mcu.pin(ad_pin[i]))

    # ----------------------------------------------------------------- etapa de audio
    Y = aout_y
    # filtro de reconstrucción (Sallen-Key)
    r1 = S.place(R, "R1", "8,2k", AX + 15.24, Y, rot=90, ref_pos=(AX + 15.24, Y - 3.5), val_pos=(AX + 15.24, Y + 3.5),
                 justify="center")
    S.wire((AX, Y), r1.pin("1"))
    xnode = AX + 25.4
    S.wire(r1.pin("2"), (xnode, Y))
    r2 = S.place(R, "R2", "8,2k", AX + 33.02, Y, rot=90, ref_pos=(AX + 33.02, Y - 5.6),
                 val_pos=(AX + 33.02, Y - 3.5), justify="center")
    S.wire((xnode, Y), r2.pin("1"))
    S.label("X", (xnode, Y))
    u1 = S.place(OPA, "U1", "LM358", AX + 50.8, Y + 2.54, unit=1, ref_pos=(AX + 48.5, Y - 7.5),
                 val_pos=(AX + 48.5, Y - 5), justify="left")
    ncap = AX + 38.1
    S.wire(r2.pin("2"), u1.pin("3"))
    S.junction((ncap, Y))
    c2 = S.place(C, "C2", "1n", ncap, Y + 3.81, ref_pos=(ncap - 2.0, Y + 2.54), val_pos=(ncap - 2.0, Y + 5.08),
                 justify="right")
    S.power("GND", *c2.pin("2"), show=False)
    fx = u1.pin("1")[0] + 7.62
    fy = u1.pin("1")[1]
    S.wire(u1.pin("2"), (u1.pin("2")[0], Y + 12.7), (fx, Y + 12.7), (fx, fy))
    S.wire(u1.pin("1"), (fx, fy))
    S.junction((fx, fy))
    S.label("F", (fx, fy))
    c1x = (xnode + fx) / 2
    c1y = Y - 15.24
    c1 = S.place(C, "C1", "2,2n", c1x, c1y, rot=90, ref_pos=(c1x, c1y - 3.5), val_pos=(c1x, c1y + 3.5),
                 justify="center")
    S.wire((xnode, Y), (xnode, c1y), c1.pin("1"))
    S.junction((xnode, Y))
    S.wire(c1.pin("2"), (fx, c1y), (fx, fy))
    # acople, atenuación y volumen
    f = (fx, fy)
    c3 = S.place(C, "C3", "1µ", fx + 10.16, fy, rot=90, ref_pos=(fx + 10.16, fy - 3.5),
                 val_pos=(fx + 10.16, fy + 3.5), justify="center")
    S.wire(f, c3.pin("1"))
    r3 = S.place(R, "R3", "150k", fx + 25.4, fy, rot=90, ref_pos=(fx + 25.4, fy - 3.5),
                 val_pos=(fx + 25.4, fy + 3.5), justify="center")
    S.wire(c3.pin("2"), r3.pin("1"))
    px = fx + 38.1
    p1 = S.place(POT, "P1", "10k log", px, fy + 3.81, mirror="x", ref_pos=(px - 3.0, fy + 4.7),
                 val_pos=(px - 3.0, fy + 7.2), justify="right")
    S.wire(r3.pin("2"), p1.pin("3"))
    S.power("GND", *p1.pin("1"), show=False)
    wp = p1.pin("2")
    u2x = wp[0] + 15.24
    u2y = wp[1] + 2.54
    u2 = S.place(AMP, "U2", "LM386", u2x, u2y, ref_pos=(u2x - 4.5, u2y - 10.0), val_pos=(u2x - 4.5, u2y - 7.6),
                 justify="right")
    S.wire(wp, u2.pin("3"))
    S.power("GND", *u2.pin("2"), show=False)
    S.power("GND", *u2.pin("4"), show=False)
    for n in ("1", "7", "8"):
        S.nc(u2.pin(n))
    S.text("Ganancia 20:\npines 1 y 8 abiertos", (u2x - 16, u2y + 12.0), 1.27)
    # alimentación y desacople de U2 (pin 6)
    v6 = u2.pin("6")
    rail_y = v6[1] - 7.62
    S.wire(v6, (v6[0], rail_y), (v6[0] + 17.78, rail_y))
    S.power("+5V", v6[0], rail_y)
    S.junction((v6[0], rail_y))
    c4 = S.place(C, "C4", "100n", v6[0] + 7.62, rail_y + 3.81, ref_pos=(v6[0] + 5.6, rail_y + 2.54),
                 val_pos=(v6[0] + 5.6, rail_y + 5.08), justify="right")
    c5 = S.place(CP, "C5", "100µ", v6[0] + 17.78, rail_y + 3.81, ref_pos=(v6[0] + 15.8, rail_y + 2.54),
                 val_pos=(v6[0] + 15.8, rail_y + 5.08), justify="right")
    S.wire((v6[0] + 7.62, rail_y), c4.pin("1"))
    S.junction((v6[0] + 7.62, rail_y))
    S.power("GND", *c4.pin("2"), show=False)
    S.power("GND", *c5.pin("2"), show=False)
    # salida del LM386: red de Zobel, C7, R5/R6 y jack
    o = u2.pin("5")
    p5x = o[0] + 7.62
    S.wire(o, (p5x, o[1]))
    S.junction((p5x, o[1]))
    S.label("P5", (p5x, o[1]))
    r4 = S.place(R, "R4", "10", p5x, o[1] + 12.7, ref_pos=(p5x - 2.4, o[1] + 11.4), val_pos=(p5x - 2.4, o[1] + 14.0), justify="right")
    S.wire((p5x, o[1]), r4.pin("1"))
    c6 = S.place(C, "C6", "47n", p5x, o[1] + 24.13, ref_pos=(p5x - 2.4, o[1] + 22.9), val_pos=(p5x - 2.4, o[1] + 25.4), justify="right")
    S.wire(r4.pin("2"), c6.pin("1"))
    S.power("GND", *c6.pin("2"), show=False)
    c7 = S.place(CP, "C7", "220µ", p5x + 10.16, o[1], rot=90, ref_pos=(p5x + 10.16, o[1] - 3.8),
                 val_pos=(p5x + 10.16, o[1] + 3.8), justify="center")
    S.wire((p5x, o[1]), c7.pin("1"))
    snode = (p5x + 22.86, o[1])
    S.wire(c7.pin("2"), snode)
    S.junction(snode)
    S.label("S", snode)
    r5y, r6y = o[1] - 6.35, o[1] + 6.35
    r5 = S.place(R, "R5", "100", snode[0] + 10.16, r5y, rot=90, ref_pos=(snode[0] + 10.16, r5y - 3.5),
                 val_pos=(snode[0] + 10.16, r5y + 3.5), justify="center")
    r6 = S.place(R, "R6", "100", snode[0] + 10.16, r6y, rot=90, ref_pos=(snode[0] + 10.16, r6y - 3.5),
                 val_pos=(snode[0] + 10.16, r6y + 3.5), justify="center")
    S.wire(snode, (snode[0], r5y), r5.pin("1"))
    S.wire(snode, (snode[0], r6y), r6.pin("1"))
    jx = snode[0] + 33.02
    j1 = S.place(JACK, "J1", "Jack estéreo 3,5 mm", jx, o[1], rot=180, ref_pos=(jx + 8.5, o[1] - 1.3),
                 val_pos=(jx + 8.5, o[1] + 1.3), justify="right")
    jt, jr, js = j1.pin("T"), j1.pin("R"), j1.pin("S")
    S.wire(r5.pin("2"), (jt[0] - 8.89, r5y), (jt[0] - 8.89, jt[1]), jt)
    S.wire(r6.pin("2"), (jr[0] - 6.35, r6y), (jr[0] - 6.35, jr[1]), jr)
    S.wire(js, (js[0] - 2.54, js[1]))
    S.power("GND", js[0] - 2.54, js[1], show=False)
    S.text("punta = L\nanillo = R\nmanga = GND", (jx + 8.5, o[1] + 8), 1.27)

    # ----------------------------------------------------------------- cargador de 5 V
    jpx, jpy = jx + 10.16, 33.02
    jp = S.place(CONN2, "J2", "Cargador 5 V", jpx, jpy, ref_pos=(jpx + 3.5, jpy - 3.5), val_pos=(jpx + 3.5, jpy - 1.0))
    p1x, p1y = jp.pin("1")
    S.wire((p1x, p1y), (p1x, p1y - 3.81), (p1x, p1y - 7.62))
    S.power("+5V", p1x, p1y - 7.62)
    S.flag_left(p1x, p1y - 3.81)
    p2x, p2y = jp.pin("2")
    S.wire((p2x, p2y), (p2x, p2y + 3.81), (p2x, p2y + 7.62))
    S.power("GND", p2x, p2y + 7.62, show=False)
    S.flag_left(p2x, p2y + 3.81)

    # ----------------------------------------------------------------- matriz WS2812B 8x8 (Bloques 12 y 13)
    MX, MY = 236.22, 140.97
    text, wpins = box_symbol("Sintetizador:WS2812B_8x8", "U4", "Matriz WS2812B 8x8", 30.48, 22.86, [
        ("L", "DIN", "DIN", "input", 7.62),
        ("R", "DOUT", "DOUT", "output", 7.62),
        ("T", "VDD", "VDD", "power_in", 15.24),
        ("B", "GND", "GND", "power_in", 15.24)])
    S.use_lib("Sintetizador:WS2812B_8x8", text, wpins)
    mat = S.place("Sintetizador:WS2812B_8x8", "U4", "Matriz WS2812B 8x8", MX, MY,
                  ref_pos=(MX, MY - 3.5), val_pos=(MX + 30.48, MY - 3.5), justify="left")
    dinp = mat.pin("DIN")
    r7 = S.place(R, "R7", "330", dinp[0] - 12.7, dinp[1], rot=90, ref_pos=(dinp[0] - 12.7, dinp[1] - 3.5),
                 val_pos=(dinp[0] - 12.7, dinp[1] + 3.5), justify="center")
    din = mcu.pin("P0.0")
    S.wire(din, (203.2, din[1]), (203.2, dinp[1]), r7.pin("1"))
    S.wire(r7.pin("2"), dinp)
    S.nc(mat.pin("DOUT"))
    S.power("+5V", *mat.pin("VDD"), value="+4V5")
    S.power("GND", *mat.pin("GND"), show=False)
    # fuente regulable de 4,5 V
    j4x, j4y = 312.42, 151.13
    j4 = S.place(CONN2, "J4", "Fuente regulable 4,5 V", j4x, j4y, ref_pos=(j4x + 3.5, j4y - 3.5),
                 val_pos=(j4x + 3.5, j4y - 1.0))
    q1x, q1y = j4.pin("1")
    S.wire((q1x, q1y), (q1x, q1y - 3.81), (q1x, q1y - 7.62))
    S.power("+5V", q1x, q1y - 7.62, value="+4V5")
    S.flag_left(q1x, q1y - 3.81)
    q2x, q2y = j4.pin("2")
    S.wire((q2x, q2y), (q2x, q2y + 3.81), (q2x, q2y + 7.62))
    S.power("GND", q2x, q2y + 7.62, show=False)
    S.text("DIN por GPIO (P0.0) con 330 Ω en serie, sin adaptador de nivel.\\n"
           "Alimentación: fuente regulable de 4,5 V, con masa común con la LPC\\n"
           "y en una línea distinta de la del audio. Brillo limitado al 25 %.",
           (210.0, 178.0), 1.27)
    S.frame((207.0, 124.0), (350.0, 190.0), "Visualizador de espectro: matriz WS2812B 8×8")

    # ----------------------------------------------------------------- marcos y títulos
    S.text("Sintetizador digital polifónico con LPC1769 · Esquemático", (30.48, 14.0), 3.5, bold=True)
    S.text("Electrónica Digital III · FCEFyN · UNC", (30.48, 20.5), 1.8)
    S.frame((43.18, 26.67), (99.06, 113.03), "Teclado y selección de onda")
    S.frame((30.48, 118.11), (99.06, 177.8), "Comunicación con la PC y medición")
    S.frame((AX + 5.08, 118.11), (AX + 49.53, 197.5), "Potenciómetros ADSR")
    S.frame((AX + 12.7, 50.0), (fx + 5.08, Y + 24.13), "Filtro de reconstrucción (Sallen-Key)")
    S.frame((fx + 7.62, 50.0), (p5x + 3.0, o[1] + 36.0), "Acople, volumen y amplificador")
    S.frame((p5x + 3.81, 50.0), (jx + 36.0, o[1] + 24.0), "Salida de audio")
    S.frame((jx + 3.0, 17.0), (jx + 50.0, 47.0), "Alimentación de 5 V")
    S.text("LPC1769: 3,3 V por USB (pull-ups internos y potenciómetros)\nPulsadores activos en bajo, sin resistencias externas\nEtapa de audio: cargador de 5 V; el positivo va solo a esa etapa\ny el negativo se une a GND de la placa. Todas las GND son una sola línea",
           (30.48, 187.0), 1.27)
    return S


if __name__ == "__main__":
    S = main()
    os.makedirs(OUTDIR, exist_ok=True)
    tb = ('\t(title_block (title "Sintetizador digital polifónico con LPC1769") (date "2026-11-15") (rev "1")\n'
          '\t\t(company "Electrónica Digital III · FCEFyN · UNC")\n'
          '\t\t(comment 1 "Brezzo, Giordano y Rocatagliatta · Grupo 18") (comment 2 "Etapa analógica: ver hardware/etapa-audio.md"))')
    S.write(os.path.join(OUTDIR, PROJECT + ".kicad_sch"), tb)
    # biblioteca del proyecto con los dos símbolos propios (placa LPCXpresso y adaptador USB-TTL)
    mine = [t.replace('(symbol "Sintetizador:', '(symbol "', 1) for k, t in S.libs.items()
            if k.startswith("Sintetizador:")]
    open(os.path.join(OUTDIR, "sintetizador.kicad_sym"), "w", encoding="utf-8").write(
        '(kicad_symbol_lib (version 20251024) (generator "kicad_symbol_editor") (generator_version "10.0")\n' +
        "\n".join(mine) + "\n)\n")
    open(os.path.join(OUTDIR, "sym-lib-table"), "w", encoding="utf-8").write(
        '(sym_lib_table\n  (version 7)\n  (lib (name "Sintetizador") (type "KiCad") '
        '(uri "${KIPRJMOD}/sintetizador.kicad_sym") (options "") (descr "Placa LPCXpresso y adaptador USB-TTL"))\n)\n')
    pro = os.path.join(OUTDIR, PROJECT + ".kicad_pro")
    # U1 (LM358) se dibuja solo con la mitad A: se ignoran los avisos de unidades sin colocar
    json.dump({"meta": {"filename": PROJECT + ".kicad_pro", "version": 3},
               "erc": {"rule_severities": {"missing_unit": "ignore", "missing_input_pin": "ignore",
                                           "missing_power_pin": "ignore"}}}, open(pro, "w"), indent=2)
    print("OK", os.path.join(OUTDIR, PROJECT + ".kicad_sch"))
