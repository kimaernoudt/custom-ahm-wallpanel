# AHM cafe wall panel: parts list

There are two builds. The **panel PCB build** (2 café panels, ordered from JLCPCB on 2026-11-20) is the one in use;
the hand-wired list further down is kept for reference.

## Panel PCB build: what to buy besides the JLCPCB order (2 panels)

JLCPCB delivers the boards with the LEDs, encoders, buttons and chips fitted. The 2 Olimex ESP32-POE boards and the
UEXT cables are already on hand. Buy in the EU (Reichelt, Conrad.be, TinyTronics or Kiwi Electronics in NL,
Amazon.com.be; screws from Brico/Hubo) so everything is there before the boards arrive.

### To solder on the boards

| Qty | Part | Spec / search term | Approx. |
|---|---|---|---|
| 2 | UEXT connector J1 | 2x5 shrouded box header, 2.54 mm, straight, through-hole (Reichelt "WSL 10G") | €1 |
| 2 | 5 V connector J2 | JST XH 2-pin header, straight (B2B-XH-A) | €1 |
| 2 | 5 V lead | pre-crimped JST XH 2-pin lead, about 15 cm; the loose end is soldered to the Olimex pads marked +5V and GND ("JST XH 2.54 2 pin kit pre-crimped" has both) | €4 |

### Front and box

| Qty | Part | Spec / search term | Approx. |
|---|---|---|---|
| 8 | Knob | for 6 mm D (flat) shaft, 20 to 24 mm diameter, at least 10 mm deep and hollow underneath so it clears the nut ("aluminium knob 6mm D shaft 20mm black") | €12 |
| 8 | Encoder nut + washer | M7 x 0.75 ("M7x0.75 potentiometer nut"); the ALPS EC11E most likely comes without | €2 |
| 8 | Faceplate screw | M3 x 8 countersunk (DIN 7991), black | €2 |
| 8 | Threaded insert | M3 heat-set, 4.0 mm hole, up to 6 mm long (e.g. Ruthex M3 short) | €4 |
| 4 | Wall screw + plug | about 4 mm screws for the keyholes | €1 |
| 1 | Foam tape or 2 cable ties | to fix each Olimex inside the box | €2 |

### 3D printing

| Qty | Part | File | Material |
|---|---|---|---|
| 2 | Faceplate for the PCB (light tubes are part of it) | `3d/faceplate-pcb.stl`, face down | black PETG, about 60 g each |
| 2 | Back box | `3d/backbox.stl` | black PETG, about 120 g each |
| 10 | Button cap (5 per panel, print a few spare) | `3d/buttoncap.stl` | clear / natural PETG |
| 2 | Label inlay (optional) | `3d/labels.stl` | white, or fill the engraving with white paint |

### Network and power

| Qty | Part | Note | Approx. |
|---|---|---|---|
| 2 | PoE port | real 802.3af (48 V); the Olimex needs at least 37 V, so no passive 24 V injectors. A PoE switch (e.g. TP-Link TL-SG1005P, 4 PoE ports) or one 802.3af injector per panel (e.g. TP-Link TL-PoE150S) | €40 switch / €20 per injector |
| 2 | Short patch cable | 0.25 to 0.5 m, slim or flexible, from the wall cable or keystone to the Olimex | €4 |
| 1 | USB data cable | for the first firmware upload, matching the Olimex USB socket. Never plug USB in while PoE is connected (plain ESP32-POE is not isolated) | €0 to 5 |

Total besides JLCPCB and filament: about €30, plus PoE if the switch has none.

## Hand-wired build (reference)

Prices are approximate (AliExpress, Amazon, Olimex shop, October 2026). The search terms are what to type into the shop.

### Electronics

| Qty | Part | Spec / search term | Approx. |
|---|---|---|---|
| 1 | Controller | Olimex ESP32-POE (already owned) or ESP32-POE-ISO | €0 (€22 new) |
| 4 | Rotary encoder with push switch | "EC11 rotary encoder 20mm D shaft", 20 detents, M7 bushing + nut (the panel PCB uses the smooth ALPS EC11E1834403, fitted by JLCPCB: see pcb/ORDERING.md) | €5 |
| 4 | Knob | "aluminium knob 6mm D shaft 20mm", black | €8 |
| 4 | LED ring | "WS2812B 12 bit ring 37mm" (12 LEDs, 37 mm OD, 23 mm ID) | €8 |
| 4 | Push button | "12mm momentary push button LED", panel mount, normally open, 5 V LED if lit | €8 |
| 1 | I/O expander | "MCP23017 I2C module" (CJMCU-2317 or similar) | €2 |
| 1 | Resistor kit | 10 kΩ (×3), 2.2 kΩ, 4.7 kΩ (button ladder), 330 Ω (LED data line) | €3 |
| 1 | Capacitor | 100 nF ceramic (button ladder) | €0.10 |
| 1 | Capacitor | 470–1000 µF 10 V electrolytic across the LED ring 5 V supply | €1 |
| 1 | Perfboard / proto board | about 5 × 7 cm | €2 |
| 1 | Wire + headers | Dupont jumpers or 26 AWG silicone wire, female headers for the Olimex | €4 |

Electronics subtotal: **about €41** (or €63 including a new Olimex).

### Enclosure (3D printed)

| Qty | Part | Spec | Approx. |
|---|---|---|---|
| 1 | Faceplate | `3d/faceplate.stl`, black PETG or PLA, about 60 g | — |
| 1 | Back box | `3d/backbox.stl`, black PETG or PLA, about 120 g | — |
| 1 | Label inlay (optional) | `3d/labels.stl` in white, or white paint fill | — |
| 4 | Faceplate screws | M3 × 8 countersunk (DIN 7991) | €1 |
| 4 | Threaded inserts | M3 heat-set, 4 mm OD × 5–6 mm long | €1 |
| 2 | Wall screws + plugs | about 4 mm screws for the keyholes | €1 |

Hardware subtotal: **about €3**, plus filament.

### Power / network

| Qty | Part | Note | Approx. |
|---|---|---|---|
| 1 | PoE switch port or injector | 802.3af. Only needed if your switch has no PoE | €15 |
| 1 | Short Ethernet patch cable | inside the wall box | €2 |

### Totals
- Using your Olimex, with a PoE switch already in place: **about €47**
- Including a new Olimex and a PoE injector: **about €82**

### Notes
- The LED rings run on 5 V from the Olimex's 5 V pin. With 48 LEDs at modest brightness they draw under 0.5 A. Keep brightness capped in software.
- Encoders, push switches and button LEDs go through the MCP23017, and the 4 scene buttons share one analog pin. See wiring/WIRING.md.
