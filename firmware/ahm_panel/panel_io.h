// Panel hardware: MCP23017 (encoders, push switches, button LEDs), resistor-ladder scene buttons, WS2812 rings.
// Pin-out matches wiring/WIRING.md (hand-wired) or pcb/README.md (PCB).
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// 0 = hand-wired panel (ring modules, button LEDs on the MCP23017, ladder on GPIO33)
// 1 = panel PCB (rings and button LEDs in one WS2812 chain, ladder on GPIO36 via UEXT pin 4)
#ifndef PANEL_PCB
#define PANEL_PCB 0
#endif

const int PIN_SDA = 13, PIN_SCL = 16, PIN_LEDS = 4, PIN_LADDER = PANEL_PCB ? 36 : 33;
const uint8_t MCP_ADDR = 0x20;
const int LEDS_PER_RING = 12;

// MCP23017 registers (IOCON.BANK = 0)
const uint8_t IODIRA = 0x00, IODIRB = 0x01, GPPUA = 0x0C, GPPUB = 0x0D, GPIOA = 0x12, OLATA = 0x14;

// Bit positions in the 16-bit port value (GPA = bits 0-7, GPB = bits 8-15)
const uint8_t ENC_A[4] = {0, 2, 4, 8};     // INPUT, TRAP, SAS, TOOG
const uint8_t ENC_B[4] = {1, 3, 5, 9};
const uint8_t PUSH[4] = {6, 10, 11, 12};
const uint8_t BTN_LED[4] = {7, 13, 14, 15}; // START, IPAD, DJ, EXTRA
const uint16_t OUTPUT_MASK = (1 << 7) | (1 << 13) | (1 << 14) | (1 << 15);
const uint8_t IN_A = (uint8_t)~OUTPUT_MASK, IN_B = (uint8_t)(~OUTPUT_MASK >> 8);   // input bits per port

// ---- shared between the I/O task and loop() ----
portMUX_TYPE ioMux = portMUX_INITIALIZER_UNLOCKED;
volatile int16_t encDelta[4];      // detents since last read
volatile uint8_t pushEvents;       // bit per knob, set on press
volatile uint16_t ledWanted;       // button LED bits to write
volatile bool mcpOk = false;

bool mcpWrite(uint8_t reg, uint8_t a, uint8_t b) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  Wire.write(a);
  Wire.write(b);
  return Wire.endTransmission() == 0;
}

bool mcpInit() {
  return mcpWrite(IODIRA, IN_A, IN_B) &&    // 1 = input
         mcpWrite(GPPUA, IN_A, IN_B) &&     // pull-ups on the inputs
         mcpWrite(OLATA, 0, 0);
}

bool mcpRead(uint16_t &v) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(GPIOA);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MCP_ADDR, (uint8_t)2) != 2) return false;
  uint8_t a = Wire.read(), b = Wire.read();
  v = a | (b << 8);
  return true;
}

// Polls the MCP every millisecond: quadrature decoding, push debouncing, LED output.
// Owns the I2C bus, so loop() never touches Wire.
void ioTask(void *) {
  static const int8_t STEPS[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  uint8_t state[4] = {3, 3, 3, 3};
  int8_t count[4] = {0};
  uint16_t pushStable = 0xFFFF, pushLast = 0xFFFF;
  uint32_t pushSince[4] = {0};
  uint16_t ledOut = 0xFFFF;
  uint32_t retryAt = 0;

  for (;;) {
    vTaskDelay(1);
    if (!mcpOk) {
      if (millis() >= retryAt) {
        mcpOk = mcpInit();
        ledOut = 0xFFFF;
        retryAt = millis() + 1000;
        if (!mcpOk) Serial.println("IO: MCP23017 not answering, retrying");
      }
      continue;
    }
    uint16_t v;
    if (!mcpRead(v)) { mcpOk = false; continue; }

    for (int k = 0; k < 4; k++) {
      uint8_t now = (((v >> ENC_A[k]) & 1) << 1) | ((v >> ENC_B[k]) & 1);
      if (now != state[k]) {
        count[k] += STEPS[(state[k] << 2) | now];
        state[k] = now;
        if (now == 3) {   // EC11 detent: A and B both high
          int8_t d = count[k] >= 2 ? 1 : count[k] <= -2 ? -1 : 0;
          count[k] = 0;
          if (d) {
            portENTER_CRITICAL(&ioMux);
            encDelta[k] += d;
            portEXIT_CRITICAL(&ioMux);
          }
        }
      }
      // push switch: 20 ms debounce, active low
      uint16_t bit = 1 << PUSH[k];
      if ((v & bit) != (pushLast & bit)) { pushLast ^= bit; pushSince[k] = millis(); }
      else if ((v & bit) != (pushStable & bit) && millis() - pushSince[k] > 20) {
        pushStable ^= bit;
        if (!(v & bit)) {
          portENTER_CRITICAL(&ioMux);
          pushEvents |= 1 << k;
          portEXIT_CRITICAL(&ioMux);
        }
      }
    }

    uint16_t want = ledWanted;
    if (want != ledOut) {
      if (mcpWrite(OLATA, want & 0xFF, want >> 8)) ledOut = want;
      else mcpOk = false;
    }
  }
}

void ioBegin() {
  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  analogSetPinAttenuation(PIN_LADDER, ADC_11db);
  xTaskCreatePinnedToCore(ioTask, "io", 4096, nullptr, 2, nullptr, 0);
}

int takeEncoder(int k) {
  portENTER_CRITICAL(&ioMux);
  int d = encDelta[k];
  encDelta[k] = 0;
  portEXIT_CRITICAL(&ioMux);
  return d;
}

uint8_t takePushes() {
  portENTER_CRITICAL(&ioMux);
  uint8_t p = pushEvents;
  pushEvents = 0;
  portEXIT_CRITICAL(&ioMux);
  return p;
}

// ---- LED rings (+ button LEDs on the PCB: pixels 48..51 = EXTRA, DJ, IPAD, START) ----
const int NUM_PIXELS = 4 * LEDS_PER_RING + (PANEL_PCB ? 4 : 0);
// The Olimex 5 V rail gives 2 A. All 48 ring LEDs fully lit at 255 draw ~1.4 A plus the ESP32 and button LEDs,
// so brightness is capped at 160 (~0.9 A worst case).
const uint8_t MAX_BRIGHTNESS = 160;
Adafruit_NeoPixel rings(NUM_PIXELS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

void setButtonLeds(uint8_t mask) {   // bit per button; on the PCB this only fills the pixel buffer, show() sends it
#if PANEL_PCB
  for (int i = 0; i < 4; i++)        // lit = active scene, the rest glow faintly so they can be found in the dark
    rings.setPixelColor(4 * LEDS_PER_RING + 3 - i, mask & (1 << i) ? rings.Color(255, 255, 255) : rings.Color(12, 12, 12));
  return;
#endif
  uint16_t v = 0;
  for (int i = 0; i < 4; i++)
    if (mask & (1 << i)) v |= 1 << BTN_LED[i];
  ledWanted = v;
}

// Scene buttons on one analog pin. Returns the button (0..3) on a new press, -1 otherwise. Call every few ms.
// Hand-wired: 10k pull-up, 0 / 2.2k / 4.7k / 10k -> 0, 0.59, 1.05, 1.65 V, idle 3.3 V.
// PCB: the Olimex's own 10k pull-up and Schottky diode on UEXT pin 4, 0 / 3.3k / 8.2k / 22k
//      -> about 0.05-0.3, 0.86-1.04, 1.51-1.65, 2.28-2.36 V (diode drop included), idle 3.3 V.
#if PANEL_PCB
const uint16_t LADDER_MV[4] = {550, 1250, 1950, 2800};
#else
const uint16_t LADDER_MV[4] = {300, 820, 1350, 2400};
#endif
int pollLadder() {
  static int last = -1, stable = -1, same = 0;
  static uint32_t lastRead = 0;
  if (millis() - lastRead < 5) return -1;
  lastRead = millis();
  uint32_t mv = analogReadMilliVolts(PIN_LADDER);
  int b = -1;
  for (int i = 3; i >= 0; i--)
    if (mv < LADDER_MV[i]) b = i;
  if (b != last) { last = b; same = 0; return -1; }
  if (++same < 4 || b == stable) return -1;   // 4 equal readings = 20 ms
  stable = b;
  return b;
}

void ringsBegin(uint8_t brightness) {
  rings.begin();
  rings.setBrightness(brightness < MAX_BRIGHTNESS ? brightness : MAX_BRIGHTNESS);   // older saved settings may be higher
  rings.clear();
  rings.show();
}

// LED index for position p (0 = minimum) on ring k
int ringPixel(int k, int p, uint8_t offset, bool reverse) {
  int pos = reverse ? (LEDS_PER_RING - p) % LEDS_PER_RING : p;
  return k * LEDS_PER_RING + (pos + offset) % LEDS_PER_RING;
}
