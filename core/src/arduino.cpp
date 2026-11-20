// CardSim: Arduino core functions, serial ports, ESP object.
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "Arduino.h"
#include "cardsim/sim.h"

namespace cardsim {
uint32_t rand32();
void seed(uint32_t s);
}  // namespace cardsim

uint32_t millis() { return cardsim::now_ms(); }
uint32_t micros() { return cardsim::now_ms() * 1000u; }
void delay(uint32_t ms) { cardsim::advance(ms); }
void delayMicroseconds(uint32_t us) {
  if (us >= 1000) cardsim::advance(us / 1000);
}
void yield() { cardsim::service(); }

long random(long howbig) {
  if (howbig <= 0) return 0;
  return (long)(cardsim::rand32() % (uint32_t)howbig);
}
long random(long howsmall, long howbig) {
  if (howsmall >= howbig) return howsmall;
  return howsmall + random(howbig - howsmall);
}
void randomSeed(unsigned long s) { cardsim::seed((uint32_t)s); }
uint32_t esp_random() { return cardsim::rand32(); }

EspClass ESP;
void EspClass::restart() {
  cardsim::log("ESP.restart() called; exiting the simulator");
  std::exit(0);
}

// ---------------------------------------------------------------- serial

static std::vector<HardwareSerial*>& ports() {
  static std::vector<HardwareSerial*> v;
  return v;
}
const std::vector<HardwareSerial*>& allSerialPorts() { return ports(); }

HardwareSerial::HardwareSerial(int uart) : uart_(uart) { ports().push_back(this); }
HardwareSerial::~HardwareSerial() {
  auto& v = ports();
  v.erase(std::remove(v.begin(), v.end(), this), v.end());
}

void HardwareSerial::begin(unsigned long baud, uint32_t, int8_t, int8_t, bool, unsigned long,
                           uint8_t) {
  open_ = true;
  baud_ = baud;
  rx_.clear();
}

void HardwareSerial::end() {
  open_ = false;
  rx_.clear();
}

size_t HardwareSerial::write(uint8_t c) {
  if (uart_ == 0) fputc(c, stdout);
  return 1;
}

size_t HardwareSerial::write(const uint8_t* buf, size_t n) {
  if (uart_ == 0) fwrite(buf, 1, n, stdout);
  return n;
}

void HardwareSerial::flush() {
  if (uart_ == 0) fflush(stdout);
}

void HardwareSerial::injectRx(const char* data) {
  if (!open_) return;
  for (const char* p = data; *p; p++) rx_.push_back((uint8_t)*p);
  while (rx_.size() > 4096) rx_.pop_front();  // like a UART FIFO overflowing
}

HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);
