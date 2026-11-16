#include <Arduino.h>
#include <avr/interrupt.h>

#define FIRST_LED 11
#define LED_NO 3
#include "led.hpp"

#define COMPA (byte)78

volatile byte ctc_cnt = 0;
ISR(TIMER2_COMPA_vect) { // called every 5ms
  if (ctc_cnt == 0) {
    led_write((byte)(~ALL_OFF));
  } else if (ctc_cnt == 60) {
    led_write((byte)(~ALL_ON));
  }
  ctc_cnt = (ctc_cnt + 1) % 200; // 1 period == 5ms*200 == 1s
}

void setup(void) {
  setup_led();
  led_write((byte)(~ALL_ON)); // active high

  noInterrupts();
  TCCR2A = (1 << WGM21); // CTC: Timer/Counter Top == OCR2A, default == 0xFF
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // frequency prescaler 1024
  TCNT2 = 0;                                        // Timer/Counter Bottom
  OCR2A = COMPA;                                    // TCNT2 is compared with this value
  TIFR2 = (1 << OCF2A);                             // initialize COMPA flag
  TIMSK2 = (1 << OCIE2A);                           // set interrupt on COMPA
  interrupts();
}

void loop(void) {}
