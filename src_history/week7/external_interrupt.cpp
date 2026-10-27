#include <Arduino.h>

#define FIRST_SW 2
#define SW_NO 3
#define FIRST_LED 5
#define LED_NO 8

#define ALL_ON 0xFF
#define ALL_OFF 0x00

const int SW1_PIN = FIRST_SW;
const int SW2_PIN = FIRST_SW + 1;

byte prev_sw = 0x00;

// volatile: can be read or modified asynchronously
//           allows preserving values across longjmp (non-local jmp)
volatile byte sw_event = 0;
volatile unsigned long last_press = 0;

void isr_sw1() {
  if (millis() - last_press < 200) {
    return;
  } // 채터링 무시
  last_press = millis();
  sw_event = sw_event | 0x01;
}

void isr_sw2() {
  if (millis() - last_press < 200) {
    return;
  }
  last_press = millis();
  sw_event = sw_event | 0x02;
}

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
}

void led_alternate(int value, int alt_no, int speed) {
  for (int c = 0; c < alt_no; ++c) {
    led_write(value);
    delay(speed);
    led_write(~value);
    delay(speed);
  }
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
}

void led_all_onoff(int no, int speed) {
  for (int c = 0; c < no; ++c) {
    led_write(ALL_ON);
    delay(speed);
    led_write(ALL_OFF);
    delay(speed);
  }
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
  for (int i = 0; i < LED_NO; i++) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SW1_PIN), isr_sw1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SW2_PIN), isr_sw2, FALLING);
  led_write(ALL_OFF);
}

void loop() {
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;

  interrupts();

  if (event & 0x01) { // SW1
    led_shift(2, 80);
  	led_write(ALL_OFF);
  }
  if (event & 0x02) { // SW2
    led_alternate(0xAA, 2, 300);
  	led_write(ALL_OFF);
  }

  led_all_onoff(1, 500);
}
