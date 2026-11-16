#include <Arduino.h>
#include <avr/interrupt.h>

// #define FIRST_LED 11
// #define LED_NO 3
// #include "led.hpp"

#define BUZZ 11

#define COMPA (byte)17 // f=440Hz, OCR2A=17

void setup(void) {
  pinMode(BUZZ, OUTPUT);
  noInterrupts();
  TCCR2A = (1 << COM2A0) | (1 << WGM21); // CTC: Timer/Counter Top == OCR2A, default == 0xFF
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // frequency prescaler 64
  TCNT2 = 0;                                        // Timer/Counter Bottom
  OCR2A = COMPA;                                    // TCNT2 is compared with this value
  TIFR2 = (1 << OCF2A);                             // initialize COMPA flag
  interrupts();
}

void loop(void) {}
