#pragma once
#include "Arduino.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
class WiFiClient {
  int fd = -1;
  int peeked = -1;
 public:
  bool connect(IPAddress ip, uint16_t port, int timeout) {
    stop();
    fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(port);
    memcpy(&a.sin_addr, ip.b, 4);
    if (::connect(fd, (sockaddr *)&a, sizeof(a)) != 0) { stop(); return false; }
    fcntl(fd, F_SETFL, O_NONBLOCK);
    return true;
  }
  void setNoDelay(bool) {}
  bool connected() {
    if (fd < 0) return false;
    if (peeked >= 0) return true;
    uint8_t c; int r = recv(fd, &c, 1, MSG_PEEK);
    if (r == 0) return false;
    if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return false;
    return true;
  }
  int available() { if (fd < 0) return 0; uint8_t c; int r = recv(fd, &c, 1, MSG_PEEK); return r > 0 ? 1 : 0; }
  int read() { uint8_t c; return recv(fd, &c, 1, 0) == 1 ? c : -1; }
  size_t write(const uint8_t *b, size_t n) { return fd < 0 ? 0 : ::send(fd, b, n, MSG_NOSIGNAL); }
  void stop() { if (fd >= 0) close(fd); fd = -1; }
};
