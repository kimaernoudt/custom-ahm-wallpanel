"""parts.csv -> production/ahm-panel-bom.csv in JLCPCB format (Comment, Designator, Footprint, LCSC).

Only the SMD parts JLCPCB fits go in that BOM; the through-hole parts (encoders, buttons, connectors) are soldered
by hand and listed in production/hand-solder.csv.
"""
import csv
from collections import defaultdict

LCSC = {("R_0805_2012Metric", "10k"): "C17414", ("R_0805_2012Metric", "4.7k"): "C17673",
        ("R_0805_2012Metric", "3.3k"): "C26010", ("R_0805_2012Metric", "8.2k"): "C17828",
        ("R_0805_2012Metric", "22k"): "C17560", ("R_0805_2012Metric", "33R"): "C17634",
        ("C_0805_2012Metric", "100nF"): "C49678", ("C_0805_2012Metric", "10uF"): "C15850",
        ("C_1206_3216Metric", "22uF"): "C12891"}
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
        w.writerow([r["Ref"], r["PosX"] + "mm", r["PosY"] + "mm", "Top", r["Rot"]])
