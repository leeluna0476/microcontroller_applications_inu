#include <Arduino.h>

#define FIRST_LED 5
#define LED_NO 8

void led_write(byte value) {
  value = ~value;

  for (int i = 0; i < LED_NO; ++i) {
    digitalWrite(FIRST_LED + i, (value >> i) & 1);
  }
}

void led_shift(int shift_no, int speed) {
  for (int c = 0; c < shift_no; ++c) {
    for (int i = 0; i < LED_NO - 1; ++i) { // 0~6
      led_write(0x01 << i);
      delay(speed);
    }
    for (int i = 0; i < LED_NO - 1; ++i) { // 7~1
      led_write((byte)0x80 >> i);
      delay(speed);
    }
  }
  // led_write(0x01); // 유한 번 동작시킬 때 불빛이 D3에서 멈추는 게 맘에 안 들었다
}

void setup() {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  led_write(0x00);
}

void loop() {
  led_shift(1, 100);
}
