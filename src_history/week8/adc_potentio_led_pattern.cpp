#include <Arduino.h>
#include <avr/interrupt.h>
#include <string.h>

#define FIRST_LED 2
#define LED_NO 8
#define ALL_OFF 0x00
#define ALL_ON 0xFF
#define BUFSIZE 96

char rx_buf[BUFSIZE];
byte buf_len;
bool line_ready = false;

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

void led_alternate(int alt_no, int speed, byte pattern) {
  for (int c = 0; c < alt_no; ++c) {
    led_write(pattern);
    delay(speed);
    led_write(~pattern);
    delay(speed);
  }
}

void tx_chr(char c) {
  Serial.write(c);
}

void tx_str(const char* s, byte len) {
  for (int i = 0; i < len; ++i) {
    tx_chr(s[i]);
  }
}

void Printf(const char* fmt, ...) {
  char buf[96];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  tx_str(buf, strlen(buf)); // 어차피 string이라면 그냥 while (*str) 이렇게 써도 되는 거 아닌교
}

void setup(void) {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  Serial.begin(9600);
  delay(200);
  led_write(ALL_OFF);
}

byte extract_pattern(int v) {
  byte p = 0;
  for (int i = 0; i < v / 128 + 1; ++i) {
    p |= 1 << i;
  }
  return p;
}

#define PO_PIN A1
void loop(void) {
  int v = analogRead(PO_PIN);
  Printf("ADC: %d ", v);
  Printf("Level: %d ", v / 128 + 1);
  byte pattern = extract_pattern(v);
  Printf("Pattern: 0x%02X\n", pattern);
  led_write(pattern);
  delay(500);
}
