// CardSim: the parts of M5Unified a Cardputer sketch usually touches.
#pragma once

#include <cstdint>

#include "Arduino.h"
#include "M5GFX.h"

namespace m5 {

struct imu_3d_t {
  float x = 0, y = 0, z = 0;
};
struct imu_data_t {
  uint32_t usec = 0;
  imu_3d_t accel;  // g
  imu_3d_t gyro;   // deg/s
  imu_3d_t mag;
};

class IMU_Class {
 public:
  bool begin() { return isEnabled(); }
  bool isEnabled() const;
  // Returns non-zero when new data is available (M5Unified returns a sensor mask).
  int update();
  const imu_data_t& getImuData() const { return data_; }
  bool getAccel(float* x, float* y, float* z) const;
  bool getAccelData(float* x, float* y, float* z) const { return getAccel(x, y, z); }
  bool getGyro(float* x, float* y, float* z) const;
  bool getGyroData(float* x, float* y, float* z) const { return getGyro(x, y, z); }
  bool getTemp(float* t) const {
    *t = 24.5f;
    return isEnabled();
  }

 private:
  imu_data_t data_;
};

class Speaker_Class {
 public:
  bool begin() { return true; }
  void end() {}
  bool isEnabled() const { return true; }
  bool isPlaying() const { return false; }
  void setVolume(uint8_t v) { vol_ = v; }
  uint8_t getVolume() const { return vol_; }
  bool tone(float freq, uint32_t ms = UINT32_MAX, int = -1, bool = true);
  void stop() {}

 private:
  uint8_t vol_ = 64;
};

class Power_Class {
 public:
  int32_t getBatteryLevel() { return 87; }
  int16_t getBatteryVoltage() { return 3980; }
  bool isCharging() { return false; }
  void powerOff();
  void deepSleep(uint64_t = 0, bool = true) { powerOff(); }
};

class Button_Class {
 public:
  explicit Button_Class(int id) : id_(id) {}
  bool isPressed() const { return pressed_; }
  bool isReleased() const { return !pressed_; }
  bool wasPressed() const { return pressed_ && changed_; }
  bool wasReleased() const { return !pressed_ && changed_; }
  bool wasClicked() const { return wasReleased(); }
  bool pressedFor(uint32_t ms) const { return pressed_ && millis() - since_ >= ms; }
  bool releasedFor(uint32_t ms) const { return !pressed_ && millis() - since_ >= ms; }
  bool wasHold() const { return false; }
  void update();  // pulls the host state

 private:
  int id_;
  bool pressed_ = false, changed_ = false;
  uint32_t since_ = 0;
};

class M5Unified {
 public:
  struct config_t {
    uint32_t serial_baudrate = 115200;
    bool clear_display = true;
    bool output_power = true;
    bool internal_imu = true;
    bool internal_rtc = true;
    bool internal_spk = true;
    bool internal_mic = true;
    bool external_imu = false;
    bool external_rtc = false;
    uint8_t fallback_board = 0;
  };
  config_t config() const { return config_t(); }
  void begin() { begin(config_t()); }
  void begin(const config_t&);
  void update();
  uint8_t getBoard() const { return 0x0E; }  // board_M5Cardputer

  M5GFX& Display;
  M5GFX& Lcd;
  IMU_Class Imu;
  Speaker_Class Speaker;
  Power_Class Power;
  Button_Class BtnA{0};
  Button_Class BtnB{1};
  Button_Class BtnC{2};
  Button_Class BtnPWR{3};

  M5Unified();
};

}  // namespace m5

extern m5::M5Unified M5;
using m5::imu_data_t;
