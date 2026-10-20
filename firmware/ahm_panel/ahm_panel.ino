// AHM wall panel firmware for the Olimex ESP32-POE
//
// 4 encoders with LED rings set levels and mute; 4 lit buttons recall scenes on an Allen & Heath AHM
// over its TCP control protocol (port 51325). The rings follow level and mute changes made elsewhere.
//
// Arduino IDE: board "OLIMEX ESP32-PoE" (esp32 by Espressif, 2.x or 3.x), library "Adafruit NeoPixel".
// First boot uses the cafe defaults in settings.h. After that, set the panel up for any room at
// http://<panel IP>/ (user "admin", password "ahmpanel" until you change it). The panel's IP is printed
// on the Serial Monitor (115200) and shown by your router. Updates can be uploaded over the network
// (Arduino IDE > Port > network port, same password).
// Hold the Olimex BUT1 button while powering up to restore the defaults.

#include <ETH.h>
#include <ArduinoOTA.h>
#include "settings.h"
#include "ahm.h"
#include "panel_io.h"
#include "web.h"

const int PIN_BUT1 = 34;   // Olimex on-board button

AhmLink ahm;
bool ethUp = false;

struct Knob {
  int8_t n = -1;          // AHM channel type, -1 = off
  uint8_t ch = 0;         // 0-based channel
  int16_t level = -1;     // -1 = not known yet
  int8_t mute = -1;       // -1 = not known yet
  bool pending = false;   // level changed locally, not sent yet
  uint32_t lastSend = 0, lastTurn = 0;
} knobs[4];

uint8_t activeButton = 0;
uint8_t sceneInput = 1;          // 1-based input for the "input of active scene" knob
uint32_t resyncAt = 0;           // re-read all levels at this time (after a scene recall)
bool ringsDirty = true;

// ---------- mapping ----------

void retarget() {
  for (int k = 0; k < 4; k++) {
    const KnobCfg &c = cfg.knob[k];
    int8_t n = ahmType(c.type);
    uint8_t ch = (c.type == K_SCENE_INPUT ? sceneInput : c.ch) - 1;
    Knob &kn = knobs[k];
    if (n != kn.n || ch != kn.ch) {
      kn.n = n;
      kn.ch = ch;
      kn.level = kn.mute = -1;
      kn.pending = false;
      if (n >= 0 && ahm.connected()) {
        ahm.getLevel(n, ch);
        ahm.getMute(n, ch);
      }
    }
  }
  if (knobs[0].n >= 0) ahm.setPing(knobs[0].n, knobs[0].ch);   // keep-alive asks for a level we use
  ringsDirty = true;
}

void activate(uint8_t b) {
  activeButton = b;
  if (cfg.button[b].input) sceneInput = cfg.button[b].input;
  saveActiveButton(b);
  retarget();
}

void requestAll() {
  for (auto &kn : knobs) {
    if (kn.n < 0) continue;
    ahm.getLevel(kn.n, kn.ch);
    ahm.getMute(kn.n, kn.ch);
  }
}

// ---------- AHM events ----------

void onLevel(uint8_t n, uint8_t ch, uint8_t lv) {
  for (auto &kn : knobs) {
    if (kn.n != n || kn.ch != ch) continue;
    // while the knob is being turned it wins over (possibly older) reports
    if (kn.level >= 0 && (kn.pending || (kn.lastSend && millis() - kn.lastSend < 300))) continue;
    kn.level = lv;
    ringsDirty = true;
  }
}

void onMute(uint8_t n, uint8_t ch, bool on) {
  for (auto &kn : knobs)
    if (kn.n == n && kn.ch == ch) { kn.mute = on; ringsDirty = true; }
}

void onScene(uint16_t scene) {
  for (int b = 0; b < 4; b++)
    if (cfg.button[b].scene == scene) activate(b);
  resyncAt = millis() + 300;
}

void onConnect() {
  requestAll();
  ringsDirty = true;
}

// ---------- network ----------

void onNetEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START: {
      String host = "ahm-panel-" + String(cfg.room);
      host.replace(' ', '-');
      ETH.setHostname(host.c_str());
      break;
    }
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("Ethernet up: http://%s/\n", ETH.localIP().toString().c_str());
      ethUp = true;
      ArduinoOTA.end();
      ArduinoOTA.setHostname(ETH.getHostname());
      ArduinoOTA.setPassword(cfg.password);
      ArduinoOTA.begin();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet down");
      ethUp = false;
      break;
    default:
      break;
  }
}

void ethBegin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  Network.onEvent(onNetEvent);
  ETH.begin(ETH_PHY_LAN8720, 0, 23, 18, 12, ETH_CLOCK_GPIO17_OUT);
#else
  WiFi.onEvent(onNetEvent);
  ETH.begin(0, 12, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT);
#endif
  if (!cfg.dhcp) {
    IPAddress ip, gw, sn;
    if (ip.fromString(cfg.ip) && gw.fromString(cfg.gateway) && sn.fromString(cfg.subnet))
      ETH.config(ip, gw, sn, gw);
  }
}

String statusText() {
  String s;
  s += "Panel IP:  " + (ethUp ? ETH.localIP().toString() : String("no network")) + "\n";
  s += "AHM:       " + String(cfg.ahmIp) + ":" + String(cfg.ahmPort) + (ahm.connected() ? "  connected" : "  NOT connected") + "\n";
  s += "I/O board: " + String(mcpOk ? "ok" : "NOT found") + "\n";
  s += "Scene:     " + String(cfg.button[activeButton].label) + " (scene " + String(cfg.button[activeButton].scene) + ")\n";
  const char *types[] = {"Input", "Zone", "CG"};
  for (int k = 0; k < 4; k++) {
    const Knob &kn = knobs[k];
    s += String(cfg.knob[k].label) + ": ";
    if (kn.n < 0) { s += "off\n"; continue; }
    s += String(types[kn.n]) + " " + String(kn.ch + 1) + "  level " + (kn.level < 0 ? String("?") : String(kn.level)) +
         (kn.mute == 1 ? "  MUTED" : "") + "\n";
  }
  s += "Uptime:    " + String(millis() / 60000) + " min\n";
  return s;
}

// ---------- LEDs ----------

void drawRings() {
  static uint32_t lastDraw = 0;
  bool animate = !ahm.connected();
  if (millis() - lastDraw < 20 || (!ringsDirty && !animate)) return;
  lastDraw = millis();
  ringsDirty = false;
  rings.clear();
  for (int k = 0; k < 4; k++) {
    const Knob &kn = knobs[k];
    auto px = [&](int p, uint32_t c) { rings.setPixelColor(ringPixel(k, p, cfg.ringOffset, cfg.ringReverse), c); };
    if (kn.n < 0) continue;
    if (animate) {   // waiting for the AHM: slow amber dot running round
      px((millis() / 150) % LEDS_PER_RING, rings.Color(120, 60, 0));
      continue;
    }
    if (kn.mute == 1) {   // muted: whole ring red
      for (int p = 0; p < LEDS_PER_RING; p++) px(p, rings.Color(255, 0, 0));
      continue;
    }
    if (kn.level < 0) { px(0, rings.Color(30, 30, 30)); continue; }
    float lit = kn.level * LEDS_PER_RING / 127.0f;
    for (int p = 0; p < LEDS_PER_RING; p++) {
      float f = constrain(lit - p, 0.0f, 1.0f);
      if (f > 0) px(p, rings.Color(0, (uint8_t)(200 * f), (uint8_t)(180 * f)));
    }
  }
  rings.show();

  bool blink = (millis() / 500) % 2;
  setButtonLeds(ahm.connected() || blink ? 1 << activeButton : 0);
}

// ---------- main ----------

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUT1, INPUT);
  loadSettings();
  if (digitalRead(PIN_BUT1) == LOW) {
    Serial.println("BUT1 held: restoring defaults");
    setDefaults();
    saveSettings();
  }
  activeButton = loadActiveButton();
  sceneInput = cfg.button[activeButton].input ? cfg.button[activeButton].input : 1;
  Serial.printf("\nAHM panel '%s', AHM at %s\n", cfg.room, cfg.ahmIp);

  ioBegin();
  ringsBegin(cfg.brightness);
  ethBegin();
  webBegin();

  ahm.onLevel = onLevel;
  ahm.onMute = onMute;
  ahm.onScene = onScene;
  ahm.onConnect = onConnect;
  ahm.begin(cfg.ahmIp, cfg.ahmPort);
  retarget();
  enableLoopWDT();   // restarts the panel if loop() ever hangs for 5 s
}

void loop() {
  ahm.poll(ethUp);
  web.handleClient();
  if (ethUp) ArduinoOTA.handle();

  // knobs
  for (int k = 0; k < 4; k++) {
    Knob &kn = knobs[k];
    int d = takeEncoder(k);
    if (kn.n < 0 || !ahm.connected()) continue;
    if (d) {
      if (kn.level < 0) {
        ahm.getLevel(kn.n, kn.ch);   // don't guess: wait for the real level
      } else {
        uint32_t dt = millis() - kn.lastTurn;
        int accel = dt < 40 ? 3 : dt < 90 ? 2 : 1;   // turning fast moves further
        kn.level = constrain(kn.level + d * cfg.step * accel, 0, 127);
        kn.pending = true;
        ringsDirty = true;
      }
      kn.lastTurn = millis();
    }
    if (kn.pending && millis() - kn.lastSend >= 40) {   // at most 25 messages a second per knob
      ahm.setLevel(kn.n, kn.ch, kn.level);
      kn.pending = false;
      kn.lastSend = millis();
    }
  }

  // knob presses: mute toggle
  uint8_t pushes = takePushes();
  for (int k = 0; k < 4; k++) {
    Knob &kn = knobs[k];
    if (!(pushes & (1 << k)) || kn.n < 0 || !ahm.connected()) continue;
    kn.mute = kn.mute == 1 ? 0 : 1;
    ahm.setMute(kn.n, kn.ch, kn.mute);
    ringsDirty = true;
  }

  // scene buttons
  int b = pollLadder();
  if (b >= 0 && cfg.button[b].scene && ahm.connected()) {
    ahm.recallScene(cfg.button[b].scene);
    activate(b);
    resyncAt = millis() + 300;
  }

  if (resyncAt && millis() > resyncAt) {
    resyncAt = 0;
    requestAll();
  }

  drawRings();
}
