#include <Arduino.h>
#include <avr/interrupt.h>

#define FIRST_LED 2
#define LED_NO 8
#define ALL_OFF 0x00
#define ALL_ON 0xFF
#define BUFSIZE 96

#define CDS_PIN A1
#define THRESHOLD 600

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

void tx_chr(char c) {
  Serial.write(c);
}

void tx_str(const char* s) {
  while (*s) {
    tx_chr(*s);
    ++s;
  }
}

void Printf(const char* fmt, ...) {
  char buf[BUFSIZE];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  tx_str(buf);
}

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
    Printf("count=%hhu\n", p);
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
  Printf("CDS=%d\n", v);
  control_lighting(v);
  delay(200);
}
