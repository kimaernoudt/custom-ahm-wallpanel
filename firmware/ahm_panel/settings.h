// Panel settings: defaults for the cafe, stored in flash, edited on the panel's web page.
#pragma once
#include <Arduino.h>
#include <Preferences.h>

enum KnobType : uint8_t {
  K_OFF = 0,          // knob does nothing
  K_INPUT = 1,        // a fixed input
  K_ZONE = 2,         // a zone
  K_CGROUP = 3,       // a control group
  K_SCENE_INPUT = 4,  // the input that belongs to the active scene button
};

// AHM channel type (N) for a knob type, -1 for none
inline int8_t ahmType(uint8_t t) {
  switch (t) {
    case K_INPUT:
    case K_SCENE_INPUT: return 0;
    case K_ZONE: return 1;
    case K_CGROUP: return 2;
  }
  return -1;
}

// Numbers are 1-based, as System Manager shows them
struct KnobCfg {
  char label[12];
  uint8_t type;
  uint8_t ch;         // input / zone / control group number
};

struct ButtonCfg {
  char label[12];
  uint16_t scene;     // scene recalled by the button, 0 = button off
  uint8_t input;      // input the K_SCENE_INPUT knob controls in this scene, 0 = keep the previous one
};

const uint16_t SETTINGS_VERSION = 1;

struct Settings {
  uint16_t version;
  char room[24];
  char password[24];  // web page + OTA updates
  char ahmIp[16];
  uint16_t ahmPort;
  bool dhcp;
  char ip[16], gateway[16], subnet[16];
  KnobCfg knob[4];
  ButtonCfg button[4];
  uint8_t step;        // level change per encoder click
  uint8_t brightness;  // LED rings, 5..255
  uint8_t ringOffset;  // LED number at the "minimum" position, 0..11
  bool ringReverse;    // rings fill anticlockwise
};

Settings cfg;

void setDefaults() {
  memset(&cfg, 0, sizeof(cfg));
  cfg.version = SETTINGS_VERSION;
  strlcpy(cfg.room, "cafe", sizeof(cfg.room));
  strlcpy(cfg.password, "ahmpanel", sizeof(cfg.password));
  strlcpy(cfg.ahmIp, "192.168.1.100", sizeof(cfg.ahmIp));
  cfg.ahmPort = 51325;
  cfg.dhcp = true;
  strlcpy(cfg.ip, "192.168.1.60", sizeof(cfg.ip));
  strlcpy(cfg.gateway, "192.168.1.1", sizeof(cfg.gateway));
  strlcpy(cfg.subnet, "255.255.255.0", sizeof(cfg.subnet));
  cfg.knob[0] = {"INPUT", K_SCENE_INPUT, 1};
  cfg.knob[1] = {"TRAP", K_CGROUP, 1};
  cfg.knob[2] = {"SAS", K_CGROUP, 2};
  cfg.knob[3] = {"TOOG", K_ZONE, 3};
  cfg.button[0] = {"START", 1, 1};
  cfg.button[1] = {"IPAD", 2, 1};
  cfg.button[2] = {"DJ", 3, 3};
  cfg.button[3] = {"EXTRA", 4, 5};
  cfg.step = 2;
  cfg.brightness = 60;
  cfg.ringOffset = 0;
  cfg.ringReverse = false;
}

Preferences prefs;

void loadSettings() {
  prefs.begin("panel", false);
  if (prefs.getBytesLength("cfg") != sizeof(cfg) ||
      prefs.getBytes("cfg", &cfg, sizeof(cfg)) != sizeof(cfg) ||
      cfg.version != SETTINGS_VERSION) {
    setDefaults();
  }
}

void saveSettings() { prefs.putBytes("cfg", &cfg, sizeof(cfg)); }

// Active scene button (0..3) survives a reboot, so the INPUT knob keeps the right channel
uint8_t loadActiveButton() { return prefs.getUChar("btn", 0) & 3; }
void saveActiveButton(uint8_t b) {
  if (prefs.getUChar("btn", 255) != b) prefs.putUChar("btn", b);
}
