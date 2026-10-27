#include "led.hpp"

void led_write(byte val) {
  val = ~val;
  for (int i = 0; i < LED_NO; ++i) {
    digitalWrite(FIRST_LED + i, (val >> i) & 1);
  }
}

void led_all_onoff(int no, int speed) {
  for (int c = 0; c < no; ++c) {
    led_write(ALL_ON);
    delay(speed);
    led_write(ALL_OFF);
    delay(speed);
  }
}
