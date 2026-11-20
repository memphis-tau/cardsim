// CardSim: M5Cardputer library (Cardputer and Cardputer ADV).
#pragma once

#include <cstdint>
#include <vector>

#include "M5Unified.h"

#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82
#define KEY_FN 0xff
#define KEY_OPT 0x00
#define KEY_BACKSPACE 0x2a
#define KEY_TAB 0x2b
#define KEY_ENTER 0x28

struct Point2D_t {
  int x;
  int y;
};

class Keyboard_Class {
 public:
  struct KeysState {
    bool tab = false;
    bool fn = false;
    bool shift = false;
    bool ctrl = false;
    bool opt = false;
    bool alt = false;
    bool del = false;
    bool backspace = false;  // same key as del
    bool enter = false;
    bool space = false;
    uint8_t modifiers = 0;
    std::vector<char> word;
    std::vector<uint8_t> hid_keys;
    std::vector<uint8_t> modifier_keys;
    void reset() { *this = KeysState(); }
  };

  void begin() {}
  void updateKeyList();
  void updateKeysState() {}
  bool isChange() const { return changed_; }
  uint8_t isPressed() const { return pressed_ ? 1 : 0; }
  bool isKeyPressed(char c) const;
  KeysState& keysState() { return state_; }
  std::vector<Point2D_t>& keyList() { return keyList_; }
  bool capslocked() const { return false; }
  void setCapsLocked(bool) {}

 private:
  bool changed_ = false;
  bool pressed_ = false;
  KeysState state_;
  std::vector<Point2D_t> keyList_;
};

class M5_CARDPUTER {
 public:
  M5_CARDPUTER();
  void begin(bool enableKeyboard = true);
  void begin(const m5::M5Unified::config_t& cfg, bool enableKeyboard = true);
  void update();

  M5GFX& Display;
  M5GFX& Lcd;
  Keyboard_Class Keyboard;
  m5::Speaker_Class& Speaker;
  m5::Button_Class& BtnA;
  m5::Power_Class& Power;
};

extern M5_CARDPUTER M5Cardputer;
