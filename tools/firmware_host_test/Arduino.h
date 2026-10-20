// Minimal host stand-ins for the Arduino-ESP32 APIs the panel uses (test only)
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <cstdlib>
#define ESP_ARDUINO_VERSION_MAJOR 3
#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 2
#define F(x) (x)
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
inline uint32_t millis() {
  static auto t0 = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() + 1;
}
inline void delay(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
#ifndef HAVE_STRLCPY_DECL
inline size_t strlcpy_(char *d, const char *s, size_t n) { size_t l = strlen(s); if (n) { size_t c = l < n - 1 ? l : n - 1; memcpy(d, s, c); d[c] = 0; } return l; }
#define strlcpy strlcpy_
#endif

class String {
 public:
  std::string s;
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const std::string &x) : s(x) {}
  String(char c) : s(1, c) {}
  String(int v) : s(std::to_string(v)) {}
  String(unsigned v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  const char *c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }
  void reserve(size_t n) { s.reserve(n); }
  long toInt() const { return atol(s.c_str()); }
  void replace(char a, char b) { for (auto &c : s) if (c == a) c = b; }
  String &operator+=(const String &o) { s += o.s; return *this; }
  String &operator+=(const char *o) { s += o; return *this; }
  String &operator+=(char c) { s += c; return *this; }
  friend String operator+(const String &a, const String &b) { return String(a.s + b.s); }
  friend String operator+(const String &a, const char *b) { return String(a.s + b); }
  friend String operator+(const char *a, const String &b) { return String(a + b.s); }
  bool operator==(const char *o) const { return s == o; }
};

struct SerialT {
  void begin(int) {}
  void printf(const char *f, ...) { va_list a; va_start(a, f); vprintf(f, a); va_end(a); fflush(stdout); }
  void println(const char *s = "") { ::printf("%s\n", s); fflush(stdout); }
  void println(const String &s) { println(s.c_str()); }
  void print(const char *s) { ::printf("%s", s); }
};
inline SerialT Serial;

// test hooks
inline int sim_but1 = HIGH;
inline uint32_t sim_ladder_mv = 3300;
inline void pinMode(int, int) {}
inline int digitalRead(int pin) { return pin == 34 ? sim_but1 : HIGH; }
inline uint32_t analogReadMilliVolts(int) { return sim_ladder_mv; }
#define ADC_11db 3
inline void analogSetPinAttenuation(int, int) {}
inline void enableLoopWDT() {}
struct EspT { void restart() { ::printf("[ESP.restart]\n"); } };
inline EspT ESP;

// FreeRTOS
typedef std::mutex portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(m) (m)->lock()
#define portEXIT_CRITICAL(m) (m)->unlock()
inline void vTaskDelay(int t) { std::this_thread::sleep_for(std::chrono::milliseconds(t)); }
inline int xTaskCreatePinnedToCore(void (*fn)(void *), const char *, int, void *arg, int, void *, int) {
  std::thread(fn, arg).detach();
  return 1;
}

class IPAddress {
 public:
  uint8_t b[4] = {0, 0, 0, 0};
  IPAddress() {}
  IPAddress(int a, int b2, int c, int d) { b[0] = a; b[1] = b2; b[2] = c; b[3] = d; }
  bool fromString(const char *s) { int a, b2, c, d; if (sscanf(s, "%d.%d.%d.%d", &a, &b2, &c, &d) != 4) return false; *this = IPAddress(a, b2, c, d); return true; }
  String toString() const { char t[20]; snprintf(t, 20, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]); return String(t); }
};
