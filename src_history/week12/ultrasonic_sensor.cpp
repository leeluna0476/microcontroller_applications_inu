#include <Arduino.h>
#include "led.hpp"

#define ECHO 12
#define TRIG 13

#define TIMEOUT_US 25000UL // 0.025s

float read_distance(void) {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  unsigned long t = pulseIn(ECHO, HIGH, TIMEOUT_US);
  return t == 0 ? -1. : t / 58.;
}

void setup(void) {
  Serial.begin(9600);
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);

  led_write(ALL_OFF);
}

void loop(void) {
  float d = read_distance();

  Serial.print("Distance=");
  Serial.println(d, 2);
  if (d >= 0. && d < 40.) {
    byte v = led_no(8 - (d / 5.));
    Serial.print("v=");
    Serial.println(8 - (int)(d / 5.));
    led_write(v);
  } else if (d >= 40.) {
    led_write(ALL_OFF);
  }
  delay(200);
}
