#include "../../firmware/ahm_panel/ahm_panel.ino"

void run(uint32_t ms) { uint32_t end = millis() + ms; while (millis() < end) { loop(); delay(1); } }
void setBit(int bit, bool high) { uint16_t v = sim_pins; v = high ? (v | (1 << bit)) : (v & ~(1 << bit)); sim_pins = v; }
void turn(int k, int clicks) {   // + = clockwise
  for (int c = 0; c < abs(clicks); c++) {
    int seq[4][2] = {{0, 1}, {0, 0}, {1, 0}, {1, 1}};   // (A,B) after 11
    for (int i = 0; i < 4; i++) {
      int a = clicks > 0 ? seq[i][0] : seq[i][1], b = clicks > 0 ? seq[i][1] : seq[i][0];
      setBit(ENC_A[k], a); setBit(ENC_B[k], b); run(4);
    }
    run(120);   // slow turning: no acceleration
  }
}
void push(int k) { setBit(PUSH[k], 0); run(60); setBit(PUSH[k], 1); run(60); }
void ladder(uint32_t mv) { sim_ladder_mv = mv; run(80); sim_ladder_mv = 3300; run(80); }

int main(int argc, char **argv) {
  setup();
  strcpy(cfg.ahmIp, "127.0.0.1");
  cfg.ahmPort = atoi(argv[1]);
  ahm.begin(cfg.ahmIp, cfg.ahmPort);
  Network.cb(ARDUINO_EVENT_ETH_START);
  Network.cb(ARDUINO_EVENT_ETH_GOT_IP);
  run(1500);
  printf("--- status after connect\n%s", statusText().c_str());
  printf("--- turn TRAP +3, SAS -2\n"); turn(1, 3); turn(2, -2); run(200);
  printf("--- press SAS (mute)\n"); push(2); run(100);
  printf("SAS ring LED0 colour %06X (expect FF0000)\n", rings.px[2 * 12]);
  printf("TRAP ring lit LEDs: "); for (int i = 0; i < 12; i++) printf("%c", rings.px[12 + i] ? '#' : '.'); printf("\n");
  printf("--- press DJ button\n"); ladder(1050); run(600);
  printf("button LEDs olat %04X (expect bit14 = DJ)\n", (unsigned)sim_olat);
  printf("--- turn INPUT +1\n"); turn(0, 1); run(200);
  printf("--- status\n%s", statusText().c_str());
  if (argc > 2) {   // reconnect test: the harness kills and restarts the emulator now
    printf("--- waiting for emulator restart\n"); fflush(stdout);
    run(9000);
    printf("--- after restart: %s\n", ahm.connected() ? "connected" : "NOT connected");
  }
  printf("--- web save\n");
  web.args = {{"room", "Bar"}, {"ahm", "10.0.0.5"}, {"port", "51325"}, {"kl0", "MIC"}, {"kt0", "1"}, {"kc0", "7"}, {"bs0", "12"}, {"step", "99"}};
  web.routes["/save"]();
  printf("room=%s ahm=%s knob0=%s type %d ch %d button0 scene %d step %d\n", cfg.room, cfg.ahmIp, cfg.knob[0].label, cfg.knob[0].type, cfg.knob[0].ch, cfg.button[0].scene, cfg.step);
  web.routes["/"]();
  printf("page bytes %zu, has MIC: %d\n", web.lastBody.size(), (int)(web.lastBody.find("MIC") != std::string::npos));
  return 0;
}
