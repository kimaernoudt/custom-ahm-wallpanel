"""Allen & Heath AHM TCP/IP protocol (V1.0) helpers: message builders and a stream parser.

N (channel type): 0 = input (CH 0-63), 1 = zone (0-63), 2 = control group (0-31).
Levels: 0x7F = +10 dB, 0x69 = 0 dB, 0x00 = -inf.
"""

INPUT, ZONE, CGROUP = 0, 1, 2
TYPE_NAMES = {INPUT: "Input", ZONE: "Zone", CGROUP: "Control group"}
PORT = 51325
SYSEX_HEADER = bytes([0xF0, 0x00, 0x00, 0x1A, 0x50, 0x12, 0x01, 0x00])
LEVEL_0DB = 0x69


def set_level(n, ch, lv):
    return bytes([0xB0 | n, 0x63, ch, 0xB0 | n, 0x62, 0x17, 0xB0 | n, 0x06, lv & 0x7F])


def step_level(n, ch, up):
    return bytes([0xB0 | n, 0x63, ch, 0xB0 | n, 0x62, 0x20, 0xB0 | n, 0x06, 0x7F if up else 0x3F])


def mute(n, ch, on):
    return bytes([0x90 | n, ch, 0x7F if on else 0x3F, 0x90 | n, ch, 0x00])


def recall_scene(scene):
    """scene is 1-based (1..500)."""
    s = scene - 1
    return bytes([0xB0, 0x00, s // 128, 0xC0, s % 128])


def get_level(n, ch):
    return SYSEX_HEADER + bytes([n, 0x01, 0x0B, ch, 0xF7])


def get_mute(n, ch):
    return SYSEX_HEADER + bytes([n, 0x01, 0x09, ch, 0xF7])


def level_text(lv):
    if lv == 0:
        return "-inf"
    if lv == LEVEL_0DB:
        return "0 dB"
    if lv == 0x7F:
        return "+10 dB"
    return f"{lv}"


def channel_name(n, ch):
    return f"{TYPE_NAMES.get(n, f'type {n}')} {ch + 1}"


class Parser:
    """Feeds raw bytes, yields decoded events as tuples:
    ("level", n, ch, lv) · ("step", n, ch, up) · ("mute", n, ch, on)
    ("scene", scene) · ("get_level", n, ch) · ("get_mute", n, ch) · ("unknown", bytes)
    """

    def __init__(self):
        self.buf = bytearray()
        self.nrpn = {}   # n -> [ch, param]
        self.bank = 0

    def feed(self, data):
        self.buf.extend(data)
        events = []
        while self.buf:
            b = self.buf[0]
            if b == 0xF0:
                end = self.buf.find(0xF7)
                if end < 0:
                    break
                msg = bytes(self.buf[:end + 1])
                del self.buf[:end + 1]
                events.append(self._sysex(msg))
            elif 0x90 <= b <= 0x9F:
                if len(self.buf) < 3:
                    break
                n, ch, vel = b & 0x0F, self.buf[1], self.buf[2]
                del self.buf[:3]
                if vel == 0x00:
                    continue   # second half of the mute message
                events.append(("mute", n, ch, vel >= 0x40))
            elif 0xB0 <= b <= 0xBF:
                if len(self.buf) < 3:
                    break
                n, cc, val = b & 0x0F, self.buf[1], self.buf[2]
                del self.buf[:3]
                ev = self._cc(n, cc, val)
                if ev:
                    events.append(ev)
            elif 0xC0 <= b <= 0xCF:
                if len(self.buf) < 2:
                    break
                prog = self.buf[1]
                del self.buf[:2]
                events.append(("scene", self.bank * 128 + prog + 1))
            else:
                events.append(("unknown", bytes([b])))
                del self.buf[:1]
        return events

    def _cc(self, n, cc, val):
        if cc == 0x00 and n == 0:
            self.bank = val
            return None
        st = self.nrpn.setdefault(n, [0, 0])
        if cc == 0x63:
            st[0] = val
        elif cc == 0x62:
            st[1] = val
        elif cc == 0x06:
            ch, param = st
            if param == 0x17:
                return ("level", n, ch, val)
            if param == 0x20:
                return ("step", n, ch, val >= 0x40)
            return ("unknown", bytes([0xB0 | n, cc, val]))
        else:
            return ("unknown", bytes([0xB0 | n, cc, val]))
        return None

    def _sysex(self, msg):
        if msg.startswith(SYSEX_HEADER) and len(msg) >= len(SYSEX_HEADER) + 5:
            n, _, cmd, ch = msg[8:12]
            if cmd == 0x0B:
                return ("get_level", n, ch)
            if cmd == 0x09:
                return ("get_mute", n, ch)
        return ("unknown", msg)
