#include <Arduino.h>
#include <DHT.h>
#include "led.hpp"

#define DHT_PIN 10

DHT dht(DHT_PIN, DHT22);

void setup(void) {
  Serial.begin(9600);
  dht.begin();
  Serial.println("DHT22 start");
  for (int i = 0; i < LED_NO; ++i) {
    pinMode(FIRST_LED + i, OUTPUT);
  }
}

void loop(void) {
  delay(2000);
  float hum = dht.readHumidity();
  if (!isnan(hum)) {
    byte v = led_no(hum / 12.5);
    led_write(v);
  }
}
