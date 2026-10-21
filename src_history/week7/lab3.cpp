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

#define STX '<'
#define CMD1 '1'
#define CMD2 '2'
#define CMD3 '4'
#define ETX '>'

#define NONE 0x00
#define READY 0x01
#define ERROR 0x02
byte state = 0;
byte cmd = 0x00;
byte data = 0x00;
byte flag = NONE;

bool is_cmd(byte c) {
  return c == CMD1 || c == CMD2 || c == CMD3;
}

byte process_cmd(byte c) {
  byte next_state;
  if (is_cmd(c)) {
    next_state = state + 1;
    cmd = c;
  } else {
    flag = ERROR;
    next_state = 0;
  }
  return next_state;
}

byte process_data(byte c) {
  byte next_state = 0;
  switch (cmd) {
  case CMD1:
    data = c - '0';
    next_state = state + 2;
    break;
  case CMD2:
    data <<= 4;
    if (isdigit(c)) {
      data |= c - '0';
      next_state = state + 1;
    } else if (c >= 'A' && c <= 'F') {
      data |= c - 55;
      next_state = state + 1;
    } else {
      flag = ERROR;
      next_state = 0;
    }
    break;
  case CMD3:
    data = c - '0';
    next_state = state + 2;
    break;
  default:
    break;
  }
  return next_state;
}

void rx_decode(void) {
  while (Serial.available() > 0) {
    byte c = Serial.read();
    if (state == 0 && c == STX) {
      ++state;
    } else if (state == 1) {
      state = process_cmd(c);
    } else if (state >= 2 && state <= 3) {
      state = process_data(c);
    } else if (state == 4) {
      if (c == ETX) {
        flag = READY;
      } else {
        flag = ERROR;
      }
      state = 0;
    }
  }
}

void rx_clear(void) {
  flag = NONE;
  cmd = 0x00;
  data = 0x00;
}

void echo_data(byte c) {
  if (c <= 9) {
    tx_chr(c + '0');
  } else {
    tx_chr(c + 55);
  }
}

void command(void) {
  tx_chr(STX);
  tx_chr(cmd);
  if (cmd == CMD2) {
    echo_data(data >> 4);
    echo_data(data & 0x0F);
  } else {
    echo_data(data);
  }
  tx_chr(ETX);
  tx_chr('\n');
  switch (cmd) {
  case CMD1:
    led_all_onoff(data, 500);
    break;
  case CMD2:
    led_alternate(3, 500, data);
    break;
  case CMD3:
    led_shift(data, 100);
    break;
  default:
    break;
  }
}

void setup(void) {
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  led_write(0x00);
  Serial.begin(9600);
  delay(200);
}

void loop(void) {
  // rx_collect();
  rx_decode();

  // if (line_ready) {
  //   command(rx_buf, buf_len);
  //   rx_clear();
  //   led_write(ALL_OFF);
  // }
  if (flag == READY) {
    command();
    rx_clear();
    led_write(ALL_OFF);
  } else if (flag == ERROR) {
    tx_str("ERROR\n", 6);
    rx_clear();
  }
}
