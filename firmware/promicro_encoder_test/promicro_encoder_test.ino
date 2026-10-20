// Pro Micro encoder test: one EC11 encoder, sent over USB to tools/serial_bridge.py.
//
// Wiring (encoder pins facing you, 3 pins on one side, 2 on the other):
//   A (left)   -> pin 2       C (middle) -> GND       B (right) -> pin 3
//   switch pin -> pin 4       other switch pin -> GND
// No resistors needed: the internal pull-ups are used.
//
// Board: "Arduino Leonardo" (or "SparkFun Pro Micro" 5V/16MHz).
// Each detent prints "+" or "-", a press prints "P". Open the Serial Monitor (115200) to see it.

const int PIN_A = 2, PIN_B = 3, PIN_SW = 4;

// Quadrature table: index = (old state << 2) | new state
const int8_t STEPS[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
uint8_t state;
int8_t count;
bool lastSw = HIGH;
unsigned long swTime;

void setup() {
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.begin(115200);
  state = (digitalRead(PIN_A) << 1) | digitalRead(PIN_B);
}

void loop() {
  uint8_t now = (digitalRead(PIN_A) << 1) | digitalRead(PIN_B);
  if (now != state) {
    count += STEPS[(state << 2) | now];
    state = now;
    if (now == 0b11) {            // detent reached (EC11 rests with A and B high)
      if (count >= 2) Serial.println("+");
      else if (count <= -2) Serial.println("-");
      count = 0;
    }
  }

  bool sw = digitalRead(PIN_SW);
  if (sw != lastSw && millis() - swTime > 30) {   // 30 ms debounce
    swTime = millis();
    lastSw = sw;
    if (sw == LOW) Serial.println("P");
  }
}
