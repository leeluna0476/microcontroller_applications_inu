#include <Arduino.h>
#include <avr/interrupt.h>
#include "serial.hpp"
#include "led.hpp"

#define SW_NO 4
#define SW_1 10
#define SW_2 11
#define SW_3 12
#define SW_4 13
#define POT_PIN A0
#define CDS_PIN A1

#define PACKET_STX 0x02
#define PACKET_ETX 0x03
#define MAX_DATA_LEN 16

#define CMD_PING 0x01
#define CMD_GET_STATUS 0x02
#define CMD_LED_SET 0x10
#define CMD_LED_RUN 0x11
#define CMD_LED_STOP 0x12
#define CMD_SET_PERIOD 0x13
#define CMD_SET_SPEED_SRC 0x14
#define CMD_ADC_READ 0x20
#define CMD_ADC_STREAM 0x21

#define CMD_ACK 0x80
#define CMD_EVT_SWITCH 0x90
#define CMD_EVT_LED 0x91
#define CMD_EVT_MODE 0x92
#define CMD_EVT_SPEED 0x93
#define CMD_ADC_DATA 0xA0
#define CMD_STATUS 0xB0

#define MODE_IDLE 0
#define MODE_ALL 1
#define MODE_ALT 2
#define MODE_SHIFT 3

#define CAUSE_PC 0
#define CAUSE_SWITCH 1
#define CAUSE_COMPLETE 2

#define SPEED_PC 0
#define SPEED_POT 1

#define RX_WAIT_STX 0
#define RX_READ_CMD 1
#define RX_READ_LEN 2
#define RX_READ_DATA 3
#define RX_READ_ETX 4

struct message {
  byte stx;
  byte cmd;
  byte len;
  byte data[MAX_DATA_LEN];
  byte etx;
};

message parse_message(const char* input, size_t input_len);
bool is_valid_command_length(byte cmd, byte len);
void initialize_controller(void);
void receive_serial_packets(void);
void reset_receiver(void);
void handle_message(const message& msg);
void send_packet(byte cmd, const byte* data, byte len);
void send_ack(byte handled_cmd);
void send_status(void);
void send_adc_data(byte channel);
void send_switch_event(byte switch_no);
void send_led_event(byte pattern);
void send_mode_event(byte mode, byte cause);
void request_speed_event(uint16_t period, uint16_t pot, byte source);
void send_pending_speed_event(void);
uint16_t read_u16_be(const byte* data);
uint16_t read_adc_channel(byte channel);
uint16_t period_from_pot(uint16_t pot);
void set_led_pattern(byte pattern);
byte pattern_for_step(byte mode, byte step);
void start_led_mode(byte mode, byte repeat, byte start_pattern, byte cause);
void stop_led_mode(byte cause);
void update_led_mode(void);
void process_switch_events(void);
void update_pot_speed(void);
void update_adc_streams(void);

volatile byte prev_pinb = 0xFF;
volatile byte sw_event = 0x00;
volatile unsigned long last_press = 0;

byte rx_state = RX_WAIT_STX;
byte rx_packet[MAX_DATA_LEN + 4];
byte rx_data_index = 0;
byte rx_data_len = 0;

bool controller_initialized = false;
byte current_mode = MODE_IDLE;
byte current_led_pattern = ALL_OFF;
uint16_t current_period = 50;
byte current_speed_source = SPEED_POT;

byte mode_repeat_target = 0;
byte mode_repeat_count = 0;
byte mode_step = 0;
byte mode_start_pattern = ALL_OFF;
unsigned long last_led_step_time = 0;

bool adc_stream_enabled[2] = {false, false};
uint16_t adc_stream_interval[2] = {0, 0};
unsigned long last_adc_stream_time[2] = {0, 0};

uint16_t current_pot_value = 0;
uint16_t last_reported_speed_period = 50;
unsigned long last_pot_sample_time = 0;
unsigned long last_speed_event_time = 0;
bool speed_event_pending = false;
uint16_t pending_speed_period = 50;
uint16_t pending_speed_pot = 0;
byte pending_speed_source = SPEED_PC;

ISR(PCINT0_vect) {
  byte now = PINB & 0x3C;
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
  for (int i = 0; i < SW_NO; ++i) {
    pinMode(SW_1 + i, INPUT_PULLUP);
  }
  PCICR |= 1 << PCIE0;
  PCMSK0 |= (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4) | (1 << PCINT5);
  Serial.begin(38400, SERIAL_8O2);
}

void loop(void) {
  initialize_controller();
  receive_serial_packets();
  process_switch_events();
  update_pot_speed();
  update_led_mode();
  update_adc_streams();
  send_pending_speed_event();
}

message parse_message(const char* input, size_t input_len) {
  message parsed = {};

  if (input == NULL || input_len < 4) {
    return parsed;
  }

  const byte* packet = reinterpret_cast<const byte*>(input);
  const byte data_len = packet[2];

  if (packet[0] != PACKET_STX || data_len > MAX_DATA_LEN) {
    return parsed;
  }
  if (input_len != static_cast<size_t>(data_len) + 4) {
    return parsed;
  }
  if (packet[data_len + 3] != PACKET_ETX) {
    return parsed;
  }
  if (!is_valid_command_length(packet[1], data_len)) {
    return parsed;
  }

  parsed.stx = packet[0];
  parsed.cmd = packet[1];
  parsed.len = data_len;
  for (byte i = 0; i < MAX_DATA_LEN; ++i) {
    parsed.data[i] = (i < data_len) ? packet[i + 3] : 0;
  }
  parsed.etx = packet[data_len + 3];

  return parsed;
}

bool is_valid_command_length(byte cmd, byte len) {
  switch (cmd) {
  case CMD_PING:
  case CMD_GET_STATUS:
  case CMD_LED_STOP:
    return len == 0;

  case CMD_LED_SET:
  case CMD_SET_SPEED_SRC:
  case CMD_ADC_READ:
    return len == 1;

  case CMD_LED_RUN:
    return len == 3;

  case CMD_SET_PERIOD:
    return len == 2;

  case CMD_ADC_STREAM:
    return len == 4;

  default:
    return false;
  }
}

void initialize_controller(void) {
  if (controller_initialized) {
    return;
  }

  led_write(ALL_OFF);
  current_pot_value = read_adc_channel(0);
  current_period = period_from_pot(current_pot_value);
  last_reported_speed_period = current_period;
  last_pot_sample_time = millis();
  last_speed_event_time = millis() - 100UL;
  controller_initialized = true;
  send_status();
}

void receive_serial_packets(void) {
  while (Serial.available() > 0) {
    const byte value = static_cast<byte>(Serial.read());

    switch (rx_state) {
    case RX_WAIT_STX:
      if (value == PACKET_STX) {
        rx_packet[0] = value;
        rx_state = RX_READ_CMD;
      }
      break;

    case RX_READ_CMD:
      rx_packet[1] = value;
      rx_state = RX_READ_LEN;
      break;

    case RX_READ_LEN:
      rx_packet[2] = value;
      rx_data_len = value;
      rx_data_index = 0;
      if (rx_data_len > MAX_DATA_LEN) {
        reset_receiver();
      } else if (rx_data_len == 0) {
        rx_state = RX_READ_ETX;
      } else {
        rx_state = RX_READ_DATA;
      }
      break;

    case RX_READ_DATA:
      rx_packet[3 + rx_data_index] = value;
      ++rx_data_index;
      if (rx_data_index == rx_data_len) {
        rx_state = RX_READ_ETX;
      }
      break;

    case RX_READ_ETX:
      rx_packet[3 + rx_data_len] = value;
      if (value == PACKET_ETX) {
        const message msg = parse_message(
            reinterpret_cast<const char*>(rx_packet),
            static_cast<size_t>(rx_data_len) + 4);
        if (msg.stx == PACKET_STX) {
          handle_message(msg);
        }
      }
      reset_receiver();
      break;

    default:
      reset_receiver();
      break;
    }
  }
}

void reset_receiver(void) {
  rx_state = RX_WAIT_STX;
  rx_data_index = 0;
  rx_data_len = 0;
}

void handle_message(const message& msg) {
  switch (msg.cmd) {
  case CMD_PING:
    send_ack(msg.cmd);
    break;

  case CMD_GET_STATUS:
    send_status();
    break;

  case CMD_LED_SET: {
    send_ack(msg.cmd);
    if (current_mode != MODE_IDLE) {
      current_mode = MODE_IDLE;
      send_mode_event(MODE_IDLE, CAUSE_PC);
    }
    set_led_pattern(msg.data[0]);
    break;
  }

  case CMD_LED_RUN:
    if (msg.data[0] > MODE_SHIFT) {
      return;
    }
    if (msg.data[0] == MODE_IDLE) {
      if (msg.data[2] != 0x00) {
        return;
      }
      send_ack(msg.cmd);
      stop_led_mode(CAUSE_PC);
      break;
    }
    if (msg.data[0] == MODE_ALL && msg.data[2] != 0x00 &&
        msg.data[2] != 0xFF) {
      return;
    }
    if (msg.data[0] == MODE_SHIFT && msg.data[2] != 0x00) {
      return;
    }
    send_ack(msg.cmd);
    start_led_mode(msg.data[0], msg.data[1], msg.data[2], CAUSE_PC);
    break;

  case CMD_LED_STOP:
    send_ack(msg.cmd);
    stop_led_mode(CAUSE_PC);
    break;

  case CMD_SET_PERIOD: {
    const uint16_t new_period = read_u16_be(msg.data);
    if (new_period < 50 || new_period > 1000) {
      return;
    }
    const uint16_t difference =
        (new_period > last_reported_speed_period)
            ? new_period - last_reported_speed_period
            : last_reported_speed_period - new_period;
    current_period = new_period;
    current_speed_source = SPEED_PC;
    send_ack(msg.cmd);
    if (difference >= 20) {
      current_pot_value = read_adc_channel(0);
      request_speed_event(current_period, current_pot_value, SPEED_PC);
    } else {
      speed_event_pending = false;
    }
    break;
  }

  case CMD_SET_SPEED_SRC:
    if (msg.data[0] > SPEED_POT) {
      return;
    }
    current_speed_source = msg.data[0];
    send_ack(msg.cmd);
    if (current_speed_source == SPEED_PC) {
      if (speed_event_pending && pending_speed_source == SPEED_POT) {
        speed_event_pending = false;
      }
    } else {
      const uint16_t new_pot = read_adc_channel(0);
      const uint16_t new_period = period_from_pot(new_pot);
      const uint16_t difference = (new_period > current_period)
                                      ? new_period - current_period
                                      : current_period - new_period;
      current_pot_value = new_pot;
      current_period = new_period;
      if (difference >= 20) {
        request_speed_event(current_period, current_pot_value, SPEED_POT);
      }
    }
    break;

  case CMD_ADC_READ:
    if (msg.data[0] > 1) {
      return;
    }
    send_adc_data(msg.data[0]);
    break;

  case CMD_ADC_STREAM: {
    const byte channel = msg.data[0];
    const byte enabled = msg.data[1];
    if (channel > 1 || enabled > 1) {
      return;
    }

    const uint16_t interval = read_u16_be(&msg.data[2]);
    if ((enabled == 0 && interval != 0) ||
        (enabled == 1 && (interval < 50 || interval > 1000))) {
      return;
    }

    adc_stream_interval[channel] = interval;
    adc_stream_enabled[channel] = enabled == 1;
    if (adc_stream_enabled[channel]) {
      last_adc_stream_time[channel] = millis();
    }
    send_ack(msg.cmd);
    break;
  }

  default:
    break;
  }
}

void send_packet(byte cmd, const byte* data, byte len) {
  Serial.write(static_cast<byte>(PACKET_STX));
  Serial.write(cmd);
  Serial.write(len);
  if (data != NULL && len > 0) {
    Serial.write(data, len);
  }
  Serial.write(static_cast<byte>(PACKET_ETX));
}

void send_ack(byte handled_cmd) {
  send_packet(CMD_ACK, &handled_cmd, 1);
}

void send_status(void) {
  byte stream_bits = 0;
  if (adc_stream_enabled[0]) {
    stream_bits |= 0x01;
  }
  if (adc_stream_enabled[1]) {
    stream_bits |= 0x02;
  }

  const byte data[6] = {
      current_mode,
      current_led_pattern,
      static_cast<byte>(current_period >> 8),
      static_cast<byte>(current_period & 0xFF),
      current_speed_source,
      stream_bits,
  };
  send_packet(CMD_STATUS, data, sizeof(data));
}

void send_adc_data(byte channel) {
  const uint16_t value = read_adc_channel(channel);
  const byte data[3] = {
      channel,
      static_cast<byte>(value >> 8),
      static_cast<byte>(value & 0xFF),
  };
  send_packet(CMD_ADC_DATA, data, sizeof(data));
}

void send_switch_event(byte switch_no) {
  send_packet(CMD_EVT_SWITCH, &switch_no, 1);
}

void send_led_event(byte pattern) {
  send_packet(CMD_EVT_LED, &pattern, 1);
}

void send_mode_event(byte mode, byte cause) {
  const byte data[2] = {mode, cause};
  send_packet(CMD_EVT_MODE, data, sizeof(data));
}

void request_speed_event(uint16_t period, uint16_t pot, byte source) {
  pending_speed_period = period;
  pending_speed_pot = pot;
  pending_speed_source = source;
  speed_event_pending = true;
}

void send_pending_speed_event(void) {
  if (!speed_event_pending || millis() - last_speed_event_time < 100UL) {
    return;
  }

  const byte data[4] = {
      static_cast<byte>(pending_speed_period >> 8),
      static_cast<byte>(pending_speed_period & 0xFF),
      static_cast<byte>(pending_speed_pot >> 8),
      static_cast<byte>(pending_speed_pot & 0xFF),
  };
  send_packet(CMD_EVT_SPEED, data, sizeof(data));
  last_speed_event_time = millis();
  last_reported_speed_period = pending_speed_period;
  speed_event_pending = false;
}

uint16_t read_u16_be(const byte* data) {
  return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

uint16_t read_adc_channel(byte channel) {
  return static_cast<uint16_t>(analogRead(channel == 0 ? POT_PIN : CDS_PIN));
}

uint16_t period_from_pot(uint16_t pot) {
  return 50 + static_cast<uint16_t>((static_cast<unsigned long>(pot) * 950UL) /
                                    1023UL);
}

void set_led_pattern(byte pattern) {
  if (pattern == current_led_pattern) {
    return;
  }
  current_led_pattern = pattern;
  led_write(pattern);
  send_led_event(pattern);
}

byte pattern_for_step(byte mode, byte step) {
  if (mode == MODE_ALL) {
    return (step == 0) ? mode_start_pattern
                       : static_cast<byte>(~mode_start_pattern);
  }
  if (mode == MODE_ALT) {
    return (step == 0) ? mode_start_pattern
                       : static_cast<byte>(~mode_start_pattern);
  }
  if (step <= 7) {
    return static_cast<byte>(0x01 << step);
  }
  return static_cast<byte>(0x01 << (14 - step));
}

void start_led_mode(byte mode, byte repeat, byte start_pattern, byte cause) {
  current_mode = mode;
  mode_repeat_target = repeat;
  mode_repeat_count = 0;
  mode_step = 0;
  if (mode == MODE_ALL) {
    mode_start_pattern = (start_pattern == 0) ? 0x00 : 0xFF;
  } else if (mode == MODE_ALT) {
    mode_start_pattern = (start_pattern == 0) ? 0xAA : start_pattern;
  } else {
    mode_start_pattern = 0x01;
  }
  last_led_step_time = millis();
  send_mode_event(current_mode, cause);
  set_led_pattern(pattern_for_step(current_mode, mode_step));
}

void stop_led_mode(byte cause) {
  const bool was_running = current_mode != MODE_IDLE;
  current_mode = MODE_IDLE;
  mode_repeat_target = 0;
  mode_repeat_count = 0;
  mode_step = 0;
  set_led_pattern(ALL_OFF);
  if (was_running || cause == CAUSE_SWITCH) {
    send_mode_event(MODE_IDLE, cause);
  }
}

void update_led_mode(void) {
  if (current_mode == MODE_IDLE ||
      millis() - last_led_step_time < current_period) {
    return;
  }

  last_led_step_time = millis();
  const byte step_count = (current_mode == MODE_SHIFT) ? 14 : 2;
  ++mode_step;

  if (mode_step < step_count) {
    set_led_pattern(pattern_for_step(current_mode, mode_step));
    return;
  }

  mode_step = 0;
  ++mode_repeat_count;
  if (mode_repeat_target != 0 && mode_repeat_count >= mode_repeat_target) {
    current_mode = MODE_IDLE;
    set_led_pattern(ALL_OFF);
    send_mode_event(MODE_IDLE, CAUSE_COMPLETE);
    return;
  }

  set_led_pattern(pattern_for_step(current_mode, mode_step));
}

void process_switch_events(void) {
  noInterrupts();
  const byte events = sw_event;
  sw_event = 0;
  interrupts();

  for (byte i = 0; i < SW_NO; ++i) {
    if ((events & (1 << i)) == 0) {
      continue;
    }

    const byte switch_no = i + 1;
    send_switch_event(switch_no);
    if (switch_no <= 3) {
      const byte start_pattern =
          (switch_no == MODE_ALT) ? 0xAA :
          (switch_no == MODE_SHIFT) ? 0x01 : 0x00;
      start_led_mode(switch_no, 3, start_pattern, CAUSE_SWITCH);
    } else {
      stop_led_mode(CAUSE_SWITCH);
    }
  }
}

void update_pot_speed(void) {
  if (current_speed_source != SPEED_POT ||
      millis() - last_pot_sample_time < 10UL) {
    return;
  }

  last_pot_sample_time = millis();
  current_pot_value = read_adc_channel(0);
  current_period = period_from_pot(current_pot_value);

  const uint16_t difference =
      (current_period > last_reported_speed_period)
          ? current_period - last_reported_speed_period
          : last_reported_speed_period - current_period;
  if (difference >= 20) {
    request_speed_event(current_period, current_pot_value, SPEED_POT);
  } else if (speed_event_pending && pending_speed_source == SPEED_POT) {
    speed_event_pending = false;
  }
}

void update_adc_streams(void) {
  const unsigned long now = millis();
  for (byte channel = 0; channel < 2; ++channel) {
    if (!adc_stream_enabled[channel] ||
        now - last_adc_stream_time[channel] < adc_stream_interval[channel]) {
      continue;
    }
    last_adc_stream_time[channel] = now;
    send_adc_data(channel);
  }
}
