# Test tools (no hardware needed)

Plain Python 3, no extra packages. The virtual panel needs Tkinter, which the python.org installer for macOS includes.

| File | What it is |
|---|---|
| `virtual_panel.py` | The wall panel on your screen: 4 knobs (slider, MUTE, −/+) and the 4 scene buttons, with the same mapping as the real panel. The log shows every byte sent and received. |
| `ahm_emulator.py` | A fake AHM on TCP 51325. It prints each command in plain words (`RECALL scene 3 CAFE DJ`, `SET LEVEL Control group 1 (TRAP) = 80`) and answers level/mute requests. |
| `serial_bridge.py` | Connects a Pro Micro with one encoder (`firmware/promicro_encoder_test`) over USB to the AHM or the editor. |
| `ahm_protocol.py` | The AHM TCP protocol (V1.0) builders and parser both tools share. |

## 1. Panel → AHM editor or real AHM
```
python3 virtual_panel.py 127.0.0.1      # AHM System Manager on this Mac
python3 virtual_panel.py 192.168.1.x    # the real AHM-16
```
Move a slider or press a scene button and watch System Manager follow.

Check first that the editor actually accepts control connections: with System Manager open, run
`nc -vz 127.0.0.1 51325`. "succeeded" means it does. "Connection refused" means the offline editor
does not expose the TCP control port, and you test against the real AHM (with System Manager online
to watch it) or against the emulator below.

## 2. Panel → fake AHM
```
python3 ahm_emulator.py                 # terminal 1
python3 virtual_panel.py 127.0.0.1      # terminal 2
```

## 3. ESP32 test sketch → fake AHM
Run `python3 ahm_emulator.py` on your laptop, set `AHM_IP` in `firmware/ahm_test/ahm_test.ino` to the
laptop's IP address, and flash the Olimex. Each turn or press appears in the emulator's output.

## 4. Pro Micro encoder → editor
The Pro Micro has no network, so it sends each detent over USB and `serial_bridge.py` forwards it to the AHM.
1. Wire the encoder: A → pin 2, middle pin → GND, B → pin 3, switch → pin 4 and GND.
2. Flash `firmware/promicro_encoder_test` (board "Arduino Leonardo"). Check that the Serial Monitor prints `+`, `-` and `P`, then close it.
3. Run `python3 tools/serial_bridge.py` (defaults: 127.0.0.1, controls TRAP). Use `--target sas`, `toog`, `dj` and so on for another channel, or `--ahm <ip>` for the real AHM.

Turning sets the level (2 steps per click, change with `--step`), pressing toggles mute. If turning right lowers the level, swap the A and B wires.
