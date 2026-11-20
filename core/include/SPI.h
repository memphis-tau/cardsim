// CardSim SPI: nothing to drive on the host.
#pragma once

#include <cstdint>

class SPIClass {
 public:
  explicit SPIClass(uint8_t bus = 0) : bus_(bus) {}
  void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1) {}
  void end() {}
  uint8_t transfer(uint8_t) { return 0xFF; }

 private:
  uint8_t bus_;
};

extern SPIClass SPI;

#define FSPI 0
#define HSPI 1
