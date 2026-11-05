# Ordering the panel PCB (2 café panels)

JLCPCB makes the board and fits the LEDs, encoders and buttons. You solder only the two connectors on the back.
Nothing here is ordered yet: prices are estimates from November 2026 and the stock must be checked in the quote.

## 1. JLCPCB order (https://cart.jlcpcb.com/quote)

Upload `production/ahm-panel-gerbers.zip`, then:

| Option | Choose |
|---|---|
| Layers / thickness | 2 layers, 1.6 mm (detected from the zip: 162 x 82 mm) |
| PCB qty | 5 (the minimum) |
| Colour | **green** (fastest; the board is hidden behind the faceplate anyway) |
| Surface finish | HASL lead-free |
| Mark on PCB | "Remove mark" or "specify location" (no effect on function) |
| PCB Assembly | **on**, PCBA type **Standard** (the LEDs are not allowed in Economic) |
| Assembly side | **Top side** |
| PCBA qty | **2** |
| Tooling holes | added by JLCPCB |
| Confirm parts placement | **yes** (free: they send photos of the placement before soldering) |

Next screens:
1. BOM: `production/ahm-panel-bom.csv`. CPL: `production/ahm-panel-cpl-jlc.csv`.
2. Parts list: all 13 lines must be matched and in stock. J1 and J2 are not in the BOM, so JLC flags them as "not
   placed"; that is expected. If a part is out of stock, the alternatives are below.
3. Placement preview, check these three things before paying:
   - **LEDs**: the corner notch / pin-1 mark of every LED sits on the white dot printed on the board (the dot always faces
     the knob). If all 52 are off by the same angle, rotate one and apply to all; that only means JLC's footprint is
     drawn differently from KiCad's.
   - **Encoders**: the 3 signal pins left of the shaft, the 2 switch pins right of it, shaft in the
     middle of the LED ring.
   - **Tact switches**: the 4 legs land on the 4 round pads under each button cap.
4. Shipping: **DHL Express** (fastest to Belgium, 2-4 days). VAT is charged at checkout under IOSS (order under €150),
   so DHL asks nothing extra at the door. Then pay.

Timing: Standard PCBA is usually 3-5 working days, +1 day for the through-hole parts, so expect the boards in Belgium
about 7-10 working days after paying.

Expected cost for 5 boards with 2 assembled: about $70 to $110 incl. shipping. The fixed part is board ~$15, setup
$8 + stencil $1.50, $3 per "extended" part type (about 6 here), and $3.50 hand-solder fee for the through-hole parts.

Parts JLC fits (per board):

| Ref | Part | LCSC | Alternative if out of stock |
|---|---|---|---|
| LED1-52 | Worldsemi WS2812B-B/W, 5050 | C114586 | C2761795 (WS2812B-B/T, same LED) |
| ENC1-4 | ALPS EC11E18244A5: 36 detents, push switch, 20 mm flat shaft, M7 bushing | C255515 | C202365 (EC11E18244AU) |
| SW1-4 | SHOU HAN TS665CJ, 6x6 mm tact, 5 mm high, through-hole | C393938 | C2837515 (Kinghelm KH-6X6X5H-TJ) |
| U1 | MCP23017-E/SO | C47023 | |
| U2 | 74AHCT1G125 | C7484 | |
| R, C | 0805/1206 passives | see BOM | |

## 2. Buy separately

For 2 panels. Buy from EU shops (Olimex ships from Bulgaria in 2-4 days; the rest from Reichelt, Conrad.be or
Amazon.be, next-day to a few days) rather than AliExpress (2-4 weeks); all of it arrives before the boards do.

| Qty | Part | Where / search term | Approx. |
|---|---|---|---|
| 1 | Olimex ESP32-POE (you have one, this is the 2nd) | https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/ | €25 |
| 2 | UEXT cable, 10-pin IDC, 15 cm (board J1 to Olimex UEXT) | https://www.olimex.com/Products/Components/Cables/ "CABLE-IDC10-15cm" | €2 |
| 2 | 2x5 pin 2.54 mm shrouded box header, straight, through-hole (J1) | Reichelt "WSL 10G" or "2x5 IDC box header 2.54 straight" | €1 |
| 2 | JST XH 2-pin socket, straight, through-hole (J2), with a pre-crimped XH 2-pin lead to female Dupont | "JST XH 2.54 2 pin kit pre-crimped" (any XH kit has both) | €4 |
| 8 | Knob for 6 mm flat (D) shaft, about 20 mm diameter | "aluminium knob 6mm D shaft 20mm black" | €12 |
| 8 | M7 x 0.75 hex nut + washer (in case the encoders arrive without) | "M7x0.75 potentiometer nut" | €2 |

Printed (`3d/`): `faceplate-pcb.stl` and `backbox.stl` x 2, `buttoncap.stl` x 8 in clear/natural PETG, plus the M3
screws, heat-set inserts and wall screws from [PARTS-LIST.md](../PARTS-LIST.md).

## 3. When the boards arrive

1. Solder J1 (notch of the shroud as printed on the silkscreen) and J2 on the back.
2. Flash `firmware/ahm_panel` with `#define PANEL_PCB 1` at the top of `panel_io.h`. That also sets the encoder
   decoding for these 36-detent encoders (one step per click).
3. Connect UEXT and the 5 V lead to the Olimex (pin-out in [README.md](README.md)), plug in PoE, open the panel's web
   page and set the AHM IP address.
