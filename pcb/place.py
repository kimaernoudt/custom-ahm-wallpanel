"""AHM wall panel PCB v1: footprint placement and netlist (KiCad 7 pcbnew API).

Board coordinates = faceplate coordinates: x from the left edge, y from the top edge, in mm.
Run:  python3 place.py  -> ahm-panel-placed.kicad_pcb  (route.py routes it)
"""
import math
import pcbnew

FP = "/usr/share/kicad/footprints/"
KX = [25, 65, 105, 145]      # knob / button centres, same as 3d/ahm-panel.scad
KY, BY = 30, 68
LED_R = 15                   # LED circle radius
X0, Y0, X1, Y1 = 4, 4, 166, 86   # board outline (box inside is 2.4..167.6 x 2.4..87.6)
NOTCH = 11                   # corner cut-outs for the back box screw bosses (d 8 at 6,6)

mm = pcbnew.FromMM
def V(x, y): return pcbnew.VECTOR2I(mm(x), mm(y))

board = pcbnew.BOARD()
ds = board.GetDesignSettings()
ds.SetCopperLayerCount(2)
nets = {}

def net(name):
    if name not in nets:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
        nets[name] = n
    return nets[name]

def place(lib, name, ref, value, x, y, rot=0, back=False, pads=None, origin=(0, 0)):
    m = pcbnew.FootprintLoad(FP + lib + ".pretty", name)
    m.SetReference(ref)
    m.SetValue(value)
    board.Add(m)
    # origin = footprint-local point that should land on (x, y)
    ox, oy = origin
    a = math.radians(rot)
    dx = ox * math.cos(a) + oy * math.sin(a)
    dy = -ox * math.sin(a) + oy * math.cos(a)
    m.SetPosition(V(x - dx, y - dy))
    m.SetOrientationDegrees(rot)
    if back:
        m.Flip(m.GetPosition(), False)
    for p in m.Pads():
        n = (pads or {}).get(p.GetNumber())
        if n:
            p.SetNet(net(n))
    m.Reference().SetVisible(False)
    m.Value().SetVisible(False)
    return m

parts = []   # (ref, value, footprint, lcsc hint) for the BOM

def add(ref, value, lib, name, x, y, rot=0, back=False, pads=None, origin=(0, 0), lcsc=""):
    m = place(lib, name, ref, value, x, y, rot, back, pads, origin)
    parts.append((ref, value, name, lcsc, back))
    return m

# ---------------- outline ----------------
def edge(pts):
    for (a, b) in zip(pts, pts[1:] + pts[:1]):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(V(*a)); s.SetEnd(V(*b))
        s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
        board.Add(s)

N = NOTCH
edge([(N, Y0), (170 - N, Y0), (170 - N, N), (X1, N), (X1, 90 - N), (170 - N, 90 - N), (170 - N, Y1), (N, Y1),
      (N, 90 - N), (X0, 90 - N), (X0, N), (N, N)])

# ---------------- encoders + LED rings ----------------
ENC_SIG = {}
for k, cx in enumerate(KX):
    e = k + 1
    ENC_SIG[k] = (f"ENC{e}_A", f"ENC{e}_B", f"ENC{e}_SW")
    add(f"ENC{e}", "EC11E with switch, 20 mm shaft", "Rotary_Encoder", "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm",
        cx, KY, origin=(7.5, 2.5),
        pads={"A": f"ENC{e}_A", "B": f"ENC{e}_B", "C": "GND", "S1": f"ENC{e}_SW", "S2": "GND", "MP": "GND"})

# LED chain: ring 1 (LED1-12), ring 2, ring 3, ring 4, then the button LEDs right to left (LED49-52)
chain = 0
def dnet(i): return "LED_DATA" if i == 0 else f"D{i}"
for k, cx in enumerate(KX):
    for i in range(12):
        th = 210 + 30 * i                     # clockwise from 12 o'clock; LED 0 at 7 o'clock
        t = math.radians(th)
        x, y = cx + LED_R * math.sin(t), KY - LED_R * math.cos(t)
        n = k * 12 + i
        m = add(f"LED{n + 1}", "WS2812B-B", "LED_SMD", "LED_WS2812B_PLCC4_5.0x5.0mm_P3.2mm", x, y,
                rot=(180 - th) % 360, pads={"1": "+5V", "2": dnet(n + 1), "3": "GND", "4": dnet(n)}, lcsc="C114586")
        for g in m.GraphicalItems():           # the ring is too tight for the outline silk; it is hidden anyway
            if g.GetLayer() == pcbnew.F_SilkS:
                g.SetLayer(pcbnew.F_Fab)
        p1 = m.FindPadByNumber("1").GetPosition()   # pin-1 dot instead, just inside pad 1, for hand soldering
        d = pcbnew.PCB_SHAPE(board)
        d.SetShape(pcbnew.SHAPE_T_CIRCLE); d.SetLayer(pcbnew.F_SilkS); d.SetWidth(mm(0.3))
        ux, uy = (cx - x) / LED_R, (KY - y) / LED_R  # towards the knob: pad 1 is on the inner side
        px, py = p1.x + mm(1.15) * ux, p1.y + mm(1.15) * uy
        d.SetCenter(pcbnew.VECTOR2I(int(px), int(py))); d.SetEnd(pcbnew.VECTOR2I(int(px + mm(0.25)), int(py)))
        board.Add(d)
for j, b in enumerate([3, 2, 1, 0]):          # EXTRA, DJ, IPAD, START
    n = 48 + j
    # same 5050 part as the rings (one part type, in stock); sits under the cap flange, clear of the switch pads
    add(f"LED{n + 1}", "WS2812B-B", "LED_SMD", "LED_WS2812B_PLCC4_5.0x5.0mm_P3.2mm", KX[b], BY - 6.5,
        pads={"1": "+5V", "2": (dnet(n + 1) if j < 3 else None), "3": "GND", "4": dnet(n)}, lcsc="C114586")

# ---------------- scene buttons + resistor ladder (one analog pin) ----------------
LADDER_R = [None, "2.2k", "4.7k", "10k"]
for b, cx in enumerate(KX):
    sw_net = "LADDER" if b == 0 else f"LAD{b + 1}"
    add(f"SW{b + 1}", "6x6 mm tact, 5 mm high", "Button_Switch_THT", "SW_PUSH_6mm_H5mm", cx, BY, origin=(3.25, 2.25),
        pads={"1": sw_net, "2": "GND"})
    if b:
        add(f"R{b + 1}", LADDER_R[b], "Resistor_SMD", "R_0805_2012Metric", cx + 8, BY, rot=90,
            pads={"1": "LADDER", "2": sw_net})
add("R1", "10k", "Resistor_SMD", "R_0805_2012Metric", 37, 66, rot=90, pads={"1": "+3V3", "2": "LADDER"})
add("C1", "100nF", "Capacitor_SMD", "C_0805_2012Metric", 37, 71, rot=90, pads={"1": "LADDER", "2": "GND"})

# ---------------- MCP23017 I/O expander ----------------
U1 = {"9": "+3V3", "10": "GND", "12": "SCL", "13": "SDA", "15": "GND", "16": "GND", "17": "GND", "18": "RST",
      "21": "ENC1_A", "22": "ENC1_B", "23": "ENC2_A", "24": "ENC2_B", "25": "ENC3_A", "26": "ENC3_B", "27": "ENC1_SW",
      "1": "ENC4_A", "2": "ENC4_B", "3": "ENC2_SW", "4": "ENC3_SW", "5": "ENC4_SW"}
add("U1", "MCP23017-E/SO", "Package_SO", "SOIC-28W_7.5x17.9mm_P1.27mm", 85, 79, rot=90, pads=U1, lcsc="C47023")
add("C2", "100nF", "Capacitor_SMD", "C_0805_2012Metric", 72, 82, rot=0, pads={"1": "+3V3", "2": "GND"})
add("R5", "10k", "Resistor_SMD", "R_0805_2012Metric", 98, 82, rot=0, pads={"1": "+3V3", "2": "RST"})

# ---------------- connectors (on the back) ----------------
# J1 = UEXT from the Olimex: 1 3V3, 2 GND, 3 GPIO4 (LED data), 4 GPIO36 (buttons), 5 SCL GPIO16, 6 SDA GPIO13
j1 = add("J1", "UEXT 2x5 box header", "Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", 46, 79, rot=90,
         back=True, origin=(1.27, 5.08),
         pads={"1": "+3V3", "2": "GND", "3": "LED_3V3", "4": "LADDER", "5": "SCL", "6": "SDA"})
j1.FindPadByNumber("2").SetZoneConnection(pcbnew.ZONE_CONNECTION_FULL)   # tracks leave room for only 1 thermal spoke
add("R6", "4.7k", "Resistor_SMD", "R_0805_2012Metric", 58, 77, rot=90, pads={"1": "+3V3", "2": "SDA"})
add("R7", "4.7k", "Resistor_SMD", "R_0805_2012Metric", 58, 82, rot=90, pads={"1": "+3V3", "2": "SCL"})
# J2 = 5 V from the Olimex for the LEDs
add("J2", "JST XH 2-pin", "Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", 125, 80, back=True,
    origin=(1.25, 0), pads={"1": "+5V", "2": "GND"})
add("C3", "22uF", "Capacitor_SMD", "C_1206_3216Metric", 117, 82, rot=90, pads={"1": "+5V", "2": "GND"})
add("C4", "22uF", "Capacitor_SMD", "C_1206_3216Metric", 133, 82, rot=90, pads={"1": "+5V", "2": "GND"})
for k, cx in enumerate(KX):
    add(f"C{5 + k}", "10uF", "Capacitor_SMD", "C_0805_2012Metric", cx + 10, 54, rot=0, pads={"1": "+5V", "2": "GND"})

# ---------------- 3.3 V -> 5 V level shifter for the LED data ----------------
add("U2", "74AHCT1G125", "Package_TO_SOT_SMD", "SOT-23-5", 11, 57, pads={"1": "GND", "2": "LED_3V3", "3": "GND",
    "4": "LED_5V", "5": "+5V"}, lcsc="C7484")
add("C9", "100nF", "Capacitor_SMD", "C_0805_2012Metric", 11, 61.5, pads={"1": "+5V", "2": "GND"})
add("R8", "33R", "Resistor_SMD", "R_0805_2012Metric", 11, 52.5, pads={"1": "LED_5V", "2": "LED_DATA"})

# ---------------- silkscreen ----------------
def text(s, x, y, size=1.2, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s); t.SetPosition(V(x, y)); t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size))); t.SetTextThickness(mm(size * 0.15))
    if layer == pcbnew.B_SilkS: t.SetMirrored(True)
    board.Add(t)

text("AHM WALL PANEL v1", 145, 82, 1.2)
text("UEXT  to Olimex", 46, 72, 1.2, pcbnew.B_SilkS)
text("5V  from Olimex", 125, 76, 1.2, pcbnew.B_SilkS)

if __name__ == "__main__":
    pcbnew.SaveBoard("ahm-panel-placed.kicad_pcb", board)
    import csv
    with open("parts.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["ref", "value", "footprint", "lcsc", "side"])
        for r in parts: w.writerow([r[0], r[1], r[2], r[3], "bottom" if r[4] else "top"])
    print("placed", len(parts), "parts,", len(nets), "nets")
