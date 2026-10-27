#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>

#ifndef FIRST_LED
#define FIRST_LED 2
#endif

#ifndef LED_NO
#define LED_NO 8
#endif

#ifndef ALL_ON
#define ALL_ON 0xFF
#endif

#ifndef ALL_OFF
#define ALL_OFF 0x00
#endif

void led_write(byte val);
void led_all_onoff(int no, int speed);

#endif
