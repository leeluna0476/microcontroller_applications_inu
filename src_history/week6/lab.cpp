#include <Arduino.h>

#define FIRST_SW 2
#define SW_NO 3
#define FIRST_LED 5
#define LED_NO 8

byte prev_sw = 0x00;

void led_write(byte value) { // active low
  value = ~value;
  for (int i = 0; i < LED_NO; ++i) {
    digitalWrite(FIRST_LED + i, (value >> i) & 1);
  }
}

void led_shift(int shift_no, int speed) {
  for (int c = 0; c < shift_no; ++c) {
    for (int i = 0; i < LED_NO - 1; ++i) {
      led_write(0x01 << i);
      delay(speed);
    }
    for (int i = LED_NO - 1; i > 0; --i) {
      led_write(0x01 << i);
      delay(speed);
    }
  }
  led_write(0x01);
  delay(speed);
  led_write(0x00);
}

void led_alternate(int alt_no, int speed) {
  for (int c = 0; c < alt_no; ++c) {
    led_write(0xAA);
    delay(speed);
    led_write(~0xAA);
    delay(speed);
  }
  led_write(0x00);
}

void led_cross(int cross_no, int speed) {
  for (int c = 0; c < cross_no; ++c) {
    for (int i = 0; i < (LED_NO >> 1) - 1; ++i) {
      led_write((0x80 >> i) | (0x01 << i));
      delay(speed);
    }
    for (int i = LED_NO >> 1; i < LED_NO - 1; ++i) {
      led_write((0x80 >> i) | (0x01 << i));
      delay(speed);
    }
  }
  led_write(0x81);
  delay(speed);
  led_write(0x00);
}

byte sw_read() { // active low
  byte value = 0x00;
  for (int i = 0; i < SW_NO; ++i) {
    value |= !digitalRead(FIRST_SW + i) << i;
  }
  return value;
}

byte sw_pressed() {
  byte curr = sw_read();
  Serial.print("curr=");
  Serial.println(curr);
  byte edge = curr & (~prev_sw);
  prev_sw = curr;
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
  Serial.begin(9600);
}

void loop() {
  byte pressed = sw_pressed();

  if (pressed & 0x01) {
    led_shift(2, 100);
  } else if (pressed & 0x02) {
    led_alternate(2, 300);
  } else if (pressed & 0x04) {
    led_cross(2, 200);
  }

  delay(10);
}
