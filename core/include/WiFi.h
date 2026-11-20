// CardSim Wi-Fi: scanning returns networks from the simulated world, with
// signal strength computed from your position. Joining networks is not
// simulated (WiFi.begin() never connects).
#pragma once

#include <cstdint>
#include <vector>

#include "Arduino.h"

typedef enum { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 } wifi_mode_t;
typedef enum {
  WIFI_AUTH_OPEN = 0,
  WIFI_AUTH_WEP,
  WIFI_AUTH_WPA_PSK,
  WIFI_AUTH_WPA2_PSK,
  WIFI_AUTH_WPA_WPA2_PSK,
  WIFI_AUTH_WPA2_ENTERPRISE,
  WIFI_AUTH_WPA3_PSK,
  WIFI_AUTH_WPA2_WPA3_PSK,
} wifi_auth_mode_t;
typedef enum {
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6
} wl_status_t;

#define WIFI_SCAN_RUNNING (-1)
#define WIFI_SCAN_FAILED (-2)

class IPAddress : public Print {
 public:
  IPAddress(uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0) : o_{a, b, c, d} {}
  String toString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", o_[0], o_[1], o_[2], o_[3]);
    return String(buf);
  }
  size_t write(uint8_t) override { return 0; }

 private:
  uint8_t o_[4];
};

class WiFiClass {
 public:
  bool mode(wifi_mode_t m) {
    mode_ = m;
    return true;
  }
  wifi_mode_t getMode() const { return mode_; }
  bool disconnect(bool = false, bool = false) { return true; }
  wl_status_t begin(const char*, const char* = nullptr) { return WL_DISCONNECTED; }
  wl_status_t status() const { return WL_DISCONNECTED; }
  bool isConnected() const { return false; }
  IPAddress localIP() const { return IPAddress(); }
  String macAddress() const { return String("A1:B2:C3:D4:E5:F6"); }
  void setSleep(bool) {}

  // Scanning
  int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false,
                       uint32_t max_ms_per_chan = 300, uint8_t channel = 0,
                       const char* ssid = nullptr, const uint8_t* bssid = nullptr);
  int16_t scanComplete();
  void scanDelete();

  String SSID(uint8_t i) const;
  String BSSIDstr(uint8_t i) const;
  uint8_t* BSSID(uint8_t i);
  int32_t RSSI(uint8_t i) const;
  int32_t channel(uint8_t i) const;
  wifi_auth_mode_t encryptionType(uint8_t i) const;
  String SSID() const { return String(); }  // connected SSID: never connected
  int32_t RSSI() const { return 0; }

  // Simulator side
  struct Result {
    String ssid;
    uint8_t bssid[6];
    int32_t rssi;
    int32_t channel;
    wifi_auth_mode_t auth;
  };

 private:
  wifi_mode_t mode_ = WIFI_OFF;
  bool scanning_ = false;
  bool hasResults_ = false;
  bool showHidden_ = false;
  uint32_t doneAt_ = 0;
  std::vector<Result> results_;
  void finishScan();
};

extern WiFiClass WiFi;
