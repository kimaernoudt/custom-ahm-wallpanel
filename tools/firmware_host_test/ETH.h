#pragma once
#include "WiFi.h"
enum arduino_event_id_t { ARDUINO_EVENT_ETH_START, ARDUINO_EVENT_ETH_GOT_IP, ARDUINO_EVENT_ETH_DISCONNECTED, ARDUINO_EVENT_ETH_STOP, ARDUINO_EVENT_OTHER };
enum eth_phy_type_t { ETH_PHY_LAN8720 };
enum eth_clock_mode_t { ETH_CLOCK_GPIO17_OUT };
struct NetworkT { void (*cb)(arduino_event_id_t) = nullptr; void onEvent(void (*c)(arduino_event_id_t)) { cb = c; } };
inline NetworkT Network;
struct EthT {
  std::string host = "esp32";
  bool begin(eth_phy_type_t, int, int, int, int, eth_clock_mode_t) { return true; }
  bool config(IPAddress, IPAddress, IPAddress, IPAddress) { return true; }
  IPAddress localIP() { return IPAddress(127, 0, 0, 1); }
  void setHostname(const char *h) { host = h; }
  const char *getHostname() { return host.c_str(); }
};
inline EthT ETH;
