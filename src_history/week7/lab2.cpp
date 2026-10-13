#include <Arduino.h>
#include <avr/interrupt.h>

#define FIRST_LED 2
#define LED_NO 8
#define SW1 10 // PCINT2
#define SW2 11 // PCINT3
#define SW3 12 // PCINT4

#define TASK_NO 4
#define TASK_DEFAULT 0
#define TASK_1 1
#define TASK_2 2
#define TASK_3 3

#define STEP_DEFAULT 2
#define STEP_TASK_1 2
#define STEP_TASK_2 2
#define STEP_TASK_3 14

#define INTVL 300
#define ADJUST_INTVL_UNIT 50

#define MAX_INTERVAL 500

byte task = TASK_DEFAULT;
byte step = 0;
unsigned long interval = INTVL;
unsigned long last_frame = 0;
long dinterval = 0;

volatile byte sw_event = 0;
volatile byte prev_pinb = 0xFF; // active low
volatile unsigned long last_press = 0;

void adjust_interval(long diff);
bool time_up(void);
void execute_task(void);
void draw_frame(void);
void led_write(byte val);
byte frame_default(void);
byte frame_task_1(void);
byte frame_task_2(void);
byte frame_task_3(void);
bool task_done(void);

ISR(PCINT0_vect) {
  byte now = PINB & 0x1C;
  byte edge = prev_pinb & ~now;
  prev_pinb = now;

  if (millis() - last_press < 200) {
    return;
  }
  last_press = millis();

  sw_event |= edge >> 2;
}

void setup(void) {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);

  PCICR |= 1 << PCIE0;
  PCMSK0 |= (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4);
}

void loop(void) {
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;
  prev_pinb = 0xFF;
  interrupts();

  if (event) {
    if (event & 0x01) {
      task = (task + 1) % TASK_NO;
      step = 0;
      execute_task();
      return;
    }
    if (event & 0x02) {
      adjust_interval(ADJUST_INTVL_UNIT);
    }
    if (event & 0x04) {
      adjust_interval(-ADJUST_INTVL_UNIT);
    }
  }

  if (time_up()) {
    if (task_done()) {
      step = 0;
    }

    execute_task();
  }
}

void adjust_interval(long diff) {
  long d = dinterval + diff;
  long i = interval + d;
  if (i > 500) {
    dinterval = -200;
  } else if (i < 100) {
    dinterval = 200;
  } else {
    dinterval = d;
  }
}

bool time_up(void) {
  return millis() - last_frame >= interval + dinterval;
}

void execute_task(void) {
  draw_frame();
  last_frame = millis();
  ++step;
}

void draw_frame(void) {
  switch (task) {
  case TASK_DEFAULT:
    led_write(frame_default());
    break;
  case TASK_1:
    led_write(frame_task_1());
    break;
  case TASK_2:
    led_write(frame_task_2());
    break;
  case TASK_3:
    led_write(frame_task_3());
    break;
  default:
    break;
  }
}

void led_write(byte val) {
  val = ~val;

  for (int i = 0; i < LED_NO; ++i) {
    digitalWrite(FIRST_LED + i, (val >> i) & 1);
  }
}

byte frame_default(void) {
  return step ? 0x55 : 0xAA;
}

byte frame_task_1(void) {
  return step ? 0x0F : 0xF0;
}

byte frame_task_2(void) {
  return step ? 0x3c : 0xc3;
}

byte frame_task_3(void) {
  byte n = step % 14;
  if (n >= 8) {
    n = 14 - n;
  }
  return 1 << n;
}

bool task_done(void) {
  switch (task) {
  case TASK_DEFAULT:
    return step == STEP_DEFAULT;
  case TASK_1:
    return step == STEP_TASK_1;
  case TASK_2:
    return step == STEP_TASK_2;
  case TASK_3:
    return step == STEP_TASK_3;
  default:
    return false;
  }
}
