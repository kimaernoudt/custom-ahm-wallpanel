#!/usr/bin/env python3
"""Bridge: Pro Micro encoder on USB -> AHM (or the offline editor) over TCP.

    python3 serial_bridge.py                       # finds the Pro Micro, controls TRAP on 127.0.0.1
    python3 serial_bridge.py --ahm 192.168.1.100 --target sas
    python3 serial_bridge.py --port /dev/cu.usbmodem14101

The Pro Micro (firmware/promicro_encoder_test) prints "+" / "-" per detent and "P" per press.
Turning sets the level (--step values per detent, absolute "set level" like the panel),
pressing toggles mute. No extra packages needed (macOS / Linux).
"""
import argparse
import glob
import os
import socket
import termios
import threading

import ahm_protocol as ahm

TARGETS = {"input": (ahm.INPUT, 0), "dj": (ahm.INPUT, 2), "extra": (ahm.INPUT, 4),
           "trap": (ahm.CGROUP, 0), "sas": (ahm.CGROUP, 1), "toog": (ahm.ZONE, 2)}


def open_serial(path, baud=termios.B115200):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0                                                  # iflag
    attrs[1] = 0                                                  # oflag
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL       # cflag
    attrs[3] = 0                                                  # lflag: raw
    attrs[4] = attrs[5] = baud
    attrs[6][termios.VMIN] = 1
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return os.fdopen(fd, "rb", buffering=0)


def find_port():
    ports = sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*"))
    if not ports:
        raise SystemExit("No Pro Micro found. Plug it in, or pass --port /dev/cu.usbmodemXXXX")
    return ports[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ahm", default="127.0.0.1", help="AHM or System Manager IP (default 127.0.0.1)")
    ap.add_argument("--target", default="trap", choices=TARGETS, help="what the encoder controls")
    ap.add_argument("--port", help="serial port of the Pro Micro")
    ap.add_argument("--step", type=int, default=2, help="level change per detent (default 2)")
    args = ap.parse_args()

    n, ch = TARGETS[args.target]
    name = f"{args.target.upper()} ({ahm.channel_name(n, ch)})"
    port = args.port or find_port()
    ser = open_serial(port)
    sock = socket.create_connection((args.ahm, ahm.PORT), timeout=3)
    sock.settimeout(None)
    print(f"Pro Micro on {port} -> {args.ahm}:{ahm.PORT}, controlling {name}. Ctrl+C to stop.")

    muted = False
    level = ahm.LEVEL_0DB   # replaced by the AHM's real level as soon as it answers

    def reader():   # print what the AHM reports back, and track level and mute
        nonlocal muted, level
        parser = ahm.Parser()
        while data := sock.recv(1024):
            for ev in parser.feed(data):
                if ev[0] == "level" and ev[1:3] == (n, ch):
                    level = ev[3]
                    print(f"   AHM: {name} level = {ahm.level_text(ev[3])}")
                elif ev[0] == "mute" and ev[1:3] == (n, ch):
                    muted = ev[3]
                    print(f"   AHM: {name} mute {'on' if muted else 'off'}")
        print("AHM closed the connection")
        os._exit(1)

    threading.Thread(target=reader, daemon=True).start()
    sock.sendall(ahm.get_mute(n, ch) + ahm.get_level(n, ch))

    buf = b""
    while True:
        buf += ser.read(1)
        if not buf.endswith(b"\n"):
            continue
        line, buf = buf.strip(), b""
        if line in (b"+", b"-"):
            level = max(0, min(127, level + (args.step if line == b"+" else -args.step)))
            sock.sendall(ahm.set_level(n, ch, level))
            print(f"turn {line.decode()}  -> set level {ahm.level_text(level)}")
        elif line == b"P":
            muted = not muted
            sock.sendall(ahm.mute(n, ch, muted))
            print(f"press  -> mute {'on' if muted else 'off'}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
