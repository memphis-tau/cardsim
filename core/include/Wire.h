// CardSim I2C: no devices answer on the host bus.
#pragma once

#include <cstdint>

#include "Print.h"

class TwoWire : public Stream {
 public:
  bool begin(int sda = -1, int scl = -1, uint32_t freq = 0) { return true; }
  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 2; }  // NACK on address
  uint8_t requestFrom(uint8_t, uint8_t, bool = true) { return 0; }
  void setClock(uint32_t) {}
  using Print::write;
  size_t write(uint8_t) override { return 1; }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};

extern TwoWire Wire;
extern TwoWire Wire1;
