#pragma once
#include "Arduino.h"
enum { HTTP_GET, HTTP_POST };
enum { BASIC_AUTH };
class WebServer {
 public:
  std::map<std::string, std::function<void()>> routes;
  std::map<std::string, std::string> args;
  std::string lastBody; int lastCode = 0;
  WebServer(int) {}
  void on(const char *p, int, std::function<void()> f) { routes[p] = f; }
  void begin() {} void handleClient() {}
  bool authenticate(const char *, const char *) { return true; }
  void requestAuthentication(int, const char *) {}
  void send(int c, const char *, const String &b) { lastCode = c; lastBody = b.s; }
  bool hasArg(const char *n) { return args.count(n); }
  bool hasArg(const String &n) { return args.count(n.s); }
  String arg(const char *n) { return String(args.count(n) ? args[n] : ""); }
  String arg(const String &n) { return arg(n.c_str()); }
};
