// CardSim Wi-Fi scanning against the simulated world.
#include <algorithm>
#include <cstring>

#include "WiFi.h"
#include "cardsim/sim.h"

// An all-channel active scan on the ESP32-S3 takes roughly 2 to 3 seconds.
static const uint32_t kScanMs = 2300;

int16_t WiFiClass::scanNetworks(bool async, bool show_hidden, bool, uint32_t, uint8_t,
                                const char*, const uint8_t*) {
  if (mode_ == WIFI_OFF) mode_ = WIFI_STA;  // the real core switches to STA too
  if (scanning_) return WIFI_SCAN_RUNNING;
  scanDelete();
  scanning_ = true;
  showHidden_ = show_hidden;
  doneAt_ = millis() + kScanMs;
  if (async) return WIFI_SCAN_RUNNING;
  while (millis() < doneAt_) delay(10);
  finishScan();
  return (int16_t)results_.size();
}

void WiFiClass::finishScan() {
  scanning_ = false;
  hasResults_ = true;
  results_.clear();
  const cardsim::World& w = cardsim::world();
  for (const auto& ap : w.aps) {
    if (ap.ssid.empty() && !showHidden_) continue;
    double r = cardsim::rssi_at(ap, w.x, w.y, true);
    if (r < w.floor_dbm) continue;
    if (r > -20) r = -20;
    Result res;
    res.ssid = String(ap.ssid.c_str());
    memcpy(res.bssid, ap.bssid, 6);
    res.rssi = (int32_t)std::lround(r);
    res.channel = ap.channel;
    res.auth = (wifi_auth_mode_t)ap.auth;
    results_.push_back(res);
  }
  std::sort(results_.begin(), results_.end(),
            [](const Result& a, const Result& b) { return a.rssi > b.rssi; });
}

int16_t WiFiClass::scanComplete() {
  if (scanning_) {
    if (millis() < doneAt_) return WIFI_SCAN_RUNNING;
    finishScan();
  }
  if (!hasResults_) return WIFI_SCAN_FAILED;
  return (int16_t)results_.size();
}

void WiFiClass::scanDelete() {
  results_.clear();
  hasResults_ = false;
}

String WiFiClass::SSID(uint8_t i) const {
  return i < results_.size() ? results_[i].ssid : String();
}

String WiFiClass::BSSIDstr(uint8_t i) const {
  if (i >= results_.size()) return String();
  const uint8_t* b = results_[i].bssid;
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", b[0], b[1], b[2], b[3], b[4],
           b[5]);
  return String(buf);
}

uint8_t* WiFiClass::BSSID(uint8_t i) { return i < results_.size() ? results_[i].bssid : nullptr; }

int32_t WiFiClass::RSSI(uint8_t i) const { return i < results_.size() ? results_[i].rssi : 0; }

int32_t WiFiClass::channel(uint8_t i) const {
  return i < results_.size() ? results_[i].channel : 0;
}

wifi_auth_mode_t WiFiClass::encryptionType(uint8_t i) const {
  return i < results_.size() ? results_[i].auth : WIFI_AUTH_OPEN;
}

WiFiClass WiFi;
