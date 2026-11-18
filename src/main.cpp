#include <Arduino.h>
#include "buzz.hpp"
#include "ultrasonic.hpp"

#define BUZZ 11
#define COMPA (byte)17

#define ECHO 12
#define TRIG 13
#define TIMEOUT_US 25000UL // 25ms
#define PRESCALER 256
#define FREQ 220

unsigned long calc_speed(float d);
void control_buzzer(float d);

void setup(void) {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(BUZZ, OUTPUT);
  setup_timer2a_ctc(false, PRESCALER, FREQ);
}

void loop(void) {
  float d = read_distance(TRIG, ECHO, TIMEOUT_US);
  control_buzzer(d);
  delay(100);
}

// range: [100, 1000]
unsigned long calc_speed(float d) {
  if (d < 10. || d > 30.) {
    return 0;
  }
  return 100 + ((unsigned long)d - 10) * 45;
}

void control_buzzer(float d) {
  static uint8_t prev_state = 0;
  static bool on = false;
  static unsigned long speed = 0;
  static unsigned long last_ms = 0;

  if (d < 0.) {
    return;
  } else if (d < 10.) {
    if (prev_state) {
      on = true;
      set_oc(&TCCR2A, on);
      prev_state = 0;
    }
  } else {
    if (d <= 30.) {
      speed = calc_speed(d);
      if (millis() - last_ms >= speed) {
        on = !on;
        set_oc(&TCCR2A, on);
        last_ms = millis();
        prev_state = 1;
      }
    } else {
      if (prev_state != 2) {
        on = false;
        set_oc(&TCCR2A, on);
        prev_state = 2;
      }
    }
  }
}
