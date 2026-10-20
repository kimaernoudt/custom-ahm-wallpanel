#pragma once
#include "Arduino.h"
#include <atomic>
// Simulated MCP23017 at 0x20
inline std::atomic<uint16_t> sim_pins{0xFFFF};   // input levels, GPA = low byte
inline std::atomic<uint16_t> sim_olat{0};
struct WireT {
  uint8_t reg = 0, buf[8]; int n = 0, rd = 0; uint8_t addr = 0;
  void begin(int, int, int) {}
  void beginTransmission(uint8_t a) { addr = a; n = 0; }
  void write(uint8_t b) { buf[n++] = b; }
  uint8_t endTransmission(bool = true) {
    if (addr != 0x20) return 2;
    reg = buf[0];
    if (reg == 0x14 && n == 3) sim_olat = buf[1] | (buf[2] << 8);
    return 0;
  }
  uint8_t requestFrom(uint8_t, uint8_t c) { rd = 0; return c; }
  int read() { uint16_t v = reg == 0x12 ? (uint16_t)sim_pins : 0; return rd++ == 0 ? (v & 0xFF) : (v >> 8); }
};
inline WireT Wire;
