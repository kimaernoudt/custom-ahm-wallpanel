// AHM test: one encoder + one button on an Olimex ESP32-POE
// - Encoder turns TRAP (control group 1) up/down, 1 step per click
// - Button recalls scene 1 (CAFE START)
// - Everything the AHM sends back is printed as hex in the Serial Monitor (115200)
//
// Arduino IDE: Tools > Board > "OLIMEX ESP32-PoE" (esp32 by Espressif).
// Wiring (encoder common/middle pin and button other leg to GND):
//   encoder A -> GPIO 32, encoder B -> GPIO 33, button -> GPIO 4

#include <ETH.h>

// ---------- settings ----------
IPAddress AHM_IP(192, 168, 1, 100);   // <-- put your AHM's IP address here
const uint16_t AHM_PORT = 51325;

const int PIN_ENC_A = 32;
const int PIN_ENC_B = 33;
const int PIN_BTN   = 4;

const uint8_t TEST_N  = 2;    // 0 = input, 1 = zone, 2 = control group
const uint8_t TEST_CH = 0;    // 0 = number 1 (TRAP is control group 1)
const uint8_t TEST_SCENE = 1; // scene recalled by the button
const int STEPS_PER_CLICK = 4;  // most EC11s; try 2 if it needs two clicks per step
// ------------------------------

WiFiClient ahm;
bool ethUp = false;

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

// NRPN 20 = increment/decrement level by one step
void levelStep(bool up) {
  uint8_t b[] = {(uint8_t)(0xB0 | TEST_N), 0x63, TEST_CH,
                 (uint8_t)(0xB0 | TEST_N), 0x62, 0x20,
                 (uint8_t)(0xB0 | TEST_N), 0x06, (uint8_t)(up ? 0x7F : 0x3F)};
  sendBytes(b, sizeof(b), up ? "level up" : "level down");
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
  Serial.println(ahm.connect(AHM_IP, AHM_PORT) ? " ... OK" : " ... failed");
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
    while (ahm.available()) Serial.printf(" %02X", ahm.read());
    Serial.println();
  }
}
