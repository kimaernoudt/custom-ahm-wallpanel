# Panel firmware (Olimex ESP32-POE)

## Install
1. Arduino IDE: add the "esp32 by Espressif" boards (2.x or 3.x) and the library **Adafruit NeoPixel**.
   For the panel PCB (`pcb/`), set `#define PANEL_PCB 1` at the top of `panel_io.h`.
2. Open `ahm_panel.ino`, choose board **OLIMEX ESP32-PoE**, upload over USB once.
3. Plug the panel into a PoE port. The Serial Monitor (115200) prints its address, e.g. `Ethernet up: http://192.168.1.57/`.

## Set up a room
Open that address in a browser (user `admin`, password `ahmpanel` until you change it). Everything below is
stored in the panel and survives power cuts:

| Setting | Cafe default |
|---|---|
| AHM IP address and port | 192.168.1.100 : 51325 |
| Panel network | DHCP, or a fixed IP |
| Knob 1–4: label, what it controls (input, zone, control group, input of the active scene, off), number | INPUT = input of active scene, TRAP = CG 1, SAS = CG 2, TOOG = zone 3 |
| Button 1–4: label, scene number, the input that belongs to it | START 1 (input 1), IPAD 2 (input 1), DJ 3 (input 3), EXTRA 4 (input 5) |
| Level steps per click, ring brightness, ring start LED and direction | 2, 60, LED 0, clockwise |

Labels on the faceplate are printed, so a new room with different names needs a new label inlay (`3d/ahm-panel.scad`).
Hold the Olimex **BUT1** button while powering up to go back to the defaults (also resets the password).

After the first USB upload, updates go over the network: Arduino IDE > Tools > Port > the panel's network port.

## How it behaves
- Turning sends an absolute level (faster turning moves further). Pressing a knob toggles mute; the ring turns red.
- Rings show the AHM's real level and follow changes made in System Manager or Custom Control.
- A scene button recalls the scene, lights up, and re-reads all levels.
- If the AHM is unreachable the rings show a slow amber dot and the active button blinks; the panel reconnects by itself
  (1 → 10 s back-off) and detects a dead link within 15 s with a keep-alive.
- A watchdog restarts the panel if the firmware ever hangs for 5 s. Status: `http://<panel IP>/status`.

## Testing without hardware
`tools/firmware_host_test/run.sh` builds this firmware on a computer, with stand-ins for the ESP32 parts, and drives it
against the fake AHM (`tools/ahm_emulator.py`).
