#ifndef BUZZ_HPP
#define BUZZ_HPP

#include <util/delay.h>
#include <avr/io.h>
#include <stdint.h>

void setup_timer2a_ctc(bool oc_on, unsigned int presc, unsigned int freq);
uint8_t calc_tccrb(unsigned int presc);
uint8_t calc_ocr(unsigned int presc, unsigned int freq);
void set_oc(volatile uint8_t* tccra, bool on);

#endif
