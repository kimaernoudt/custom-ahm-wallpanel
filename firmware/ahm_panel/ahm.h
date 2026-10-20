// Allen & Heath AHM TCP protocol V1.0: message builders, a stream parser and a self-healing connection.
#pragma once
#include <Arduino.h>
#include <WiFi.h>   // WiFiClient also runs over Ethernet

const uint8_t LEVEL_0DB = 0x69;

class AhmLink {
 public:
  // Called for every level / mute / scene the AHM reports
  void (*onLevel)(uint8_t n, uint8_t ch, uint8_t lv) = nullptr;
  void (*onMute)(uint8_t n, uint8_t ch, bool on) = nullptr;
  void (*onScene)(uint16_t scene) = nullptr;
  void (*onConnect)() = nullptr;

  void begin(const char *ip, uint16_t port) {
    host.fromString(ip);
    this->port = port;
    client.stop();
  }

  bool connected() { return up; }

  // Call often from loop(): reconnects with back-off, reads incoming data, detects a dead link
  void poll(bool networkUp) {
    if (!networkUp) {
      if (up) drop("network down");
      return;
    }
    if (!up) {
      if (lastTry && millis() - lastTry < backoff) return;
      lastTry = millis();
      Serial.printf("AHM: connecting to %s:%u\n", host.toString().c_str(), port);
      if (client.connect(host, port, 2000)) {
        client.setNoDelay(true);
        up = true;
        backoff = 1000;
        lastRx = lastPing = millis();
        resetParser();
        Serial.println("AHM: connected");
        if (onConnect) onConnect();
      } else {
        backoff = backoff >= 5000 ? 10000 : backoff * 2;
        Serial.printf("AHM: connect failed, retry in %u s\n", (unsigned)(backoff / 1000));
      }
      return;
    }
    if (!client.connected()) {
      drop("closed by AHM");
      return;
    }
    while (client.available()) {
      feed(client.read());
      lastRx = millis();
    }
    // Keep-alive: ask for a level every 5 s, give up after 15 s of silence
    if (millis() - lastPing > 5000) {
      lastPing = millis();
      getLevel(pingN, pingCh);
    }
    if (millis() - lastRx > 15000) drop("no answer for 15 s");
  }

  void setPing(uint8_t n, uint8_t ch) { pingN = n; pingCh = ch; }

  void setLevel(uint8_t n, uint8_t ch, uint8_t lv) {
    uint8_t b[] = {(uint8_t)(0xB0 | n), 0x63, ch, (uint8_t)(0xB0 | n), 0x62, 0x17,
                   (uint8_t)(0xB0 | n), 0x06, (uint8_t)(lv & 0x7F)};
    send(b, sizeof(b));
  }

  void setMute(uint8_t n, uint8_t ch, bool on) {
    uint8_t b[] = {(uint8_t)(0x90 | n), ch, (uint8_t)(on ? 0x7F : 0x3F), (uint8_t)(0x90 | n), ch, 0x00};
    send(b, sizeof(b));
  }

  void recallScene(uint16_t scene) {   // 1..500
    uint16_t s = scene - 1;
    uint8_t b[] = {0xB0, 0x00, (uint8_t)(s / 128), 0xC0, (uint8_t)(s % 128)};
    send(b, sizeof(b));
  }

  void getLevel(uint8_t n, uint8_t ch) { getReq(n, 0x0B, ch); }
  void getMute(uint8_t n, uint8_t ch) { getReq(n, 0x09, ch); }

 private:
  WiFiClient client;
  IPAddress host;
  uint16_t port = 51325;
  bool up = false;
  uint32_t lastTry = 0, backoff = 1000, lastRx = 0, lastPing = 0;
  uint8_t pingN = 0, pingCh = 0;

  // parser state
  uint8_t status = 0, data[2], nData = 0, bank = 0;
  bool inSysex = false;
  uint8_t sysex[16], nSysex = 0;
  uint8_t nrpnCh[16], nrpnParam[16];

  void drop(const char *why) {
    Serial.printf("AHM: disconnected (%s)\n", why);
    client.stop();
    up = false;
    lastTry = millis();
  }

  void send(const uint8_t *b, size_t n) {
    if (up && client.write(b, n) != n) drop("write failed");
  }

  void getReq(uint8_t n, uint8_t cmd, uint8_t ch) {
    uint8_t b[] = {0xF0, 0x00, 0x00, 0x1A, 0x50, 0x12, 0x01, 0x00, n, 0x01, cmd, ch, 0xF7};
    send(b, sizeof(b));
  }

  void resetParser() {
    status = nData = nSysex = bank = 0;
    inSysex = false;
    memset(nrpnCh, 0, sizeof(nrpnCh));
    memset(nrpnParam, 0, sizeof(nrpnParam));
  }

  // MIDI-style byte stream, with running status
  void feed(uint8_t b) {
    if (b == 0xF0) { inSysex = true; nSysex = 0; return; }
    if (inSysex) {
      if (b == 0xF7) { inSysex = false; handleSysex(); }
      else if (nSysex < sizeof(sysex)) sysex[nSysex++] = b;
      return;
    }
    if (b & 0x80) { status = b; nData = 0; return; }
    if (!status) return;
    data[nData++] = b;
    uint8_t kind = status & 0xF0, n = status & 0x0F;
    if (kind == 0x90 && nData == 2) {
      if (data[1] && onMute) onMute(n, data[0], data[1] >= 0x40);   // velocity 0 = second half of the message
      nData = 0;
    } else if (kind == 0xB0 && nData == 2) {
      controlChange(n, data[0], data[1]);
      nData = 0;
    } else if (kind == 0xC0 && nData == 1) {
      if (onScene) onScene(bank * 128 + data[0] + 1);
      nData = 0;
    } else if (nData >= 2) {
      nData = 0;
    }
  }

  void controlChange(uint8_t n, uint8_t cc, uint8_t val) {
    if (cc == 0x00) { bank = val; return; }
    if (cc == 0x63) { nrpnCh[n] = val; return; }
    if (cc == 0x62) { nrpnParam[n] = val; return; }
    if (cc == 0x06 && nrpnParam[n] == 0x17 && onLevel) onLevel(n, nrpnCh[n], val);
  }

  // Get replies may come back as SysEx: 00 00 1A 50 12 01 00 | N 01 cmd CH value
  void handleSysex() {
    if (nSysex < 12 || sysex[3] != 0x50 || sysex[4] != 0x12) return;
    uint8_t n = sysex[7], cmd = sysex[9], ch = sysex[10], val = sysex[11];
    if (cmd == 0x0B && onLevel) onLevel(n, ch, val);
    else if (cmd == 0x09 && onMute) onMute(n, ch, val >= 0x40);
  }
};
