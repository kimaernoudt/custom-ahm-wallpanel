# AHM cafe wall panel: parts list

Prices are approximate (AliExpress, Amazon, Olimex shop, October 2026). The search terms are what to type into the shop.

## Electronics

| Qty | Part | Spec / search term | Approx. |
|---|---|---|---|
| 1 | Controller | Olimex ESP32-POE (already owned) or ESP32-POE-ISO | €0 (€22 new) |
| 4 | Rotary encoder with push switch | "EC11 rotary encoder 20mm D shaft", 20 detents, M7 bushing + nut | €5 |
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

## Enclosure (3D printed)

| Qty | Part | Spec | Approx. |
|---|---|---|---|
| 1 | Faceplate | `3d/faceplate.stl`, black PETG or PLA, about 60 g | — |
| 1 | Back box | `3d/backbox.stl`, black PETG or PLA, about 120 g | — |
| 1 | Label inlay (optional) | `3d/labels.stl` in white, or white paint fill | — |
| 4 | Faceplate screws | M3 × 8 countersunk (DIN 7991) | €1 |
| 4 | Threaded inserts | M3 heat-set, 4 mm OD × 5–6 mm long | €1 |
| 2 | Wall screws + plugs | about 4 mm screws for the keyholes | €1 |

Hardware subtotal: **about €3**, plus filament.

## Power / network

| Qty | Part | Note | Approx. |
|---|---|---|---|
| 1 | PoE switch port or injector | 802.3af. Only needed if your switch has no PoE | €15 |
| 1 | Short Ethernet patch cable | inside the wall box | €2 |

## Totals
- Using your Olimex, with a PoE switch already in place: **about €47**
- Including a new Olimex and a PoE injector: **about €82**

## Notes
- The LED rings run on 5 V from the Olimex's 5 V pin. With 48 LEDs at modest brightness they draw under 0.5 A. Keep brightness capped in software.
- Encoders, push switches and button LEDs go through the MCP23017, and the 4 scene buttons share one analog pin. See wiring/WIRING.md.
