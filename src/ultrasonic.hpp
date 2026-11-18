#ifndef ULTRASONIC_HPP
#define ULTRASONIC_HPP

#include <Arduino.h>

// timeout: us (microsec)
float read_distance(byte trig, byte echo, unsigned long timeout);

#endif
