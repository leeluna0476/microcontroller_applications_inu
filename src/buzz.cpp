#include "buzz.hpp"

// CTC: Timer/Counter Top == OCR2A. turn off.
void setup_timer2_ctc(bool oc_on, unsigned int presc, unsigned int freq) {
  TCCR2A = 0x00; // turn off timer while setup
  TCCR2B = 0x00;

  TCNT2 = 0;                     // Timer/Counter Bottom
  OCR2A = calc_ocr(presc, freq); // TCNT2 is compared with this value
  TIFR2 = (1 << OCF2A);          // initialize COMPA flag

  TCCR2A = (1 << WGM21);
  set_oc(&TCCR2A, oc_on);
  TCCR2B = calc_tccrb(presc);
}

uint8_t calc_tccrb(unsigned int presc) {
  switch (presc) {
  case 1:
    return 0x01;
  case 8:
    return 0x02;
  case 32:
    return 0x03;
  case 64:
    return 0x04;
  case 128:
    return 0x05;
  case 256:
    return 0x06;
  case 1024:
    return 0x07;
  default:
    return 0x00;
  }
}

// calculate the output compare register value
uint8_t calc_ocr(unsigned int presc, unsigned int freq) {
  if (presc == 0 || freq == 0) {
    return 0x00;
  }
  uint32_t ocr = F_CPU / (2UL * presc * freq) - 1;
  return ocr > 0xFF ? 0x00 : ocr;
}

// turns on/off the output compare mode
void set_oc(volatile uint8_t* tccra, bool on) {
  if (on) {
    *tccra |= (1 << COM2A0);
  } else {
    *tccra &= ~(1 << COM2A0);
  }
}
