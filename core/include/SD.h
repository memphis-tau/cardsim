// CardSim microSD. SD.begin() succeeds if the simulated card is inserted;
// files live in a host folder (./sdcard by default, /sdcard in the browser).
#pragma once

#include "FS.h"
#include "SPI.h"

typedef enum { CARD_NONE, CARD_MMC, CARD_SD, CARD_SDHC, CARD_UNKNOWN } sdcard_type_t;

class SDFS : public fs::FS {
 public:
  bool begin(uint8_t ssPin = 4, SPIClass& spi = SPI, uint32_t frequency = 4000000,
             const char* mountpoint = "/sd", uint8_t max_files = 5, bool format_if_empty = false);
  void end() { mounted_ = false; }
  sdcard_type_t cardType() const { return mounted_ ? CARD_SDHC : CARD_NONE; }
  uint64_t cardSize() const { return 16ULL * 1024 * 1024 * 1024; }
  uint64_t totalBytes() const { return cardSize(); }
  uint64_t usedBytes() const { return 1024 * 1024; }

 protected:
  bool mounted() const override { return mounted_; }

 private:
  bool mounted_ = false;
};

extern SDFS SD;
