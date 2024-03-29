#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#else 
#define PI 3.1415926535897932384626433832795
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#endif

#define SAMPLINGRATE 44100
#define T_SAMPLE (1.0/SAMPLINGRATE)
