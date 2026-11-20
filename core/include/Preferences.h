// CardSim Preferences (ESP32 NVS). Stored in a text file next to the SD folder.
#pragma once

#include <map>
#include <string>

#include "Arduino.h"

class Preferences {
 public:
  bool begin(const char* name, bool readOnly = false, const char* partition = nullptr);
  void end();
  bool clear();
  bool remove(const char* key);
  bool isKey(const char* key);

  size_t putInt(const char* k, int32_t v) { return putStr(k, std::to_string(v)); }
  size_t putUInt(const char* k, uint32_t v) { return putStr(k, std::to_string(v)); }
  size_t putLong(const char* k, int32_t v) { return putInt(k, v); }
  size_t putULong(const char* k, uint32_t v) { return putUInt(k, v); }
  size_t putLong64(const char* k, int64_t v) { return putStr(k, std::to_string(v)); }
  size_t putULong64(const char* k, uint64_t v) { return putStr(k, std::to_string(v)); }
  size_t putShort(const char* k, int16_t v) { return putInt(k, v); }
  size_t putUShort(const char* k, uint16_t v) { return putUInt(k, v); }
  size_t putChar(const char* k, int8_t v) { return putInt(k, v); }
  size_t putUChar(const char* k, uint8_t v) { return putUInt(k, v); }
  size_t putBool(const char* k, bool v) { return putUInt(k, v ? 1 : 0); }
  size_t putFloat(const char* k, float v) { return putStr(k, std::to_string(v)); }
  size_t putDouble(const char* k, double v) { return putStr(k, std::to_string(v)); }
  size_t putString(const char* k, const char* v) { return putStr(k, v ? v : ""); }
  size_t putString(const char* k, const String& v) { return putStr(k, v.c_str()); }

  int32_t getInt(const char* k, int32_t d = 0) { return (int32_t)getNum(k, d); }
  uint32_t getUInt(const char* k, uint32_t d = 0) { return (uint32_t)getNum(k, d); }
  int32_t getLong(const char* k, int32_t d = 0) { return getInt(k, d); }
  uint32_t getULong(const char* k, uint32_t d = 0) { return getUInt(k, d); }
  int64_t getLong64(const char* k, int64_t d = 0) { return (int64_t)getNum(k, (double)d); }
  uint64_t getULong64(const char* k, uint64_t d = 0) { return (uint64_t)getNum(k, (double)d); }
  int16_t getShort(const char* k, int16_t d = 0) { return (int16_t)getInt(k, d); }
  uint16_t getUShort(const char* k, uint16_t d = 0) { return (uint16_t)getUInt(k, d); }
  int8_t getChar(const char* k, int8_t d = 0) { return (int8_t)getInt(k, d); }
  uint8_t getUChar(const char* k, uint8_t d = 0) { return (uint8_t)getUInt(k, d); }
  bool getBool(const char* k, bool d = false) { return getUInt(k, d ? 1 : 0) != 0; }
  float getFloat(const char* k, float d = NAN) { return (float)getNum(k, d); }
  double getDouble(const char* k, double d = NAN) { return getNum(k, d); }
  String getString(const char* k, const String& d = String());
  size_t getString(const char* k, char* buf, size_t len);

 private:
  size_t putStr(const char* k, const std::string& v);
  double getNum(const char* k, double d);
  void save();
  std::string ns_;
  bool open_ = false;
  bool ro_ = false;
};
