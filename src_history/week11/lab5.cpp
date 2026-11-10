#include <Arduino.h>
#include <avr/interrupt.h>
#include <Adafruit_NeoPixel.h>

#define SW_PIN 8
#define NEO_PIN A4
#define LED_NO 16

Adafruit_NeoPixel ring(LED_NO, NEO_PIN, NEO_GRBW + NEO_KHZ800);
const uint32_t YELLOW = ring.Color(128, 128, 0, 0);

int f_no = 0;
unsigned long last_frame = 0;

#define INTVL_SLOW 700
#define INTVL_INTERM 400
#define INTVL_FAST 100
unsigned long interval = INTVL_INTERM;

volatile unsigned long last_press = 0;
volatile byte sw_event = 0;
volatile byte prev_pinb = 0x01;

ISR(PCINT0_vect) {
  byte now = PINB & 0x01;
  byte edge = prev_pinb & ~now;
  prev_pinb = now;

  if (millis() - last_press < 200) {
    return;
  }
  last_press = millis();

  sw_event = edge;
}

void led_all_off(void) {
  ring.clear();
  ring.show();
}

uint32_t check_idx(const int* IDX, int len, int i, int shift) {
  for (int j = 0; j < len; ++j) {
    if (i == (IDX[j] + shift) % LED_NO) {
      return YELLOW;
    }
  }
  return 0;
}

void led_star_init(const int* IDX, int len, int f_no) {
  if (f_no >= len) {
    return;
  }

  ring.setPixelColor(IDX[f_no], YELLOW);
  ring.show();
}

void led_star_shift(const int* IDX, int len, int f_no) {
  for (int j = 0; j < LED_NO; ++j) {
    uint32_t color = check_idx(IDX, len, j, f_no);
    ring.setPixelColor(j, color);
  }
  ring.show();
}

void led_star(unsigned long interval) {
  const static int IDX[] = {0, 6, 13, 3, 10};
  static int f_no = 0;
  if (millis() - last_frame < interval) {
    return;
  }

  if (f_no == 0) {
    led_all_off();
  }

  if (f_no < 5) {
    led_star_init(IDX, 5, f_no);
  } else {
    led_star_shift(IDX, 5, f_no - 4);
  }
  f_no = (f_no + 1) % 21;
  last_frame = millis();
}

unsigned long change_interval(void) {
  static const unsigned long INTVL[] = {INTVL_INTERM, INTVL_FAST, INTVL_SLOW};
  static int cnt = 0;
  cnt = (cnt + 1) % 3;
  return INTVL[cnt];
}

void setup(void) {
  ring.begin();
  led_all_off();

  pinMode(SW_PIN, INPUT_PULLUP);
  PCICR |= 1 << PCIE0;
  PCMSK0 |= (1 << PCINT0);
}

void loop(void) {
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;
  prev_pinb = 0x01;

  interrupts();

  if (event & 0x01) {
    interval = change_interval();
  }
  led_star(interval);
}
