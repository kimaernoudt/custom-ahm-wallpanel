// Settings page at http://<panel IP>/ (user "admin", the panel password). Saving reboots the panel.
#pragma once
#include <WebServer.h>

WebServer web(80);
extern String statusText();   // from ahm_panel.ino

const char *TYPE_NAMES[] = {"Off", "Input", "Zone", "Control group", "Input of active scene"};

String esc(const char *s) {
  String o;
  for (; *s; s++) {
    if (*s == '<') o += "&lt;";
    else if (*s == '"') o += "&quot;";
    else if (*s == '&') o += "&amp;";
    else o += *s;
  }
  return o;
}

String field(const char *label, const String &name, const String &value, const char *extra = "") {
  return "<label>" + String(label) + "<input name='" + name + "' value=\"" + value + "\" " + extra + "></label>";
}

bool authorised() {
  if (web.authenticate("admin", cfg.password)) return true;
  web.requestAuthentication(BASIC_AUTH, cfg.room);
  return false;
}

void handleRoot() {
  if (!authorised()) return;
  String h;
  h.reserve(9000);
  h += F("<!doctype html><meta name=viewport content='width=device-width'><title>AHM panel</title><style>"
         "body{font:15px system-ui;background:#17181b;color:#eee;max-width:760px;margin:auto;padding:16px}"
         "fieldset{border:1px solid #444;border-radius:8px;margin:12px 0}label{display:inline-block;margin:4px 12px 4px 0}"
         "input,select{display:block;background:#222;color:#eee;border:1px solid #555;border-radius:4px;padding:4px}"
         "input[type=checkbox]{display:inline}button{padding:8px 18px;margin-top:8px}pre{background:#0f1012;padding:8px}"
         "</style><h2>AHM wall panel: ");
  h += esc(cfg.room);
  h += F("</h2><pre>");
  h += statusText();
  h += F("</pre><form method=post action=/save><fieldset><legend>Panel</legend>");
  h += field("Room name", "room", esc(cfg.room), "maxlength=23");
  h += field("Password (blank = keep)", "pw", "", "type=password maxlength=23");
  h += F("</fieldset><fieldset><legend>Network</legend>");
  h += field("AHM IP address", "ahm", esc(cfg.ahmIp), "maxlength=15");
  h += field("AHM port", "port", String(cfg.ahmPort), "type=number");
  h += "<label><input type=checkbox name=dhcp " + String(cfg.dhcp ? "checked" : "") + "> Panel uses DHCP</label><br>";
  h += field("Panel IP (no DHCP)", "ip", esc(cfg.ip), "maxlength=15");
  h += field("Gateway", "gw", esc(cfg.gateway), "maxlength=15");
  h += field("Subnet", "sn", esc(cfg.subnet), "maxlength=15");
  h += F("</fieldset><fieldset><legend>Knobs, left to right</legend>");
  for (int k = 0; k < 4; k++) {
    String i(k);
    h += "<div>";
    h += field("Label", "kl" + i, esc(cfg.knob[k].label), "maxlength=11 size=8");
    h += "<label>Controls<select name=kt" + i + ">";
    for (int t = 0; t < 5; t++)
      h += "<option value=" + String(t) + (cfg.knob[k].type == t ? " selected>" : ">") + TYPE_NAMES[t] + "</option>";
    h += "</select></label>";
    h += field("Number", "kc" + i, String(cfg.knob[k].ch), "type=number min=1 max=64");
    h += "</div>";
  }
  h += F("</fieldset><fieldset><legend>Scene buttons, left to right</legend>");
  for (int b = 0; b < 4; b++) {
    String i(b);
    h += "<div>";
    h += field("Label", "bl" + i, esc(cfg.button[b].label), "maxlength=11 size=8");
    h += field("Scene (0 = off)", "bs" + i, String(cfg.button[b].scene), "type=number min=0 max=500");
    h += field("Its input (0 = keep)", "bi" + i, String(cfg.button[b].input), "type=number min=0 max=64");
    h += "</div>";
  }
  h += F("</fieldset><fieldset><legend>Feel and LEDs</legend>");
  h += field("Level steps per click", "step", String(cfg.step), "type=number min=1 max=10");
  h += field("Ring brightness (5-255)", "br", String(cfg.brightness), "type=number min=5 max=255");
  h += field("Ring start LED (0-11)", "ro", String(cfg.ringOffset), "type=number min=0 max=11");
  h += "<label><input type=checkbox name=rr " + String(cfg.ringReverse ? "checked" : "") + "> Rings fill anticlockwise</label>";
  h += F("</fieldset><button>Save and restart</button></form>"
         "<p><a href=/defaults style=color:#888>Reset to cafe defaults</a></p>");
  web.send(200, "text/html", h);
}

void copyArg(const char *name, char *dst, size_t n) {
  if (web.hasArg(name)) strlcpy(dst, web.arg(name).c_str(), n);
}

long numArg(const String &name, long lo, long hi, long keep) {
  if (!web.hasArg(name)) return keep;
  return constrain(web.arg(name).toInt(), lo, hi);
}

void restartSoon() {
  delay(500);
  ESP.restart();
}

void handleSave() {
  if (!authorised()) return;
  copyArg("room", cfg.room, sizeof(cfg.room));
  if (web.arg("pw").length()) copyArg("pw", cfg.password, sizeof(cfg.password));
  copyArg("ahm", cfg.ahmIp, sizeof(cfg.ahmIp));
  cfg.ahmPort = numArg("port", 1, 65535, cfg.ahmPort);
  cfg.dhcp = web.hasArg("dhcp");
  copyArg("ip", cfg.ip, sizeof(cfg.ip));
  copyArg("gw", cfg.gateway, sizeof(cfg.gateway));
  copyArg("sn", cfg.subnet, sizeof(cfg.subnet));
  for (int k = 0; k < 4; k++) {
    String i(k);
    copyArg(("kl" + i).c_str(), cfg.knob[k].label, sizeof(cfg.knob[k].label));
    cfg.knob[k].type = numArg("kt" + i, 0, 4, cfg.knob[k].type);
    cfg.knob[k].ch = numArg("kc" + i, 1, 64, cfg.knob[k].ch);
  }
  for (int b = 0; b < 4; b++) {
    String i(b);
    copyArg(("bl" + i).c_str(), cfg.button[b].label, sizeof(cfg.button[b].label));
    cfg.button[b].scene = numArg("bs" + i, 0, 500, cfg.button[b].scene);
    cfg.button[b].input = numArg("bi" + i, 0, 64, cfg.button[b].input);
  }
  cfg.step = numArg("step", 1, 10, cfg.step);
  cfg.brightness = numArg("br", 5, 255, cfg.brightness);
  cfg.ringOffset = numArg("ro", 0, 11, cfg.ringOffset);
  cfg.ringReverse = web.hasArg("rr");
  saveSettings();
  web.send(200, "text/html", "<meta http-equiv=refresh content='8;url=/'>Saved. Restarting...");
  restartSoon();
}

void handleDefaults() {
  if (!authorised()) return;
  setDefaults();
  saveSettings();
  web.send(200, "text/html", "<meta http-equiv=refresh content='8;url=/'>Defaults restored. Restarting...");
  restartSoon();
}

void webBegin() {
  web.on("/", HTTP_GET, handleRoot);
  web.on("/save", HTTP_POST, handleSave);
  web.on("/defaults", HTTP_GET, handleDefaults);
  web.on("/status", HTTP_GET, [] { web.send(200, "text/plain", statusText()); });
  web.begin();
}
