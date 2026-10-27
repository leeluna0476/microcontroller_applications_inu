#include "serial.hpp"

void tx_chr(char c) {
  Serial.write(c);
}

void tx_str(const char* s) {
  while (*s) {
    tx_chr(*s);
    ++s;
  }
}

void print(const char* fmt, ...) {
  char buf[BUFSIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  tx_str(buf);
}

char rx_chr(void) {
  char c = '\0';
  if (Serial.available() > 0) {
    c = Serial.read();
  }
  return c;
}

void rx_str(char* buf, size_t bufsize) {
  size_t i = 0;
  while (Serial.available() > 0 && i < bufsize - 1) {
    char c = Serial.read();
    if (c == '\n') {
      break;
    } else if (c == '\r') {
      continue;
    } else {
      buf[i] = c;
      ++i;
    }
  }
  buf[i] = '\0';
}
