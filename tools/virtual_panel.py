#!/usr/bin/env python3
"""Virtual wall panel: the same 4 knobs and 4 scene buttons as the real panel, on your computer.

    python3 virtual_panel.py                 # then type the AHM's IP and press Connect
    python3 virtual_panel.py 192.168.1.100   # connect straight away

Point it at the real AHM to check the commands with System Manager / Custom Control,
or at ahm_emulator.py (127.0.0.1) to test without the AHM.
Needs only Python 3 with Tkinter (included in the python.org installer for macOS).
"""
import queue
import socket
import sys
import threading
import tkinter as tk
from tkinter import ttk

import ahm_protocol as ahm

# Same mapping as the wall panel
KNOBS = [("INPUT", None, None),            # follows the active scene
         ("TRAP", ahm.CGROUP, 0),
         ("SAS", ahm.CGROUP, 1),
         ("TOOG", ahm.ZONE, 2)]
SCENES = [("START", 1), ("IPAD", 2), ("DJ", 3), ("EXTRA", 4)]
SCENE_INPUT = {1: 0, 2: 0, 3: 2, 4: 4}     # scene -> input channel (IPAD 1, DJ 3, EXTRA 5)


class Connection:
    def __init__(self, on_event, on_status):
        self.sock = None
        self.on_event = on_event
        self.on_status = on_status
        self.lock = threading.Lock()

    def connect(self, host, port):
        self.close()
        try:
            s = socket.create_connection((host, port), timeout=3)
            s.settimeout(None)
        except OSError as e:
            self.on_status(f"Could not connect to {host}:{port} ({e})")
            return False
        self.sock = s
        self.on_status(f"Connected to {host}:{port}")
        threading.Thread(target=self._reader, args=(s,), daemon=True).start()
        return True

    def _reader(self, s):
        parser = ahm.Parser()
        try:
            while data := s.recv(1024):
                for ev in parser.feed(data):
                    self.on_event(ev)
        except OSError:
            pass
        if s is self.sock:
            self.sock = None
            self.on_status("Disconnected")

    def send(self, data, what):
        with self.lock:
            if not self.sock:
                self.on_status("Not connected")
                return
            try:
                self.sock.sendall(data)
                self.on_event(("sent", what, data))
            except OSError as e:
                self.on_status(f"Send failed: {e}")

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None


class Panel(tk.Tk):
    def __init__(self, host):
        super().__init__()
        self.title("Virtual AHM wall panel")
        self.configure(bg="#17181b")
        self.events = queue.Queue()
        self.conn = Connection(lambda ev: self.events.put(ev), lambda s: self.events.put(("status", s)))
        self.scene = 1
        self.updating = False
        self.pending = {}
        self.known = {}      # slider index -> last level sent to / reported by the AHM

        style = ttk.Style(self)
        style.theme_use("clam")

        top = tk.Frame(self, bg="#17181b")
        top.pack(fill="x", padx=14, pady=(12, 4))
        tk.Label(top, text="AHM IP", fg="#ccc", bg="#17181b").pack(side="left")
        self.host = tk.Entry(top, width=16)
        self.host.insert(0, host or "127.0.0.1")
        self.host.pack(side="left", padx=6)
        self.port = tk.Entry(top, width=6)
        self.port.insert(0, str(ahm.PORT))
        self.port.pack(side="left")
        tk.Button(top, text="Connect", command=self.connect).pack(side="left", padx=6)
        self.status = tk.Label(top, text="Not connected", fg="#00e0d0", bg="#17181b")
        self.status.pack(side="left", padx=10)

        knobs = tk.Frame(self, bg="#17181b")
        knobs.pack(padx=14, pady=8)
        self.sliders, self.mutes, self.values = [], [], []
        for i, (name, _, _) in enumerate(KNOBS):
            col = tk.Frame(knobs, bg="#1f2024", padx=10, pady=8)
            col.grid(row=0, column=i, padx=6)
            tk.Label(col, text=name, fg="#eee", bg="#1f2024", font=("Helvetica", 14, "bold")).pack()
            val = tk.Label(col, text="-", fg="#00e0d0", bg="#1f2024", width=15)
            val.pack()
            s = tk.Scale(col, from_=127, to=0, orient="vertical", length=180, showvalue=False,
                         bg="#1f2024", fg="#eee", troughcolor="#2b2d31", highlightthickness=0,
                         command=lambda v, i=i: self.slider_moved(i, int(float(v))))
            s.set(ahm.LEVEL_0DB)
            self.known[i] = ahm.LEVEL_0DB   # don't send the start position to the AHM
            s.pack()
            m = tk.BooleanVar()
            tk.Checkbutton(col, text="MUTE", variable=m, fg="#ff3b30", bg="#1f2024", selectcolor="#2b2d31",
                           command=lambda i=i: self.mute_clicked(i)).pack(pady=(6, 0))
            row = tk.Frame(col, bg="#1f2024")
            row.pack(pady=4)
            tk.Button(row, text="−", width=2, command=lambda i=i: self.step(i, False)).pack(side="left")
            tk.Button(row, text="+", width=2, command=lambda i=i: self.step(i, True)).pack(side="left")
            self.sliders.append(s)
            self.mutes.append(m)
            self.values.append(val)

        btns = tk.Frame(self, bg="#17181b")
        btns.pack(padx=14, pady=6)
        self.scene_buttons = []
        for i, (name, num) in enumerate(SCENES):
            b = tk.Button(btns, text=name, width=10, height=2, command=lambda n=num: self.recall(n))
            b.grid(row=0, column=i, padx=6)
            self.scene_buttons.append(b)

        self.log = tk.Text(self, height=10, width=78, bg="#0f1012", fg="#bbb", font=("Menlo", 10))
        self.log.pack(padx=14, pady=(6, 12))
        self.refresh_labels()
        self.after(30, self.poll)
        if host:
            self.connect()

    # --- mapping ---
    def target(self, i):
        name, n, ch = KNOBS[i]
        if n is None:
            return ahm.INPUT, SCENE_INPUT.get(self.scene, 0)
        return n, ch

    def refresh_labels(self):
        for i, (name, _, _) in enumerate(KNOBS):
            n, ch = self.target(i)
            self.values[i].config(text=f"{ahm.level_text(self.sliders[i].get())}\n{ahm.channel_name(n, ch)}")
        for b, (name, num) in zip(self.scene_buttons, SCENES):
            b.config(relief="sunken" if num == self.scene else "raised")

    # --- actions ---
    def connect(self):
        try:
            port = int(self.port.get())
        except ValueError:
            port = ahm.PORT
        host = self.host.get().strip()
        threading.Thread(target=self._connect_and_sync, args=(host, port), daemon=True).start()

    def _connect_and_sync(self, host, port):
        if self.conn.connect(host, port):
            self.sync()

    def sync(self):
        for i in range(len(KNOBS)):
            n, ch = self.target(i)
            self.conn.send(ahm.get_level(n, ch), f"get level {ahm.channel_name(n, ch)}")
            self.conn.send(ahm.get_mute(n, ch), f"get mute {ahm.channel_name(n, ch)}")

    def slider_moved(self, i, lv):
        self.refresh_labels()
        # Tk calls this later for programmatic set() too, so skip values the AHM already has
        if self.updating or lv == self.known.get(i):
            return
        # throttle: send the latest value at most every 50 ms per knob
        first = i not in self.pending
        self.pending[i] = lv
        if first:
            self.after(50, lambda: self.flush(i))

    def flush(self, i):
        lv = self.pending.pop(i, None)
        if lv is not None:
            self.known[i] = lv
            n, ch = self.target(i)
            self.conn.send(ahm.set_level(n, ch, lv), f"set level {ahm.channel_name(n, ch)} = {ahm.level_text(lv)}")

    def step(self, i, up):
        n, ch = self.target(i)
        self.conn.send(ahm.step_level(n, ch, up), f"level {'up' if up else 'down'} {ahm.channel_name(n, ch)}")

    def mute_clicked(self, i):
        n, ch = self.target(i)
        on = self.mutes[i].get()
        self.conn.send(ahm.mute(n, ch, on), f"mute {'on' if on else 'off'} {ahm.channel_name(n, ch)}")

    def recall(self, scene):
        self.scene = scene
        self.refresh_labels()
        self.conn.send(ahm.recall_scene(scene), f"recall scene {scene}")
        self.after(300, self.sync)   # levels change with the scene, read them back

    # --- incoming ---
    def poll(self):
        while not self.events.empty():
            self.handle(self.events.get())
        self.after(30, self.poll)

    def write_log(self, text):
        self.log.insert("end", text + "\n")
        self.log.see("end")

    def handle(self, ev):
        kind = ev[0]
        if kind == "status":
            self.status.config(text=ev[1])
            self.write_log(f"-- {ev[1]}")
        elif kind == "sent":
            self.write_log(f"->  {ev[1]:<34} {ev[2].hex(' ').upper()}")
        elif kind in ("level", "mute"):
            _, n, ch, value = ev
            self.write_log(f"<-  {kind} {ahm.channel_name(n, ch)} = "
                           + (ahm.level_text(value) if kind == "level" else ("on" if value else "off")))
            for i in range(len(KNOBS)):
                if self.target(i) == (n, ch):
                    self.updating = True
                    if kind == "level":
                        self.known[i] = value
                        self.sliders[i].set(value)
                    else:
                        self.mutes[i].set(value)
                    self.updating = False
            self.refresh_labels()
        elif kind == "scene":
            self.scene = ev[1]
            self.write_log(f"<-  scene {ev[1]} recalled")
            self.refresh_labels()
        else:
            self.write_log(f"<-  {ev}")


if __name__ == "__main__":
    Panel(sys.argv[1] if len(sys.argv) > 1 else None).mainloop()
