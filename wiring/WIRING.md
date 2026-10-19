# AHM wall panel: wiring

Diagram: [wiring.png](wiring.png) (source: wiring.svg)

The Olimex ESP32-POE's Ethernet takes most of its pins. GPIO35 and GPIO39 are reserved for battery and power sensing, and GPIO34 is the on-board button. So the 4 encoders, their push switches and the 4 button LEDs go through one **MCP23017 I/O expander** (16 pins over 2 wires). The Olimex itself only uses 5 pins.

## Olimex ESP32-POE

| Olimex pin | Goes to | Note |
|---|---|---|
| 5V | 5V of all 4 LED rings | 1000 µF capacitor across 5V/GND at the first ring |
| 3.3V | MCP23017 VDD, 10k pull-ups | |
| GND | everything's GND | |
| GPIO13 | MCP23017 SDA | UEXT SDA |
| GPIO16 | MCP23017 SCL | UEXT SCL. Most modules have I²C pull-ups; if not, add 4.7k to 3.3V on SDA and SCL |
| GPIO36 | MCP23017 INTA | input-only pin: add a 10k pull-up to 3.3V |
| GPIO4 | DIN of LED ring 1 (INPUT) | through a 330 Ω resistor |
| GPIO33 | scene-button resistor ladder | analog input, see below |

## MCP23017 (address 0x20: A0, A1, A2 → GND; RESET → VDD)

| MCP pin | Connect to | | MCP pin | Connect to |
|---|---|---|---|---|
| GPA0 | INPUT encoder A | | GPB0 | TOOG encoder A |
| GPA1 | INPUT encoder B | | GPB1 | TOOG encoder B |
| GPA2 | TRAP encoder A | | GPB2 | TRAP push switch |
| GPA3 | TRAP encoder B | | GPB3 | SAS push switch |
| GPA4 | SAS encoder A | | GPB4 | TOOG push switch |
| GPA5 | SAS encoder B | | GPB5 | IPAD button LED + |
| GPA6 | INPUT push switch | | GPB6 | DJ button LED + |
| GPA7 | START button LED + | | GPB7 | EXTRA button LED + |

GPA7 and GPB7 are output-only on current MCP23017 chips, which is why they drive LEDs.

## Encoders (EC11, 5 pins)
- The 3-pin side has A, C (middle) and B. Wire C to GND, and A and B to the MCP pins above.
- The 2-pin side is the push switch. Wire one leg to GND and the other to the MCP push pin.
- The firmware turns on the MCP's internal pull-ups, so no resistors are needed.

## LED rings (WS2812, 12 LEDs each)
- Data chain: GPIO4 → 330 Ω → ring INPUT DIN, then ring INPUT DOUT → TRAP DIN → SAS DIN → TOOG DIN.
- Every ring gets 5V and GND. Put a 1000 µF capacitor across 5V/GND at the first ring.
- If the LEDs flicker, it's because the 3.3 V data signal is marginal for 5 V LEDs. Add a 74AHCT125 level shifter on the data line.

## Scene buttons
**Switch contacts: resistor ladder on GPIO33**
- 10k from 3.3V to the GPIO33 node, plus 100 nF from the node to GND.
- Each button connects the node to GND through its own resistor:

| Button | Resistor | Voltage on GPIO33 when pressed |
|---|---|---|
| START | none (wire) | ≈ 0.00 V |
| IPAD | 2.2k | ≈ 0.59 V |
| DJ | 4.7k | ≈ 1.05 V |
| EXTRA | 10k | ≈ 1.65 V |
| (none) | — | 3.3 V |

**LEDs:** LED + goes to the MCP pin (GPA7, GPB5, GPB6, GPB7) and LED − goes to GND. Buy the 3–6 V LED version of the button. For a bare LED, add 150 Ω in series.

## Power
Everything runs from PoE through the Olimex. Keep the LED-ring brightness capped in firmware (about 25%) so the 5 V rail stays under about 0.5 A.

If yours is the non-ISO Olimex, don't connect USB to a laptop while it's powered by PoE.
