// CardSim graphics: the commonly used part of the M5GFX / LovyanGFX API.
// Colours are RGB565. All fonts render with the classic 6x8 font (Font0),
// scaled to roughly the requested size.
#pragma once

#include <cstdint>
#include <vector>

#include "Print.h"
#include "WString.h"

// Colours (M5GFX defines both the plain and the TFT_ names)
constexpr uint16_t TFT_BLACK = 0x0000, TFT_NAVY = 0x000F, TFT_DARKGREEN = 0x03E0,
                   TFT_DARKCYAN = 0x03EF, TFT_MAROON = 0x7800, TFT_PURPLE = 0x780F,
                   TFT_OLIVE = 0x7BE0, TFT_LIGHTGREY = 0xD69A, TFT_LIGHTGRAY = 0xD69A,
                   TFT_DARKGREY = 0x7BEF, TFT_DARKGRAY = 0x7BEF, TFT_BLUE = 0x001F,
                   TFT_GREEN = 0x07E0, TFT_CYAN = 0x07FF, TFT_RED = 0xF800,
                   TFT_MAGENTA = 0xF81F, TFT_YELLOW = 0xFFE0, TFT_WHITE = 0xFFFF,
                   TFT_ORANGE = 0xFDA0, TFT_GREENYELLOW = 0xB7E0, TFT_PINK = 0xFE19,
                   TFT_BROWN = 0x9A60, TFT_GOLD = 0xFEA0, TFT_SILVER = 0xC618,
                   TFT_SKYBLUE = 0x867D, TFT_VIOLET = 0x915C;
constexpr uint16_t BLACK = TFT_BLACK, NAVY = TFT_NAVY, DARKGREEN = TFT_DARKGREEN,
                   DARKCYAN = TFT_DARKCYAN, MAROON = TFT_MAROON, PURPLE = TFT_PURPLE,
                   OLIVE = TFT_OLIVE, LIGHTGREY = TFT_LIGHTGREY, DARKGREY = TFT_DARKGREY,
                   BLUE = TFT_BLUE, GREEN = TFT_GREEN, CYAN = TFT_CYAN, RED = TFT_RED,
                   MAGENTA = TFT_MAGENTA, YELLOW = TFT_YELLOW, WHITE = TFT_WHITE,
                   ORANGE = TFT_ORANGE, GREENYELLOW = TFT_GREENYELLOW, PINK = TFT_PINK;

// Text datum (LovyanGFX values)
constexpr uint8_t top_left = 0, top_center = 1, top_centre = 1, top_right = 2,
                  middle_left = 4, middle_center = 5, middle_centre = 5, middle_right = 6,
                  bottom_left = 8, bottom_center = 9, bottom_centre = 9, bottom_right = 10,
                  baseline_left = 16, baseline_center = 17, baseline_right = 18;
constexpr uint8_t TL_DATUM = 0, TC_DATUM = 1, TR_DATUM = 2, ML_DATUM = 4, MC_DATUM = 5,
                  MR_DATUM = 6, BL_DATUM = 8, BC_DATUM = 9, BR_DATUM = 10;

namespace fonts {
struct IFont {
  uint8_t scale;  // multiple of the 6x8 classic font used to stand in for it
};
extern const IFont Font0, Font2, Font4, Font6, Font7, Font8, AsciiFont8x16, AsciiFont24x48, TomThumb, DejaVu9, DejaVu12, DejaVu18, DejaVu24, DejaVu40, DejaVu56, DejaVu72, Orbitron_Light_24, Orbitron_Light_32, Roboto_Thin_24, Satisfy_24, Yellowtail_32, efontCN_12, efontCN_16, efontJA_12, efontJA_16, efontKR_12, efontKR_16, efontTW_12, efontTW_16, lgfxJapanGothic_12, lgfxJapanGothic_16, lgfxJapanMincho_12, lgfxJapanMincho_16, FreeMono9pt7b, FreeMono12pt7b, FreeMono18pt7b, FreeMono24pt7b, FreeMonoBold9pt7b, FreeMonoBold12pt7b, FreeMonoBold18pt7b, FreeMonoBold24pt7b, FreeMonoOblique9pt7b, FreeMonoOblique12pt7b, FreeMonoOblique18pt7b, FreeMonoOblique24pt7b, FreeMonoBoldOblique9pt7b, FreeMonoBoldOblique12pt7b, FreeMonoBoldOblique18pt7b, FreeMonoBoldOblique24pt7b, FreeSans9pt7b, FreeSans12pt7b, FreeSans18pt7b, FreeSans24pt7b, FreeSansBold9pt7b, FreeSansBold12pt7b, FreeSansBold18pt7b, FreeSansBold24pt7b, FreeSansOblique9pt7b, FreeSansOblique12pt7b, FreeSansOblique18pt7b, FreeSansOblique24pt7b, FreeSansBoldOblique9pt7b, FreeSansBoldOblique12pt7b, FreeSansBoldOblique18pt7b, FreeSansBoldOblique24pt7b, FreeSerif9pt7b, FreeSerif12pt7b, FreeSerif18pt7b, FreeSerif24pt7b, FreeSerifBold9pt7b, FreeSerifBold12pt7b, FreeSerifBold18pt7b, FreeSerifBold24pt7b, FreeSerifItalic9pt7b, FreeSerifItalic12pt7b, FreeSerifItalic18pt7b, FreeSerifItalic24pt7b, FreeSerifBoldItalic9pt7b, FreeSerifBoldItalic12pt7b, FreeSerifBoldItalic18pt7b, FreeSerifBoldItalic24pt7b;
}  // namespace fonts
using fonts::IFont;

class LGFXBase : public Print {
 public:
  using Print::write;
  size_t write(uint8_t c) override;

  int32_t width() const { return w_; }
  int32_t height() const { return h_; }

  static constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }
  static constexpr uint16_t color888(uint8_t r, uint8_t g, uint8_t b) {
    return color565(r, g, b);
  }

  // Drawing
  void drawPixel(int32_t x, int32_t y, uint32_t color) { plot(x, y, (uint16_t)color); }
  void writePixel(int32_t x, int32_t y, uint32_t color) { plot(x, y, (uint16_t)color); }
  uint16_t readPixel(int32_t x, int32_t y);
  void fillScreen(uint32_t color) { fillRect(0, 0, w_, h_, color); }
  void clear(uint32_t color = 0) { fillScreen(color); }
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) { fillRect(x, y, w, 1, color); }
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) { fillRect(x, y, 1, h, color); }
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
  void drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
  void fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
  void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
  void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data);

  // Text
  void setCursor(int32_t x, int32_t y) { cx_ = x; cy_ = y; }
  void setCursor(int32_t x, int32_t y, uint8_t) { setCursor(x, y); }
  int32_t getCursorX() const { return cx_; }
  int32_t getCursorY() const { return cy_; }
  void setTextColor(uint32_t fg) { fg_ = (uint16_t)fg; bg_ = (uint16_t)fg; }
  void setTextColor(uint32_t fg, uint32_t bg) { fg_ = (uint16_t)fg; bg_ = (uint16_t)bg; }
  void setTextSize(float s) { setTextSize(s, s); }
  void setTextSize(float sx, float sy);
  float getTextSizeX() const { return tsx_; }
  float getTextSizeY() const { return tsy_; }
  void setTextWrap(bool wrapX, bool wrapY = false) { wrapX_ = wrapX; wrapY_ = wrapY; }
  void setTextScroll(bool on) { scroll_ = on; }
  void setBaseColor(uint32_t c) { base_ = (uint16_t)c; }
  void scroll(int32_t dx, int32_t dy);
  void setTextDatum(uint8_t d) { datum_ = d; }
  uint8_t getTextDatum() const { return datum_; }
  void setTextPadding(uint32_t) {}
  void setFont(const IFont* f) { fontScale_ = f ? f->scale : 1; }
  void setFont(const IFont& f) { fontScale_ = f.scale; }
  void setTextFont(int n);
  void setTextFont(const IFont* f) { setFont(f); }
  void setTextFont(const IFont& f) { setFont(f); }
  const IFont* getFont() const { return nullptr; }

  int32_t textWidth(const char* s) const;
  int32_t textWidth(const String& s) const { return textWidth(s.c_str()); }
  int32_t fontHeight() const { return charH(); }
  int32_t fontWidth() const { return charW(); }
  int32_t drawString(const char* s, int32_t x, int32_t y);
  int32_t drawString(const String& s, int32_t x, int32_t y) { return drawString(s.c_str(), x, y); }
  int32_t drawCentreString(const char* s, int32_t x, int32_t y, uint8_t = 0);
  int32_t drawCenterString(const char* s, int32_t x, int32_t y, uint8_t f = 0) {
    return drawCentreString(s, x, y, f);
  }
  int32_t drawRightString(const char* s, int32_t x, int32_t y, uint8_t = 0);
  int32_t drawChar(uint16_t c, int32_t x, int32_t y);
  int32_t drawNumber(long v, int32_t x, int32_t y) { return drawString(String(v), x, y); }
  int32_t drawFloat(float v, uint8_t dp, int32_t x, int32_t y) {
    return drawString(String(v, (unsigned int)dp), x, y);
  }

  // No-ops on the host
  void startWrite() {}
  void endWrite() {}
  void display() {}
  void waitDisplay() {}
  void setColorDepth(int) {}
  void setSwapBytes(bool) {}
  void setBrightness(uint8_t b) { brightness_ = b; }
  uint8_t getBrightness() const { return brightness_; }
  void sleep() {}
  void wakeup() {}
  void powerSaveOn() {}
  void powerSaveOff() {}

 protected:
  virtual void putPixel(int32_t x, int32_t y, uint16_t c) = 0;  // in range, logical coords
  virtual uint16_t getPixel(int32_t x, int32_t y) = 0;
  void plot(int32_t x, int32_t y, uint16_t c) {
    if (x >= 0 && y >= 0 && x < w_ && y < h_) putPixel(x, y, c);
  }
  int32_t charW() const { return (int32_t)(6 * tsx_ * fontScale_); }
  int32_t charH() const { return (int32_t)(8 * tsy_ * fontScale_); }
  void glyph(int32_t x, int32_t y, uint8_t c, bool paintBg);
  void hspan(int32_t x0, int32_t x1, int32_t y, uint16_t c);

  int32_t w_ = 0, h_ = 0;
  int32_t cx_ = 0, cy_ = 0;
  uint16_t fg_ = TFT_WHITE, bg_ = TFT_WHITE;
  float tsx_ = 1, tsy_ = 1;
  uint8_t fontScale_ = 1;
  bool wrapX_ = true, wrapY_ = false;
  bool scroll_ = false;
  uint16_t base_ = TFT_BLACK;
  void newline();
  uint8_t datum_ = top_left;
  uint8_t brightness_ = 128;
};

// The Cardputer's 240 x 135 ST7789 panel.
class M5GFX : public LGFXBase {
 public:
  static constexpr int PANEL_W = 240, PANEL_H = 135;
  M5GFX();
  bool begin() { return true; }
  bool init() { return true; }
  void setRotation(uint8_t r);
  uint8_t getRotation() const { return rot_; }
  void invertDisplay(bool) {}

  const uint16_t* framebuffer() const { return fb_.data(); }  // landscape 240x135

 protected:
  void putPixel(int32_t x, int32_t y, uint16_t c) override;
  uint16_t getPixel(int32_t x, int32_t y) override;

 private:
  void map(int32_t x, int32_t y, int32_t& px, int32_t& py) const;
  uint8_t rot_ = 1;
  std::vector<uint16_t> fb_;
};
using LGFX_Device = M5GFX;

// Off-screen sprite.
class M5Canvas : public LGFXBase {
 public:
  M5Canvas() = default;
  explicit M5Canvas(LGFXBase* parent) : parent_(parent) {}
  void* createSprite(int32_t w, int32_t h);
  void deleteSprite() { buf_.clear(); w_ = h_ = 0; }
  void fillSprite(uint32_t color) { fillScreen(color); }
  void pushSprite(int32_t x, int32_t y) { pushSprite(parent_, x, y); }
  void pushSprite(int32_t x, int32_t y, uint32_t transparent) { pushSprite(parent_, x, y, transparent); }
  void pushSprite(LGFXBase* dst, int32_t x, int32_t y);
  void pushSprite(LGFXBase* dst, int32_t x, int32_t y, uint32_t transparent);
  void setPsram(bool) {}
  void* getBuffer() { return buf_.data(); }

 protected:
  void putPixel(int32_t x, int32_t y, uint16_t c) override { buf_[y * w_ + x] = c; }
  uint16_t getPixel(int32_t x, int32_t y) override { return buf_[y * w_ + x]; }

 private:
  LGFXBase* parent_ = nullptr;
  std::vector<uint16_t> buf_;
};
using LGFX_Sprite = M5Canvas;
