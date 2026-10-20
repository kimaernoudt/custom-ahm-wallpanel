#pragma once
#include "Arduino.h"
#include <vector>
class Preferences {
  std::map<std::string, std::vector<uint8_t>> m;
 public:
  bool begin(const char *, bool) { return true; }
  size_t getBytesLength(const char *k) { return m.count(k) ? m[k].size() : 0; }
  size_t getBytes(const char *k, void *b, size_t n) { if (!m.count(k)) return 0; memcpy(b, m[k].data(), n); return n; }
  size_t putBytes(const char *k, const void *b, size_t n) { m[k] = std::vector<uint8_t>((uint8_t *)b, (uint8_t *)b + n); return n; }
  uint8_t getUChar(const char *k, uint8_t d) { return m.count(k) ? m[k][0] : d; }
  size_t putUChar(const char *k, uint8_t v) { m[k] = {v}; return 1; }
};
