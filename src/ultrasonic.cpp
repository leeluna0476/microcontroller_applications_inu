#include "ultrasonic.hpp"

float read_distance(byte trig, byte echo, unsigned long timeout) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long t = pulseIn(echo, HIGH, timeout);
  return t == 0 ? -1. : t / 58.;
}
