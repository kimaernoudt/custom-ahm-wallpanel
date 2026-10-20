#!/usr/bin/env python3
"""Fake AHM: listens on TCP 51325 and prints every command it receives in plain words.

Use it to test the wall panel (or the virtual panel) without touching the real AHM:
    python3 ahm_emulator.py            # then point the panel at this computer's IP

Like the real unit it answers level/mute requests and sends changes to the other
connected clients, so two panels stay in sync.
"""
import argparse
import asyncio
import datetime

import ahm_protocol as ahm

SCENE_NAMES = {1: "CAFE START", 2: "CAFE IPAD", 3: "CAFE DJ", 4: "CAFE EXTRA",
               5: "panic", 10: "IPAD preset", 11: "cafe tester"}
NAMES = {(ahm.INPUT, 0): "IPAD", (ahm.INPUT, 2): "DJ", (ahm.INPUT, 4): "EXTRA",
         (ahm.ZONE, 0): "TRAP", (ahm.ZONE, 2): "TOOG", (ahm.CGROUP, 0): "TRAP", (ahm.CGROUP, 1): "SAS"}


class FakeAhm:
    def __init__(self):
        self.levels = {}
        self.mutes = {}
        self.clients = set()

    def name(self, n, ch):
        label = NAMES.get((n, ch))
        return ahm.channel_name(n, ch) + (f" ({label})" if label else "")

    def log(self, peer, text):
        print(f"{datetime.datetime.now():%H:%M:%S}  {peer:<21} {text}", flush=True)

    def broadcast(self, data, sender):
        for w in list(self.clients):
            if w is not sender:
                w.write(data)

    def handle(self, ev, writer, peer):
        kind = ev[0]
        if kind == "level":
            _, n, ch, lv = ev
            self.levels[(n, ch)] = lv
            self.log(peer, f"SET LEVEL   {self.name(n, ch)} = {ahm.level_text(lv)}")
            self.broadcast(ahm.set_level(n, ch, lv), writer)
        elif kind == "step":
            _, n, ch, up = ev
            lv = self.levels.get((n, ch), ahm.LEVEL_0DB)
            lv = max(0, min(127, lv + (1 if up else -1)))
            self.levels[(n, ch)] = lv
            self.log(peer, f"LEVEL {'UP  ' if up else 'DOWN'}  {self.name(n, ch)} -> {ahm.level_text(lv)}")
            self.broadcast(ahm.set_level(n, ch, lv), None)   # real AHM reports the new level
        elif kind == "mute":
            _, n, ch, on = ev
            self.mutes[(n, ch)] = on
            self.log(peer, f"MUTE {'ON ' if on else 'OFF'}    {self.name(n, ch)}")
            self.broadcast(ahm.mute(n, ch, on), writer)
        elif kind == "scene":
            s = ev[1]
            self.log(peer, f"RECALL      scene {s} {SCENE_NAMES.get(s, '')}")
        elif kind == "get_level":
            _, n, ch = ev
            lv = self.levels.get((n, ch), ahm.LEVEL_0DB)
            self.log(peer, f"GET LEVEL   {self.name(n, ch)} -> {ahm.level_text(lv)}")
            writer.write(ahm.set_level(n, ch, lv))
        elif kind == "get_mute":
            _, n, ch = ev
            on = self.mutes.get((n, ch), False)
            self.log(peer, f"GET MUTE    {self.name(n, ch)} -> {'on' if on else 'off'}")
            writer.write(ahm.mute(n, ch, on))
        else:
            self.log(peer, f"UNKNOWN     {ev[1].hex(' ')}")

    async def serve_client(self, reader, writer):
        peer = "%s:%d" % writer.get_extra_info("peername")[:2]
        self.clients.add(writer)
        self.log(peer, "connected")
        parser = ahm.Parser()
        try:
            while data := await reader.read(1024):
                for ev in parser.feed(data):
                    self.handle(ev, writer, peer)
                await writer.drain()
        except ConnectionError:
            pass
        finally:
            self.clients.discard(writer)
            self.log(peer, "disconnected")


async def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=ahm.PORT)
    ap.add_argument("--host", default="0.0.0.0")
    args = ap.parse_args()
    fake = FakeAhm()
    server = await asyncio.start_server(fake.serve_client, args.host, args.port)
    print(f"Fake AHM listening on {args.host}:{args.port}  (Ctrl+C to stop)", flush=True)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
