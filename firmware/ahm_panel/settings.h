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

enum ButtonType : uint8_t {
  B_OFF = 0,          // button does nothing
  B_SCENE = 1,        // recall a scene
  B_POWER = 2,        // amps on/off: toggles the mute of an input that the AHM's GPO follows (unmuted = on)
  B_SAMPLE = 3,       // play an AHM playback track
};

const int NUM_BUTTONS = 5;   // ON/OFF, IPAD, DJ, EXTRA, BEL (the hand-wired panel has only the first 4)

struct ButtonCfg {
  char label[12];
  uint8_t type;
  uint16_t scene;     // B_SCENE: scene recalled by the button
  uint8_t input;      // B_SCENE: input the K_SCENE_INPUT knob controls in this scene, 0 = keep the previous one
  uint8_t ch;         // B_POWER: the input whose mute the GPO follows; B_SAMPLE: track 1-128
  uint8_t player;     // B_SAMPLE: 0 = mono 1, 1 = mono 2, 2 = stereo playback
};

const uint16_t SETTINGS_VERSION = 2;

struct Settings {
  uint16_t version;
  char room[24];
  char password[24];  // web page + OTA updates
  char ahmIp[16];
  uint16_t ahmPort;
  bool dhcp;
  char ip[16], gateway[16], subnet[16];
  KnobCfg knob[4];
  ButtonCfg button[NUM_BUTTONS];
  uint8_t step;        // level change per encoder click
  uint8_t brightness;  // LED rings, 5..MAX_BRIGHTNESS
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
  cfg.button[0] = {"ON/OFF", B_POWER, 0, 0, 16, 0};   // set GPO 1 on the AHM to follow input 16's mute
  cfg.button[1] = {"IPAD", B_SCENE, 2, 1, 0, 0};
  cfg.button[2] = {"DJ", B_SCENE, 3, 3, 0, 0};
  cfg.button[3] = {"EXTRA", B_SCENE, 4, 5, 0, 0};
  cfg.button[4] = {"BEL", B_SAMPLE, 0, 0, 1, 2};      // track 1 on the stereo player
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

// Active scene button survives a reboot, so the INPUT knob keeps the right channel
uint8_t loadActiveButton() {
  uint8_t b = prefs.getUChar("btn", 1);
  return b < NUM_BUTTONS && cfg.button[b].type == B_SCENE ? b : 1;
}
void saveActiveButton(uint8_t b) {
  if (prefs.getUChar("btn", 255) != b) prefs.putUChar("btn", b);
}
