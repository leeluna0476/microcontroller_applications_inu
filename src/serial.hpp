#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <Arduino.h>
#include <stdio.h>

#ifndef BUFSIZE
#define BUFSIZE 96
#endif

void tx_chr(char c);
void tx_str(const char* s);
void print(const char* s, ...);

char rx_chr(void);
void rx_str(char* buf, size_t bufsize);

#endif
