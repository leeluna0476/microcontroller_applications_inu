#include <Arduino.h>
#include "buzz.hpp"

#define BUZZ 11
#define PRESCALER 256

uint16_t notes[] = {262, 392, 440, 392, 349, 330, 294, 262};

void setup(void) {
  pinMode(BUZZ, OUTPUT);
  Serial.begin(9600);
  delay(200);
}

// c c g g a a g f f e e d d c
void loop(void) {
  static uint8_t n = 0;
  static uint8_t cnt = 0;
  if (!cnt || (n != 3 && n != 7)) { // change notes
    setup_timer2a_ctc(true, PRESCALER, notes[n]);
  }

  if (cnt || (n != 3 && n != 7)) { // rest on a beat
    delay(900);
    set_oc(&TCCR2A, false);
    digitalWrite(BUZZ, LOW);
    delay(100);
  } else {
    delay(1000);
  }

  cnt++;
  if (cnt == 2) {
    n = (n + 1) % 8;
    cnt = 0;
  }
}
