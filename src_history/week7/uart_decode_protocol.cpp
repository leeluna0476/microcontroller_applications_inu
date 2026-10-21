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

void led_alternate(int alt_no, int speed) {
  for (int c = 0; c < alt_no; ++c) {
    led_write(0xAA);
    delay(speed);
    led_write(0x55);
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

// received data => stored in a two-level FIFO
// 버퍼 크기에 제한이 있기 때문에, 상시 시리얼 통신이 일어날 수 있다면
// 상시 데이터를 수신하여 메모리에 따로 저장해둬야 한다.
// 그러나 계속 기다리고 있을 수는 없으므로 인터럽트 발생 시에만 처리한다.
// 인터럽트 자체는 라이브러리에서 처리함.
void rx_collect(void) {
  // 현재 읽을 데이터가 있으면 바로바로 비워준다.
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (buf_len == BUFSIZE - 1 || c == '\n') {
      rx_buf[buf_len] = '\0';
      line_ready = true;
    } else if (c != '\r') {
      rx_buf[buf_len++] = c;
    }
  }
}

#define STX '<'
#define CMD1 '1'
#define CMD2 '2'
#define CMD3 '3'
#define ETX '>'

#define NONE 0x00
#define READY 0x01
#define ERROR 0x02
byte state = 0;
byte cmd = 0x00;
byte flag = NONE;
void rx_decode(void) {
  while (Serial.available() > 0) {
    byte c = Serial.read();
    if (state == 0 && c == STX) {
      ++state;
    } else if (state == 1) {
      ++state;
      if (c >= CMD1 && c <= CMD3) {
        cmd = c;
      } else {
        state = 0;
        flag = ERROR;
      }
    } else if (state == 2) {
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
}

// void rx_clear(void) {
//   line_ready = 0;
//   buf_len = 0;
// }

// protocol
// void command(const char* cmd, byte len) {
//   tx_str(cmd, len);
//   tx_chr('\n');
//   if (!strcmp(cmd, "LED_ON_OFF")) {
//     tx_str("recv\n", 5);
//     led_all_onoff(2, 500);
//   } else if (!strcmp(cmd, "LED_ALT")) {
//     led_alternate(2, 500);
//   } else if (!strcmp(cmd, "LED_SHIFT")) {
//     led_shift(1, 500);
//   }
// }

void command(void) {
  tx_chr(STX);
  tx_chr(cmd);
  tx_chr(ETX);
  tx_chr('\n');
  switch (cmd) {
  case CMD1:
    led_all_onoff(2, 500);
    break;
  case CMD2:
    led_alternate(2, 500);
    break;
  case CMD3:
    led_shift(2, 200);
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
