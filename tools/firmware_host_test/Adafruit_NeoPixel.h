#pragma once
#include "Arduino.h"
#define NEO_GRB 0
#define NEO_KHZ800 0
class Adafruit_NeoPixel {
 public:
  uint32_t px[64] = {0}; int count;
  Adafruit_NeoPixel(int n, int, int) : count(n) {}
  void begin() {} void setBrightness(uint8_t) {} void clear() { memset(px, 0, sizeof(px)); } void show() {}
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return (r << 16) | (g << 8) | b; }
  void setPixelColor(int i, uint32_t c) { if (i >= 0 && i < count) px[i] = c; }
};
