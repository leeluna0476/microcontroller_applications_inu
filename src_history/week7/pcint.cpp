#include <Arduino.h>
#include <avr/interrupt.h>

#define FIRST_SW 2
#define SW_NO 3
#define FIRST_LED 5
#define LED_NO 8

#define ALL_ON 0xFF
#define ALL_OFF 0x00

// switch pin no.
const int SW1_PIN = FIRST_SW;
const int SW2_PIN = FIRST_SW + 1;
const int SW3_PIN = FIRST_SW + 2;

// task no.
const int TASK_DEFAULT = 0;
const int TASK_LED_SHIFT = 1;
const int TASK_LED_ALTERNATE = 2;
const int TASK_LED_CROSS = 3;

// step no.
const int STEP_DEFAULT = 2;
const int STEP_LED_SHIFT = 14 * 2 + 1;
const int STEP_LED_ALTERNATE = 2 * 2;
const int STEP_LED_CROSS = 6 * 2 + 1;

// intervals
const int INTVL_DEFAULT = 300;
const int INTVL_LED_SHIFT = 200;
const int INTVL_LED_ALTERNATE = 2000;
const int INTVL_LED_CROSS = 300;

// global variables
byte prev_pind = 0xFF; // active low
byte task = TASK_DEFAULT;
byte step = 0;
unsigned long last_frame = 0;
unsigned long interval = INTVL_DEFAULT;
unsigned long left = 0;

// interrupt variables
volatile byte sw_event = 0;
volatile unsigned long last_press = 0;

ISR(PCINT2_vect) { // just like a signal handler in linux
  // active low => falling edge => pressed
  byte now = PIND & 0x1C; // 0001 1100
  byte edge = prev_pind & ~now;
  prev_pind = now;

  if (millis() - last_press < 200) {
    return;
  }
  last_press = millis();

  sw_event |= edge >> 2;
}

void led_write(byte value) { // active low
  value = ~value;
  for (int i = 0; i < LED_NO; ++i) {
    digitalWrite(FIRST_LED + i, (value >> i) & 1);
  }
}

// 14 steps per round trip
// 0 <= step <= 13
byte frame_led_shift(byte step) {
  int s = step % 14;
  if (s >= 8) {
    s = 14 - s;
  }
  return 1 << s;
}

// 2 steps per round
// 0 <= step <= 1
byte frame_led_alternate(byte step) {
  return step & 1 ? 0x55 : 0xAA;
}

// 12 steps per round
// 0 <= step <= 5
byte frame_led_cross(byte step) {
  byte mod = STEP_LED_CROSS >> 1;
  byte s = step % mod;
  if (s < (LED_NO >> 1) - 1) {
    return (0x80 >> s) | (0x01 << s);
  } else {
    return (0x02 << s) | (0x40 >> s);
  }
}

// 2 steps per round
// 0 <= step <= 1
byte frame_default(byte step) {
  return step & 1 ? ALL_OFF : ALL_ON;
}

void draw_frame() {
  static byte (*frame[])(byte) = {frame_default, frame_led_shift, frame_led_alternate,
                                  frame_led_cross};
  led_write(frame[task](step));
}

// the stack storing each state when an interrupt occurs
#define STACK_MAX 4
struct state {
  byte task;
  byte step;
  unsigned long interval;
  unsigned long left;
} stack[STACK_MAX];
int top = -1;

void push_prev_state(void) {
  if (top + 1 == STACK_MAX) {
    return;
  }
  unsigned long past = millis() - last_frame;
  unsigned long left = interval < past ? 0 : interval - past;
  stack[++top] = (struct state){task, step, interval, left};
}

struct state pop_prev_state(void) {
  struct state st = {TASK_DEFAULT, STEP_DEFAULT, INTVL_DEFAULT, INTVL_DEFAULT};
  if (top > -1) {
    st = stack[top--];
  }
  return st;
}

bool task_done() {
  if (task == TASK_LED_SHIFT) {
    return step == STEP_LED_SHIFT;
  } else if (task == TASK_LED_ALTERNATE) {
    return step == STEP_LED_ALTERNATE;
  } else if (task == TASK_LED_CROSS) {
    return step == STEP_LED_CROSS;
  } else {
    return false;
  }
}

bool time_up(void) {
  return millis() - last_frame >= interval;
}

void execute_step(void) {
  draw_frame();
  last_frame = millis();
  ++step;
}

void restore_state(void) {
  struct state st = pop_prev_state(); // something is pushed whenever an event occurs
  task = st.task;
  step = st.step - 1;
  interval = st.interval;
  left = st.left;
  execute_step(); // restore previous state
  last_frame = millis() - left;
}

//      D2,     D3,     D4,    D5,    D6,    D7,    D8,    D9,   D10,   D11,   D12 (Arduino Uno R3)
//     PD2,    PD3,    PD4,   PD5,   PD6,   PD7,   PB0,   PB1,   PB2,   PB3,   PB4 (Atmega 328p)
// PCINT18,PCINT19,PCINT20,PCINT5,PCINT6,PCINT7,PCINT0,PCINT1,PCINT2,PCINT3,PCINT4 (PCINT)
// sw pin to interrupt pin => PCMSK2(PCINT16~PCINT23) => 0001 1100
void setup(void) {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  for (int i = 0; i < SW_NO; ++i) {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }
  PCICR |= 1 << PCIE2; // give interrupt when level changes in any pins from PCIE2(PCINT16~23)
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // 0x1C. the active pins specified

  led_write(ALL_OFF);
}

void loop(void) {
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;
  prev_pind = 0xFF;
  interrupts();

  if (event != 0) {    // event checked
    push_prev_state(); // push the previous state -- even if the state wasn't ongoing?
    if (event & 0x01) {
      task = TASK_LED_SHIFT;
      interval = INTVL_LED_SHIFT;
    }
    if (event & 0x02) {
      task = TASK_LED_ALTERNATE;
      interval = INTVL_LED_ALTERNATE;
    }
    if (event & 0x04) {
      task = TASK_LED_CROSS;
      interval = INTVL_LED_CROSS;
    }
    step = 0;       // new event, new task, init the step
    execute_step(); // don't wait
    return;
  }

  if (time_up()) {
    if (task == TASK_DEFAULT && step == STEP_DEFAULT) {
      step = 0;
    }

    execute_step();

    if (task_done()) {
      restore_state();
    }
  }
}
