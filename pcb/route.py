"""Route the placed board with Freerouting, pour GND on both layers, run DRC.

Needs Java 21+ and freerouting-2.1.0.jar (github.com/freerouting/freerouting/releases).
Run:  python3 route.py /path/to/freerouting-2.1.0.jar   ->  ahm-panel.kicad_pcb + drc.rpt
"""
import os
import re
import subprocess
import sys
import pcbnew

JAR = sys.argv[1] if len(sys.argv) > 1 else "freerouting-2.1.0.jar"
SRC, OUT = "ahm-panel-placed.kicad_pcb", "ahm-panel.kicad_pcb"
WORK = "build"
mm = pcbnew.FromMM

board = pcbnew.LoadBoard(SRC)
nc = board.GetDesignSettings().m_NetSettings.m_DefaultNetClass
nc.SetTrackWidth(mm(0.25)); nc.SetClearance(mm(0.2))
nc.SetViaDiameter(mm(0.6)); nc.SetViaDrill(mm(0.3))
board.GetDesignSettings().m_CopperEdgeClearance = mm(0.3)    # JLCPCB minimum is 0.3 mm



def gnd_zone(layer, prio):
    """GND pour over the whole board (the outline polygon from Edge.Cuts)."""
    poly = pcbnew.SHAPE_POLY_SET()
    board.GetBoardPolygonOutlines(poly)
    z = pcbnew.ZONE(board)
    z.SetLayer(layer); z.SetNet(board.FindNet("GND")); z.SetAssignedPriority(prio)
    z.SetLocalClearance(mm(0.3)); z.SetMinThickness(mm(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(mm(0.3)); z.SetThermalReliefSpokeWidth(mm(0.4))
    z.Outline().AddOutline(poly.Outline(0))
    board.Add(z)
    return z



def fill():
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())


# 1. bottom GND plane first, so Freerouting sees it and drops GND pads onto it with vias
gnd_zone(pcbnew.B_Cu, 0)
fill()
os.makedirs(WORK, exist_ok=True)
dsn, ses = os.path.join(WORK, "ahm-panel.dsn"), os.path.join(WORK, "ahm-panel.ses")
pcbnew.ExportSpecctraDSN(board, dsn)

# 2. +5V gets its own 0.6 mm class, everything else 0.25 mm
d = open(dsn).read()
d = re.sub(r"(\(class kicad_default \"\"[^()]*?)\s\+5V\s", r"\1 ", d, count=1)
d = d.replace("(width 200)", "(width 250)")
power = ("    (class power +5V\n      (circuit\n        (use_via Via[0-1]_600:300_um)\n      )\n"
         "      (rule\n        (width 600)\n        (clearance 200)\n      )\n    )\n")
d = d.replace("    (class kicad_default", power + "    (class kicad_default", 1)
open(dsn, "w").write(d)

# 3. autoroute (headless)
if "--reuse" not in sys.argv:
    subprocess.run(["java", "-jar", JAR, "--gui.enabled=false", "-de", dsn, "-do", ses, "-mp", "100"], check=True)


def sexpr(text):
    toks = re.findall(r'\(|\)|"[^"]*"|[^\s()]+', text)
    stack = [[]]
    for t in toks:
        if t == "(":
            stack.append([])
        elif t == ")":
            x = stack.pop(); stack[-1].append(x)
        else:
            stack[-1].append(t.strip('"'))
    return stack[0][0]


def find(node, key):
    for c in node:
        if isinstance(c, list) and c and c[0] == key:
            yield c


# pcbnew.ImportSpecctraSES only works inside the editor, so read the session file here
tree = sexpr(open(ses).read())
routes = next(find(tree, "routes"))
res = next(find(routes, "resolution"))
scale = {"um": 1000, "mil": 25400, "mm": 1000000}[res[1]] / int(res[2])     # session unit -> nm
P = lambda x, y: pcbnew.VECTOR2I(round(float(x) * scale), round(-float(y) * scale))
layers = {"F.Cu": pcbnew.F_Cu, "B.Cu": pcbnew.B_Cu}
seen = set()
for n in find(next(find(routes, "network_out")), "net"):
    netinfo = board.FindNet(n[1])
    for w in find(n, "wire"):
        path = next(find(w, "path"))
        pts = [P(path[i], path[i + 1]) for i in range(3, len(path) - 1, 2)]
        for a, b in zip(pts, pts[1:]):
            key = (path[1],) + tuple(sorted([(a.x, a.y), (b.x, b.y)]))
            if a == b or key in seen:
                continue
            seen.add(key)
            t = pcbnew.PCB_TRACK(board)
            t.SetStart(a); t.SetEnd(b); t.SetWidth(round(float(path[2]) * scale))
            t.SetLayer(layers[path[1]]); t.SetNet(netinfo)
            board.Add(t)
    for v in find(n, "via"):
        via = pcbnew.PCB_VIA(board)
        via.SetPosition(P(v[2], v[3])); via.SetWidth(mm(0.6)); via.SetDrill(mm(0.3))
        via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu); via.SetNet(netinfo)
        board.Add(via)

# drop stubs the router left with a loose end (connectivity is only reliable on a reloaded board)
pcbnew.SaveBoard(OUT, board)
board = pcbnew.LoadBoard(OUT)
fill()
while True:
    board.BuildConnectivity()
    conn = board.GetConnectivity()
    loose = [t for t in board.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T and conn.TestTrackEndpointDangling(t, True)]
    if not loose:
        break
    for t in loose:
        board.Remove(t)

# 4. top GND pour fills whatever is left, then DRC
gnd_zone(pcbnew.F_Cu, 1)
fill()
pcbnew.SaveBoard(OUT, board)
pcbnew.WriteDRCReport(board, "drc.rpt", pcbnew.EDA_UNITS_MILLIMETRES, True)
print("".join(l for l in open("drc.rpt") if l.startswith("** ")))
