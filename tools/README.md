# Test tools (no hardware needed)

Plain Python 3, no extra packages. The virtual panel needs Tkinter, which the python.org installer for macOS includes.

| File | What it is |
|---|---|
| `virtual_panel.py` | The wall panel on your screen: 4 knobs (slider, MUTE, −/+) and the 4 scene buttons, with the same mapping as the real panel. The log shows every byte sent and received. |
| `ahm_emulator.py` | A fake AHM on TCP 51325. It prints each command in plain words (`RECALL scene 3 CAFE DJ`, `SET LEVEL Control group 1 (TRAP) = 80`) and answers level/mute requests. |
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
