// Arduino-compatible Print and Stream for the CardSim host build.
#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "WString.h"

#ifndef DEC
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#endif

class Print {
 public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* buf, size_t n) {
    size_t k = 0;
    while (n--) k += write(*buf++);
    return k;
  }
  size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }
  size_t write(const char* b, size_t n) { return write((const uint8_t*)b, n); }
  virtual void flush() {}

  size_t print(const char* s) { return write(s); }
  size_t print(const String& s) { return write(s.c_str(), s.length()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char v, int base = DEC) { return printUnsigned(v, base); }
  size_t print(int v, int base = DEC) { return printSigned(v, base); }
  size_t print(unsigned int v, int base = DEC) { return printUnsigned(v, base); }
  size_t print(long v, int base = DEC) { return printSigned(v, base); }
  size_t print(unsigned long v, int base = DEC) { return printUnsigned(v, base); }
  size_t print(long long v, int base = DEC) { return printSigned(v, base); }
  size_t print(unsigned long long v, int base = DEC) { return printUnsigned(v, base); }
  size_t print(double v, int digits = 2) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", digits, v);
    return write(buf);
  }
  size_t print(bool b) { return print((int)b); }

  size_t println() { return write("\r\n"); }
  template <typename T>
  size_t println(const T& v) {
    size_t n = print(v);
    return n + println();
  }
  template <typename T>
  size_t println(const T& v, int fmt) {
    size_t n = print(v, fmt);
    return n + println();
  }

  size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
    char stackbuf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap);
    va_end(ap);
    if (n < 0) return 0;
    if ((size_t)n < sizeof(stackbuf)) return write(stackbuf, (size_t)n);
    char* big = new char[n + 1];
    va_start(ap, fmt);
    vsnprintf(big, n + 1, fmt, ap);
    va_end(ap);
    size_t r = write(big, (size_t)n);
    delete[] big;
    return r;
  }

 private:
  size_t printUnsigned(unsigned long long v, int base) {
    return print(String(v, (unsigned char)base));
  }
  size_t printSigned(long long v, int base) {
    if (base == DEC) return print(String(v));
    return printUnsigned((unsigned long long)(uint32_t)v, base);
  }
};

class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;

  void setTimeout(unsigned long ms) { timeout_ = ms; }

  size_t readBytes(char* buf, size_t len) {
    size_t n = 0;
    while (n < len) {
      int c = read();
      if (c < 0) break;
      buf[n++] = (char)c;
    }
    return n;
  }
  size_t readBytes(uint8_t* buf, size_t len) { return readBytes((char*)buf, len); }

  String readStringUntil(char terminator) {
    String s;
    int c;
    while ((c = read()) >= 0 && c != terminator) s += (char)c;
    return s;
  }
  String readString() {
    String s;
    int c;
    while ((c = read()) >= 0) s += (char)c;
    return s;
  }

 protected:
  unsigned long timeout_ = 1000;
};
