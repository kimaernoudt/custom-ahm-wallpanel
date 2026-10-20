#!/bin/sh
# Production files from ahm-panel.kicad_pcb (run after route.py): gerbers + drill (zipped), placement, BOM
set -e
cd "$(dirname "$0")"
B=ahm-panel.kicad_pcb
rm -rf production && mkdir -p production/gerbers
kicad-cli pcb export gerbers -o production/gerbers/ --no-x2 --subtract-soldermask \
  -l F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts $B
kicad-cli pcb export drill -o production/gerbers/ --format excellon --excellon-separate-th \
  --generate-map --map-format gerberx2 $B
(cd production/gerbers && python3 -m zipfile -c ../ahm-panel-gerbers.zip *)
kicad-cli pcb export pos -o production/ahm-panel-cpl-top.csv --format csv --units mm --side front $B
python3 bom.py
