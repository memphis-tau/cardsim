// CardSim: the subset of the Arduino / ESP32 core API that Cardputer
// sketches commonly use, implemented for a desktop or browser host.
#pragma once

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "HardwareSerial.h"
#include "Print.h"
#include "WString.h"

typedef bool boolean;
typedef uint8_t byte;
typedef uint16_t word;

using std::max;
using std::min;

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

inline double radians(double deg) { return deg * DEG_TO_RAD; }
inline double degrees(double rad) { return rad * RAD_TO_DEG; }
template <typename T>
inline T sq(T x) { return x * x; }
template <typename T, typename L, typename H>
inline T constrain(T x, L lo, H hi) { return x < (T)lo ? (T)lo : (x > (T)hi ? (T)hi : x); }
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Time
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void yield();

// Random
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);
uint32_t esp_random();

// GPIO (no real pins on the host; reads return idle levels)
#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define INPUT_PULLDOWN 0x09
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return HIGH; }
inline uint16_t analogRead(uint8_t) { return 0; }
inline void analogWrite(uint8_t, int) {}

// Minimal ESP object
class EspClass {
 public:
  void restart();
  uint32_t getFreeHeap() { return 280000; }
  uint32_t getHeapSize() { return 320000; }
  uint32_t getFreePsram() { return 0; }
  const char* getChipModel() { return "ESP32-S3 (CardSim)"; }
  uint64_t getEfuseMac() { return 0x0000A1B2C3D4E5F6ULL; }
};
extern EspClass ESP;

// The sketch provides these.
void setup();
void loop();
