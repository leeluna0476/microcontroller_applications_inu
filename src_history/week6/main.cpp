#include <Arduino.h>

#define FIRST_SW 2
#define SW_NO 3
#define FIRST_LED 5
#define LED_NO 8

byte sw_prev = 0x00;

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
  led_write(0x01);
}

byte sw_read() {
  byte val = 0;
  for (int i = 0; i < SW_NO; ++i) {
    val |= !digitalRead(FIRST_SW + i) << i;
  }
  return val;
}

byte sw_pressed() {
  byte curr = sw_read();
  byte edge = curr & (~sw_prev);
  sw_prev = curr;
  return edge;
}

void setup() {
  for (int i = 0; i < SW_NO; ++i) {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }

  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  led_write(0x00);
}

void loop() {
  static byte count = 0x00;
  static byte prev_edge = 0x00;
  static byte prev_count = 0x00;
  byte pressed = sw_pressed();

  if (pressed & 0x01) {
    if (prev_edge & 0x02) { // 실습3
      count = (count + 2) % LED_NO;
    }
    led_write(1 << count);
    prev_count = count;
    // count = (count + 1) % (LED_NO + 1); // 실습1
    count = (count + 1) % LED_NO; // 실습2
    prev_edge = 0x01;
  } else if (pressed & 0x02) {
    if (prev_edge & 0x01) { // 실습3
      count = (count + LED_NO - 2) % LED_NO;
    }
    led_write(1 << count);
    prev_count = count;
    count = (count + LED_NO - 1) % LED_NO;
    prev_edge = 0x02;
  } else if (pressed & 0x04) {
    led_shift(2, 100);
    led_write(1 << prev_count); // restore the previous state
  }
  delay(10);
}
