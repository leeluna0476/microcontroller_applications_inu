#include <Arduino.h>

#define BUZZ 11
#define COMPA (byte)17

#define ECHO 12
#define TRIG 13

#define TIMEOUT_US 25000UL // 25ms

unsigned long calc_speed(float d);
void set_oc(volatile uint8_t* tccr, bool on);
void control_buzzer(float d);
float read_distance(void);

void setup(void) {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(BUZZ, OUTPUT);
  TCCR2A = (1 << WGM21);                            // CTC: Timer/Counter Top == OCR2A. turn off.
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // frequency prescaler 64
  TCNT2 = 0;                                        // Timer/Counter Bottom
  OCR2A = COMPA;                                    // TCNT2 is compared with this value
  TIFR2 = (1 << OCF2A);                             // initialize COMPA flag
}

void loop(void) {
  float d = read_distance();
  control_buzzer(d);
  delay(100);
}

// range: [100, 1000]
unsigned long calc_speed(float d) {
  if (d < 10. || d > 30.) {
    return 0;
  }
  return 100 + ((unsigned long)d - 10) * 45;
}

void control_buzzer(float d) {
  static uint8_t prev_state = 0;
  static bool on = false;
  static unsigned long speed = 0;
  static unsigned long last_ms = 0;

  if (d < 0.) {
    return;
  } else if (d < 10.) {
    if (prev_state) {
      on = true;
      set_oc(&TCCR2A, on);
      prev_state = 0;
    }
  } else {
    if (d <= 30.) {
      speed = calc_speed(d);
      if (millis() - last_ms >= speed) {
        on = !on;
        set_oc(&TCCR2A, on);
        last_ms = millis();
        prev_state = 1;
      }
    } else {
      if (prev_state != 2) {
        on = false;
        set_oc(&TCCR2A, on);
        prev_state = 2;
      }
    }
  }
}

void set_oc(volatile uint8_t* tccr, bool on) {
  if (on) {
    *tccr |= (1 << COM2A0);
  } else {
    *tccr &= ~(1 << COM2A0);
  }
}

float read_distance(void) {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  unsigned long t = pulseIn(ECHO, HIGH, TIMEOUT_US);
  return t == 0 ? -1. : t / 58.;
}
