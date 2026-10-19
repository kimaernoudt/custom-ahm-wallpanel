# Kim's AHM config ("Ahm cafe install ok.cfg", saved 2026-10-16)

AHM-16, firmware V1.61 rev 94016. The file is a gzip'd tar: CurrentSettings.dat, Scene*.dat, CustomControllerConfig.xml (Custom Control app 1.5.3), Devices/*.cfg.

- Inputs: 1 IPAD, (2 IPAD R?), DJ, EXTRA, Dante
- Zones: 1 TRAP, 2 ?, 3 TOOG, 4/5 SAS + SUB TRAP, SUB SAS (exact zone numbers to confirm)
- Control groups: CG1 TRAP, CG2 SAS
- Scenes: 1 CAFE START, 2 CAFE IPAD, 3 CAFE DJ, 4 CAFE EXTRA, 5 panic, 10 IPAD preset, 11 cafe tester
- Custom Control layout "cafe pages", tabs SYSTEM / IPAD / DJ / EXTRA: SYSTEM tab has scene recall buttons for scenes 2-4 and a GPO toggle; other tabs have faders for CG1 (TRAP), CG2 (SAS), Zone 3 (TOOG) and the source input, plus mute buttons.
- Custom Control devices at 192.168.1.91 ("cc") and 192.168.1.86 (iPads); the AHM's own IP is not in this file.

Wall panel mapping (left to right): INPUT = input level of the active scene (IPAD ip1, DJ ip3, EXTRA ip5), TRAP = CG1, SAS = CG2, TOOG = zone 3; buttons recall scenes START / IPAD / DJ / EXTRA (1-4).

## AHM TCP protocol (source: AHM TCP/IP Protocol V1.0, https://artwiz.jp/ah/wp-content/uploads/AHM64_TCP_Protocol_V1.pdf)
- TCP 51325 (plain), 51327 (TLS: send "UserProfile,UserPassword" first, expect "AuthOK").
- N: 0 = inputs (CH 00-3F), 1 = zones (00-3F), 2 = control groups (00-1F).
- Mute on: 9N CH 7F 9N CH 00 · Mute off: 9N CH 3F 9N CH 00
- Set level: BN 63 CH BN 62 17 BN 06 LV (7F = +10 dB, 69 = 0 dB, 00 = -inf)
- Inc/dec: BN 63 CH BN 62 20 then BN 06 7F (inc) / BN 06 3F (dec)
- Preset recall 1-128: B0 00 00 C0 SS (SS = preset-1); bank byte 01/02/03 for higher presets
- Get level: F0 00 00 1A 50 12 01 00 0N 01 0B CH F7 · Get mute: ... 0N 01 09 CH F7
