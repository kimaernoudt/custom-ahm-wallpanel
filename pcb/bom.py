"""parts.csv -> production/ahm-panel-bom.csv in JLCPCB format (Comment, Designator, Footprint, LCSC).

Only the SMD parts JLCPCB fits go in that BOM; the through-hole parts (encoders, buttons, connectors) are soldered
by hand and listed in production/hand-solder.csv.
"""
import csv
import math
from collections import defaultdict

LCSC = {("R_0805_2012Metric", "10k"): "C17414", ("R_0805_2012Metric", "4.7k"): "C17673",
        ("R_0805_2012Metric", "3.3k"): "C26010", ("R_0805_2012Metric", "8.2k"): "C17828",
        ("R_0805_2012Metric", "22k"): "C17560", ("R_0805_2012Metric", "33R"): "C17634",
        ("C_0805_2012Metric", "100nF"): "C49678", ("C_0805_2012Metric", "10uF"): "C15850",
        ("C_1206_3216Metric", "22uF"): "C12891",
        # fitted by JLCPCB too (Kim wants the LEDs and encoders on the board when it arrives)
        ("LED_WS2812B_PLCC4_5.0x5.0mm_P3.2mm", "WS2812B-B"): "C2874885",                   # Worldsemi WS2812B-V5/W (C114586 left JLC assembly, Nov 2026)
        ("RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm", "EC11E with switch, 20 mm shaft"): "C361165",  # ALPS EC11E1834403, smooth (no detents)
        ("SW_PUSH_6mm_H5mm", "6x6 mm tact, 5 mm high"): "C393938"}                        # SHOU HAN TS665CJ, 6x6x5 THT

# KiCad puts these THT footprints' origin on pin 1; JLCPCB wants the body centre (offset in footprint coordinates).
CENTRE = {"ENC": (7.5, 2.5), "SW": (3.25, 2.25)}
groups = defaultdict(list)
for r in csv.DictReader(open("parts.csv")):
    lcsc = r["lcsc"] or LCSC.get((r["footprint"], r["value"]), "")
    groups[(r["value"], r["footprint"], lcsc)].append(r["ref"])
with open("production/ahm-panel-bom.csv", "w", newline="") as f, open("production/hand-solder.csv", "w", newline="") as h:
    w, wh = csv.writer(f), csv.writer(h)
    w.writerow(["Comment", "Designator", "Footprint", "LCSC"])
    wh.writerow(["Part", "Designator", "Footprint", "Qty"])
    for (val, fp, lcsc), refs in sorted(groups.items(), key=lambda g: g[1][0]):
        if lcsc:
            w.writerow([val, ",".join(refs), fp, lcsc])
        else:
            wh.writerow([val, ",".join(refs), fp, len(refs)])

# KiCad placement file -> JLCPCB column names (Designator, Mid X, Mid Y, Layer, Rotation)
with open("production/ahm-panel-cpl-top.csv") as src, open("production/ahm-panel-cpl-jlc.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
    for r in csv.DictReader(src):
        x, y, rot = float(r["PosX"]), float(r["PosY"]), float(r["Rot"])
        dx, dy = CENTRE.get(r["Ref"].rstrip("0123456789"), (0, 0))
        a = math.radians(rot)   # pos file is Y-up, footprint offsets are Y-down
        x += dx * math.cos(a) + dy * math.sin(a)
        y += dx * math.sin(a) - dy * math.cos(a)
        w.writerow([r["Ref"], f"{x:.4f}mm", f"{y:.4f}mm", "Top", r["Rot"]])
