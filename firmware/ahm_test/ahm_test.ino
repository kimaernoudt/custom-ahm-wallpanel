// AHM test: one encoder + one button on an Olimex ESP32-POE
// - Encoder turns TRAP (control group 1) up/down; it sends absolute levels, because the
//   offline System Manager ignores the increment/decrement message
// - Button recalls scene 1 (CAFE START)
// - Everything the AHM sends back is printed as hex in the Serial Monitor (115200)
//
// Arduino IDE: Tools > Board > "OLIMEX ESP32-PoE" (esp32 by Espressif).
// Wiring (encoder common/middle pin and button other leg to GND):
//   encoder A -> GPIO 32, encoder B -> GPIO 33, button -> GPIO 4

#include <ETH.h>

// ---------- settings ----------
IPAddress AHM_IP(192, 168, 100, 91);   // <-- put your AHM's IP address here
const uint16_t AHM_PORT = 51325;

const int PIN_ENC_A = 32;
const int PIN_ENC_B = 33;
const int PIN_BTN   = 4;

const uint8_t TEST_N  = 2;    // 0 = input, 1 = zone, 2 = control group
const uint8_t TEST_CH = 0;    // 0 = number 1 (TRAP is control group 1)
const uint8_t TEST_SCENE = 1; // scene recalled by the button
const int STEPS_PER_CLICK = 4;  // most EC11s; try 2 if it needs two clicks per step
const int LEVEL_STEP = 2;       // level units per click (0..127 scale)
// ------------------------------

NetworkClient ahm;
bool ethUp = false;
int level = -1;   // TEST channel level as last read from the AHM, -1 = not known yet

void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Ethernet up, my IP: ");
      Serial.println(ETH.localIP());
      ethUp = true;
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

void sendBytes(const uint8_t *b, size_t n, const char *what) {
  if (!ahm.connected()) {
    Serial.println("Not connected to AHM, skipped");
    return;
  }
  ahm.write(b, n);
  Serial.printf("-> %s:", what);
  for (size_t i = 0; i < n; i++) Serial.printf(" %02X", b[i]);
  Serial.println();
}

// NRPN 17 = absolute level 0..127
void levelStep(bool up) {
  if (level < 0) {
    Serial.println("Level not read from the AHM yet, skipped");
    return;
  }
  level = constrain(level + (up ? LEVEL_STEP : -LEVEL_STEP), 0, 127);
  uint8_t b[] = {(uint8_t)(0xB0 | TEST_N), 0x63, TEST_CH,
                 (uint8_t)(0xB0 | TEST_N), 0x62, 0x17,
                 (uint8_t)(0xB0 | TEST_N), 0x06, (uint8_t)level};
  sendBytes(b, sizeof(b), up ? "level up" : "level down");
}

// ask the AHM for the TEST channel level; it answers with a level message
void getLevel() {
  uint8_t b[] = {0xF0, 0x00, 0x00, 0x1A, 0x50, 0x12, 0x01, 0x00, TEST_N, 0x01, 0x0B, 0x17, TEST_CH, 0xF7};
  sendBytes(b, sizeof(b), "get level");
}

// watch the bytes from the AHM for the TEST channel level. The AHM uses MIDI running status,
// so a level arrives as  BN 63 CH 62 17 06 LV  (the BN may be repeated), or as SysEx
// F0 00 00 1A 50 12 01 00 0N 01 0B CH LV F7
void watchLevel(uint8_t c) {
  static uint8_t status = 0, data[2], nData = 0, nrpnCh = 0xFF, nrpnParam = 0;
  static uint8_t sysex[16], nSysex = 0;
  static bool inSysex = false;
  int lv = -1;
  if (c == 0xF0) { inSysex = true; nSysex = 0; return; }
  if (inSysex) {
    if (c != 0xF7) { if (nSysex < sizeof(sysex)) sysex[nSysex++] = c; return; }
    inSysex = false;
    if (nSysex >= 12 && sysex[3] == 0x50 && sysex[4] == 0x12 && sysex[7] == TEST_N && sysex[9] == 0x0B &&
        sysex[10] == TEST_CH)
      lv = sysex[11];
  } else if (c & 0x80) {
    status = c; nData = 0;
  } else if (status == (0xB0 | TEST_N)) {
    data[nData++] = c;
    if (nData == 2) {
      nData = 0;
      if (data[0] == 0x63) nrpnCh = data[1];
      else if (data[0] == 0x62) nrpnParam = data[1];
      else if (data[0] == 0x06 && nrpnCh == TEST_CH && nrpnParam == 0x17) lv = data[1];
    }
  }
  if (lv >= 0) {
    level = lv;
    Serial.printf("   TEST level is now %d\n", level);
  }
}

// Bank select + program change; scenes 1..128
void recallScene(uint8_t scene) {
  uint8_t b[] = {0xB0, 0x00, 0x00, 0xC0, (uint8_t)(scene - 1)};
  sendBytes(b, sizeof(b), "recall scene");
}

void ensureConnected() {
  static uint32_t lastTry = 0;
  if (!ethUp || ahm.connected() || millis() - lastTry < 3000) return;
  lastTry = millis();
  Serial.print("Connecting to AHM ");
  Serial.print(AHM_IP);
  Serial.print(":");
  Serial.print(AHM_PORT);
  bool ok = ahm.connect(AHM_IP, AHM_PORT);
  Serial.println(ok ? " ... OK" : " ... failed");
  if (ok) getLevel();
}

// quadrature decoding by polling
int8_t readEncoder() {
  static uint8_t prev = 0;
  static int8_t acc = 0;
  static const int8_t table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  uint8_t cur = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  acc += table[(prev << 2) | cur];
  prev = cur;
  if (acc >= STEPS_PER_CLICK)  { acc = 0; return 1; }
  if (acc <= -STEPS_PER_CLICK) { acc = 0; return -1; }
  return 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT_PULLUP);
  Network.onEvent(onEvent);   // on esp32 core 2.x use: WiFi.onEvent(onEvent);
  ETH.begin();                // board "OLIMEX ESP32-PoE" supplies the PHY settings
  // fixed address on the AHM network (it has no DHCP server); remove this line to use DHCP
  ETH.config(IPAddress(192, 168, 100, 200), IPAddress(192, 168, 100, 1), IPAddress(255, 255, 255, 0));
  Serial.println("AHM test starting");
}

void loop() {
  ensureConnected();

  int8_t d = readEncoder();
  if (d) levelStep(d > 0);

  static bool lastBtn = HIGH;
  static uint32_t lastChange = 0;
  bool btn = digitalRead(PIN_BTN);
  if (btn != lastBtn && millis() - lastChange > 30) {
    lastChange = millis();
    lastBtn = btn;
    if (btn == LOW) recallScene(TEST_SCENE);
  }

  // print whatever the AHM sends back
  if (ahm.connected() && ahm.available()) {
    Serial.print("<- AHM:");
    uint8_t buf[64];
    int n = 0;
    while (ahm.available() && n < (int)sizeof(buf)) buf[n++] = ahm.read();
    for (int i = 0; i < n; i++) Serial.printf(" %02X", buf[i]);
    Serial.println();
    for (int i = 0; i < n; i++) watchLevel(buf[i]);
  }
}
