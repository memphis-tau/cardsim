// Arduino-compatible String class for the CardSim host build.
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>

class String {
 public:
  String() = default;
  String(const char* s) : s_(s ? s : "") {}
  String(const std::string& s) : s_(s) {}
  String(const String&) = default;
  String(String&&) noexcept = default;
  String& operator=(const String&) = default;
  String& operator=(String&&) noexcept = default;
  String& operator=(const char* s) {
    s_ = s ? s : "";
    return *this;
  }

  explicit String(char c) : s_(1, c) {}
  explicit String(unsigned char v, unsigned char base = 10) { fromUnsigned(v, base); }
  explicit String(int v, unsigned char base = 10) { fromSigned(v, base); }
  explicit String(unsigned int v, unsigned char base = 10) { fromUnsigned(v, base); }
  explicit String(long v, unsigned char base = 10) { fromSigned(v, base); }
  explicit String(unsigned long v, unsigned char base = 10) { fromUnsigned(v, base); }
  explicit String(long long v, unsigned char base = 10) { fromSigned(v, base); }
  explicit String(unsigned long long v, unsigned char base = 10) { fromUnsigned(v, base); }
  explicit String(float v, unsigned int decimals = 2) { fromDouble(v, decimals); }
  explicit String(double v, unsigned int decimals = 2) { fromDouble(v, decimals); }

  // Access
  unsigned int length() const { return (unsigned int)s_.size(); }
  bool isEmpty() const { return s_.empty(); }
  const char* c_str() const { return s_.c_str(); }
  const std::string& str() const { return s_; }
  char charAt(unsigned int i) const { return i < s_.size() ? s_[i] : 0; }
  void setCharAt(unsigned int i, char c) {
    if (i < s_.size()) s_[i] = c;
  }
  char operator[](unsigned int i) const { return charAt(i); }
  char& operator[](unsigned int i) {
    static char dummy;
    if (i >= s_.size()) {
      dummy = 0;
      return dummy;
    }
    return s_[i];
  }
  bool reserve(unsigned int n) {
    s_.reserve(n);
    return true;
  }
  void getBytes(unsigned char* buf, unsigned int len, unsigned int index = 0) const {
    toCharArray((char*)buf, len, index);
  }
  void toCharArray(char* buf, unsigned int len, unsigned int index = 0) const {
    if (!buf || len == 0) return;
    if (index >= s_.size()) {
      buf[0] = 0;
      return;
    }
    size_t n = std::min<size_t>(len - 1, s_.size() - index);
    memcpy(buf, s_.data() + index, n);
    buf[n] = 0;
  }

  // Concatenation
  bool concat(const String& o) { s_ += o.s_; return true; }
  bool concat(const char* o) { if (o) s_ += o; return true; }
  bool concat(char c) { s_ += c; return true; }
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value &&
                                                    !std::is_same<T, char>::value &&
                                                    !std::is_same<T, bool>::value,
                                                int>::type = 0>
  bool concat(T v) { s_ += String(v).s_; return true; }

  String& operator+=(const String& o) { concat(o); return *this; }
  String& operator+=(const char* o) { concat(o); return *this; }
  String& operator+=(char c) { concat(c); return *this; }
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value &&
                                                    !std::is_same<T, char>::value &&
                                                    !std::is_same<T, bool>::value,
                                                int>::type = 0>
  String& operator+=(T v) { concat(v); return *this; }

  // Comparison
  int compareTo(const String& o) const { return s_.compare(o.s_); }
  bool equals(const String& o) const { return s_ == o.s_; }
  bool equals(const char* o) const { return s_ == (o ? o : ""); }
  bool equalsIgnoreCase(const String& o) const {
    if (s_.size() != o.s_.size()) return false;
    for (size_t i = 0; i < s_.size(); i++)
      if (tolower((unsigned char)s_[i]) != tolower((unsigned char)o.s_[i])) return false;
    return true;
  }
  bool operator==(const String& o) const { return s_ == o.s_; }
  bool operator==(const char* o) const { return equals(o); }
  bool operator!=(const String& o) const { return s_ != o.s_; }
  bool operator!=(const char* o) const { return !equals(o); }
  bool operator<(const String& o) const { return s_ < o.s_; }
  bool operator>(const String& o) const { return s_ > o.s_; }
  bool operator<=(const String& o) const { return s_ <= o.s_; }
  bool operator>=(const String& o) const { return s_ >= o.s_; }
  bool startsWith(const String& p) const { return s_.compare(0, p.s_.size(), p.s_) == 0; }
  bool startsWith(const String& p, unsigned int offset) const {
    return offset <= s_.size() && s_.compare(offset, p.s_.size(), p.s_) == 0;
  }
  bool endsWith(const String& p) const {
    return p.s_.size() <= s_.size() &&
           s_.compare(s_.size() - p.s_.size(), p.s_.size(), p.s_) == 0;
  }

  // Search
  int indexOf(char c, unsigned int from = 0) const { return pos(s_.find(c, from)); }
  int indexOf(const String& t, unsigned int from = 0) const { return pos(s_.find(t.s_, from)); }
  int lastIndexOf(char c) const { return pos(s_.rfind(c)); }
  int lastIndexOf(char c, unsigned int from) const { return pos(s_.rfind(c, from)); }
  int lastIndexOf(const String& t) const { return pos(s_.rfind(t.s_)); }
  int lastIndexOf(const String& t, unsigned int from) const { return pos(s_.rfind(t.s_, from)); }

  String substring(unsigned int from) const { return substring(from, length()); }
  String substring(unsigned int from, unsigned int to) const {
    if (from > to) std::swap(from, to);
    if (from >= s_.size()) return String();
    if (to > s_.size()) to = (unsigned int)s_.size();
    return String(s_.substr(from, to - from));
  }

  // Modification
  void replace(char find, char repl) {
    for (auto& c : s_)
      if (c == find) c = repl;
  }
  void replace(const String& find, const String& repl) {
    if (find.s_.empty()) return;
    size_t p = 0;
    while ((p = s_.find(find.s_, p)) != std::string::npos) {
      s_.replace(p, find.s_.size(), repl.s_);
      p += repl.s_.size();
    }
  }
  void remove(unsigned int index) {
    if (index < s_.size()) s_.erase(index);
  }
  void remove(unsigned int index, unsigned int count) {
    if (index < s_.size()) s_.erase(index, count);
  }
  void toLowerCase() {
    for (auto& c : s_) c = (char)tolower((unsigned char)c);
  }
  void toUpperCase() {
    for (auto& c : s_) c = (char)toupper((unsigned char)c);
  }
  void trim() {
    size_t a = 0, b = s_.size();
    while (a < b && isspace((unsigned char)s_[a])) a++;
    while (b > a && isspace((unsigned char)s_[b - 1])) b--;
    s_ = s_.substr(a, b - a);
  }

  // Conversion
  long toInt() const { return atol(s_.c_str()); }
  float toFloat() const { return (float)atof(s_.c_str()); }
  double toDouble() const { return atof(s_.c_str()); }

 private:
  static int pos(size_t p) { return p == std::string::npos ? -1 : (int)p; }
  void fromUnsigned(unsigned long long v, unsigned char base) {
    if (base < 2 || base > 36) base = 10;
    char buf[72];
    int i = 70;
    buf[71] = 0;
    if (v == 0) buf[i--] = '0';
    while (v) {
      int d = (int)(v % base);
      buf[i--] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
      v /= base;
    }
    s_ = &buf[i + 1];
  }
  void fromSigned(long long v, unsigned char base) {
    if (v < 0 && base == 10) {
      fromUnsigned((unsigned long long)(-(v + 1)) + 1, base);
      s_.insert(s_.begin(), '-');
    } else {
      fromUnsigned((unsigned long long)(unsigned long)v, base);
    }
  }
  void fromDouble(double v, unsigned int decimals) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", (int)decimals, v);
    s_ = buf;
  }

  std::string s_;
};

inline String operator+(const String& a, const String& b) {
  String r(a);
  r += b;
  return r;
}
inline String operator+(const String& a, const char* b) {
  String r(a);
  r += b;
  return r;
}
inline String operator+(const char* a, const String& b) {
  String r(a);
  r += b;
  return r;
}
inline String operator+(const String& a, char c) {
  String r(a);
  r += c;
  return r;
}
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value &&
                                                  !std::is_same<T, char>::value &&
                                                  !std::is_same<T, bool>::value,
                                              int>::type = 0>
inline String operator+(const String& a, T v) {
  String r(a);
  r += v;
  return r;
}
inline bool operator==(const char* a, const String& b) { return b == a; }
inline bool operator!=(const char* a, const String& b) { return b != a; }

// Arduino's F() macro (flash strings) is a no-op on the host.
#ifndef F
#define F(s) (s)
#endif
