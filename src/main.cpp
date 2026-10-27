#include <Arduino.h>
#include "serial.hpp"
#include "led.hpp"

#define CDS_PIN A1
#define THRESHOLD 600

byte extract_pattern(void) {
  static byte count = 245; // 0으로 돌아가는 결과를 빨리 보기 위해...
  ++count;                 // byte(uint8_t) => [0, 255] circulate
  return count;
}

void control_lighting(int cds) {
  static bool prev = false;
  bool covered = cds < THRESHOLD;
  if (!prev && covered) {
    byte p = extract_pattern();
    print("count=%hhu\n", p);
    led_write(p);
  }
  prev = covered;
}

void setup(void) {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  Serial.begin(9600);
  delay(200);
  led_write(ALL_OFF);
}

void loop(void) {
  int v = analogRead(CDS_PIN);
  control_lighting(v);
  delay(200);
}
