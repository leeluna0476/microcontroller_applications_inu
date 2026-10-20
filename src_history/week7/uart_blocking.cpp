#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define SW_1 10
#define SW_2 11
#define SW_3 12

#define BAUD_RATE 103 // 9600bps

volatile byte sw_event = 0x00;
volatile byte prev_pb = 0x1C;
volatile unsigned long last_press = 0;

ISR(PCINT0_vect) {
  byte now = PINB & 0x1C;
  byte edge = prev_pb & ~now;
  prev_pb = now;

  if (millis() - last_press < 200) {
    return;
  }
  last_press = millis();
  sw_event |= edge >> 2;
}

void uart_init(void) {
  // baud rate register # high, low
  UBRR0H = (unsigned char)BAUD_RATE >> 8;
  UBRR0L = (unsigned char)BAUD_RATE;

  // control and status register # A B C
  UCSR0B = (1 << TXEN0) | (1 << RXEN0);   // enables write(tx)/read(rx)
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // number of data bits (011=>8bits)
}

void tx_chr(char c) {
  while (!((UCSR0A >> UDRE0) & 1)) {
  }
  UDR0 = c;
}

void tx_str(const char* s) {
  while (*s) {
    tx_chr(*s);
    ++s;
  }
}

#define BUFSIZE 96
void Printf(const char* fmt, ...) {
  char buf[BUFSIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  tx_str(buf);
}

void setup(void) {
  pinMode(SW_1, INPUT_PULLUP);
  pinMode(SW_2, INPUT_PULLUP);
  pinMode(SW_3, INPUT_PULLUP);

  PCICR |= 1 << PCIE0;
  PCMSK0 |= (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4);

  uart_init();
  delay(200);
  Printf("Clock speed = %lu\n", F_CPU);
  Printf("PCMSK0      = 0x%02X\n", PCMSK0);
}

void loop(void) {
  noInterrupts();
  byte event = sw_event;
  sw_event = 0x00;
  prev_pb = 0x1C;
  interrupts();

  if (event & 0x01) {
    Printf("INT%d PUSHED\n", 0);
  }
  if (event & 0x02) {
    Printf("INT%d PUSHED\n", 1);
  }
}
