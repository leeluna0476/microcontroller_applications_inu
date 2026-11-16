#include <Arduino.h>
#include <avr/interrupt.h>

#define FIRST_LED 11
#define LED_NO 3
#include "led.hpp"

#define TCNT_START (byte)177

volatile uint16_t tov_cnt = 0;
ISR(TIMER2_OVF_vect) { // called every 5ms
  TCNT2 = TCNT_START;
  if (tov_cnt == 0) {
    led_write((byte)(~ALL_OFF));
  } else if (tov_cnt == 200) {
    led_write((byte)(~ALL_ON));
  }
  tov_cnt = (tov_cnt + 1) % 400;
}

void setup(void) {
  setup_led();
  led_write((byte)(~ALL_ON)); // active high

  noInterrupts();
  TCCR2A = 0x00;
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // f/1024
  TCNT2 = TCNT_START;
  TIFR2 = (1 << TOV2);
  TIMSK2 = (1 << TOIE2);

  interrupts();
}

void loop(void) {}
