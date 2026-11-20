// CardSim serial ports. Serial (UART 0) prints to the host console.
// Other UARTs receive data from simulated peripherals (the GPS module).
#pragma once

#include <cstdint>
#include <deque>
#include <vector>

#include "Print.h"

#define SERIAL_8N1 0x800001c
#define SERIAL_8E1 0x800001e
#define SERIAL_8O1 0x800001f

class HardwareSerial : public Stream {
 public:
  explicit HardwareSerial(int uart);
  ~HardwareSerial();
  HardwareSerial(const HardwareSerial&) = delete;
  HardwareSerial& operator=(const HardwareSerial&) = delete;

  void begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1,
             int8_t txPin = -1, bool invert = false, unsigned long timeout_ms = 20000UL,
             uint8_t rxfifo_full_thrhd = 112);
  void end();
  void setRxBufferSize(size_t) {}
  void setTxBufferSize(size_t) {}

  int available() override { return (int)rx_.size(); }
  int read() override {
    if (rx_.empty()) return -1;
    int c = rx_.front();
    rx_.pop_front();
    return c;
  }
  int peek() override { return rx_.empty() ? -1 : rx_.front(); }

  using Print::write;
  size_t write(uint8_t c) override;
  size_t write(const uint8_t* buf, size_t n) override;
  void flush() override;
  operator bool() const { return true; }

  // Simulator side: is this port open at this baud rate?
  bool isOpenAt(unsigned long baud) const { return open_ && baud_ == baud; }
  bool isOpen() const { return open_; }
  unsigned long baud() const { return baud_; }
  void injectRx(const char* data);
  int uartNum() const { return uart_; }

 private:
  int uart_;
  bool open_ = false;
  unsigned long baud_ = 0;
  std::deque<uint8_t> rx_;
};

// Every port that exists, so simulated peripherals can find open ones.
const std::vector<HardwareSerial*>& allSerialPorts();

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
