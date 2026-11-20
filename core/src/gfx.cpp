// CardSim graphics: software renderer for the M5GFX subset.
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "M5GFX.h"
#include "cardsim/sim.h"

#include "glcdfont.inc"

namespace fonts {
const IFont Font0{1}, Font2{2}, Font4{3}, Font6{5}, Font7{6}, Font8{9}, AsciiFont8x16{2}, AsciiFont24x48{6}, TomThumb{1}, DejaVu9{1}, DejaVu12{2}, DejaVu18{2}, DejaVu24{3}, DejaVu40{5}, DejaVu56{7}, DejaVu72{9}, Orbitron_Light_24{3}, Orbitron_Light_32{4}, Roboto_Thin_24{3}, Satisfy_24{3}, Yellowtail_32{4}, efontCN_12{2}, efontCN_16{2}, efontJA_12{2}, efontJA_16{2}, efontKR_12{2}, efontKR_16{2}, efontTW_12{2}, efontTW_16{2}, lgfxJapanGothic_12{2}, lgfxJapanGothic_16{2}, lgfxJapanMincho_12{2}, lgfxJapanMincho_16{2}, FreeMono9pt7b{2}, FreeMono12pt7b{2}, FreeMono18pt7b{3}, FreeMono24pt7b{4}, FreeMonoBold9pt7b{2}, FreeMonoBold12pt7b{2}, FreeMonoBold18pt7b{3}, FreeMonoBold24pt7b{4}, FreeMonoOblique9pt7b{2}, FreeMonoOblique12pt7b{2}, FreeMonoOblique18pt7b{3}, FreeMonoOblique24pt7b{4}, FreeMonoBoldOblique9pt7b{2}, FreeMonoBoldOblique12pt7b{2}, FreeMonoBoldOblique18pt7b{3}, FreeMonoBoldOblique24pt7b{4}, FreeSans9pt7b{2}, FreeSans12pt7b{2}, FreeSans18pt7b{3}, FreeSans24pt7b{4}, FreeSansBold9pt7b{2}, FreeSansBold12pt7b{2}, FreeSansBold18pt7b{3}, FreeSansBold24pt7b{4}, FreeSansOblique9pt7b{2}, FreeSansOblique12pt7b{2}, FreeSansOblique18pt7b{3}, FreeSansOblique24pt7b{4}, FreeSansBoldOblique9pt7b{2}, FreeSansBoldOblique12pt7b{2}, FreeSansBoldOblique18pt7b{3}, FreeSansBoldOblique24pt7b{4}, FreeSerif9pt7b{2}, FreeSerif12pt7b{2}, FreeSerif18pt7b{3}, FreeSerif24pt7b{4}, FreeSerifBold9pt7b{2}, FreeSerifBold12pt7b{2}, FreeSerifBold18pt7b{3}, FreeSerifBold24pt7b{4}, FreeSerifItalic9pt7b{2}, FreeSerifItalic12pt7b{2}, FreeSerifItalic18pt7b{3}, FreeSerifItalic24pt7b{4}, FreeSerifBoldItalic9pt7b{2}, FreeSerifBoldItalic12pt7b{2}, FreeSerifBoldItalic18pt7b{3}, FreeSerifBoldItalic24pt7b{4};
}  // namespace fonts

// ---------------------------------------------------------------- shapes

void LGFXBase::hspan(int32_t x0, int32_t x1, int32_t y, uint16_t c) {
  if (y < 0 || y >= h_) return;
  if (x0 > x1) std::swap(x0, x1);
  x0 = std::max<int32_t>(x0, 0);
  x1 = std::min<int32_t>(x1, w_ - 1);
  for (int32_t x = x0; x <= x1; x++) putPixel(x, y, c);
}

void LGFXBase::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
  if (w < 0) { x += w; w = -w; }
  if (h < 0) { y += h; h = -h; }
  int32_t x1 = std::min(x + w, w_), y1 = std::min(y + h, h_);
  x = std::max<int32_t>(x, 0);
  y = std::max<int32_t>(y, 0);
  for (int32_t yy = y; yy < y1; yy++)
    for (int32_t xx = x; xx < x1; xx++) putPixel(xx, yy, (uint16_t)color);
  cardsim::note_draw();
}

void LGFXBase::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) {
  if (w <= 0 || h <= 0) return;
  drawFastHLine(x, y, w, c);
  drawFastHLine(x, y + h - 1, w, c);
  drawFastVLine(x, y, h, c);
  drawFastVLine(x + w - 1, y, h, c);
}

void LGFXBase::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
  int32_t dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int32_t dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;
  for (;;) {
    plot(x0, y0, (uint16_t)color);
    if (x0 == x1 && y0 == y1) break;
    int32_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  cardsim::note_draw();
}

void LGFXBase::drawCircle(int32_t xc, int32_t yc, int32_t r, uint32_t color) {
  int32_t x = r, y = 0, err = 1 - r;
  uint16_t c = (uint16_t)color;
  while (x >= y) {
    plot(xc + x, yc + y, c); plot(xc + y, yc + x, c);
    plot(xc - y, yc + x, c); plot(xc - x, yc + y, c);
    plot(xc - x, yc - y, c); plot(xc - y, yc - x, c);
    plot(xc + y, yc - x, c); plot(xc + x, yc - y, c);
    y++;
    if (err < 0) err += 2 * y + 1;
    else { x--; err += 2 * (y - x) + 1; }
  }
  cardsim::note_draw();
}

void LGFXBase::fillCircle(int32_t xc, int32_t yc, int32_t r, uint32_t color) {
  for (int32_t dy = -r; dy <= r; dy++) {
    int32_t dx = (int32_t)std::sqrt((double)r * r - (double)dy * dy);
    hspan(xc - dx, xc + dx, yc + dy, (uint16_t)color);
  }
  cardsim::note_draw();
}

void LGFXBase::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r,
                             uint32_t c) {
  r = std::min(r, std::min(w, h) / 2);
  drawFastHLine(x + r, y, w - 2 * r, c);
  drawFastHLine(x + r, y + h - 1, w - 2 * r, c);
  drawFastVLine(x, y + r, h - 2 * r, c);
  drawFastVLine(x + w - 1, y + r, h - 2 * r, c);
  // corners
  int32_t cx[4] = {x + r, x + w - 1 - r, x + r, x + w - 1 - r};
  int32_t cy[4] = {y + r, y + r, y + h - 1 - r, y + h - 1 - r};
  int32_t px = r, py = 0, err = 1 - r;
  while (px >= py) {
    for (int k = 0; k < 4; k++) {
      int sxs = (k & 1) ? 1 : -1, sys = (k & 2) ? 1 : -1;
      plot(cx[k] + sxs * px, cy[k] + sys * py, (uint16_t)c);
      plot(cx[k] + sxs * py, cy[k] + sys * px, (uint16_t)c);
    }
    py++;
    if (err < 0) err += 2 * py + 1;
    else { px--; err += 2 * (py - px) + 1; }
  }
}

void LGFXBase::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r,
                             uint32_t c) {
  r = std::min(r, std::min(w, h) / 2);
  for (int32_t yy = 0; yy < h; yy++) {
    int32_t inset = 0;
    int32_t dy = -1;
    if (yy < r) dy = r - yy;
    else if (yy >= h - r) dy = yy - (h - 1 - r);
    if (dy >= 0) inset = r - (int32_t)std::sqrt((double)r * r - (double)dy * dy);
    hspan(x + inset, x + w - 1 - inset, y + yy, (uint16_t)c);
  }
  cardsim::note_draw();
}

void LGFXBase::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2,
                            int32_t y2, uint32_t c) {
  drawLine(x0, y0, x1, y1, c);
  drawLine(x1, y1, x2, y2, c);
  drawLine(x2, y2, x0, y0, c);
}

void LGFXBase::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2,
                            int32_t y2, uint32_t c) {
  if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
  if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
  if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
  auto edge = [](int32_t ya, int32_t xa, int32_t yb, int32_t xb, int32_t y) {
    if (yb == ya) return (double)xa;
    return xa + (double)(xb - xa) * (y - ya) / (yb - ya);
  };
  for (int32_t y = y0; y <= y2; y++) {
    double xa = edge(y0, x0, y2, x2, y);
    double xb = y < y1 ? edge(y0, x0, y1, x1, y) : edge(y1, x1, y2, x2, y);
    hspan((int32_t)std::lround(xa), (int32_t)std::lround(xb), y, (uint16_t)c);
  }
  cardsim::note_draw();
}

void LGFXBase::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
  for (int32_t yy = 0; yy < h; yy++)
    for (int32_t xx = 0; xx < w; xx++) plot(x + xx, y + yy, data[yy * w + xx]);
  cardsim::note_draw();
}

uint16_t LGFXBase::readPixel(int32_t x, int32_t y) {
  if (x < 0 || y < 0 || x >= w_ || y >= h_) return 0;
  return getPixel(x, y);
}

// ---------------------------------------------------------------- text

void LGFXBase::setTextSize(float sx, float sy) {
  tsx_ = sx > 0 ? sx : 1;
  tsy_ = sy > 0 ? sy : 1;
}

void LGFXBase::setTextFont(int n) {
  switch (n) {
    case 2: fontScale_ = 2; break;
    case 4: fontScale_ = 3; break;
    case 6: fontScale_ = 5; break;
    case 7: fontScale_ = 6; break;
    case 8: fontScale_ = 9; break;
    default: fontScale_ = 1; break;
  }
}

void LGFXBase::glyph(int32_t x, int32_t y, uint8_t c, bool paintBg) {
  float sx = tsx_ * fontScale_, sy = tsy_ * fontScale_;
  for (int col = 0; col < 6; col++) {
    uint8_t bits = col < 5 ? font[c * 5 + col] : 0;
    for (int row = 0; row < 8; row++) {
      bool on = bits & (1 << row);
      if (!on && !paintBg) continue;
      uint16_t color = on ? fg_ : bg_;
      int32_t px0 = x + (int32_t)(col * sx), px1 = x + (int32_t)((col + 1) * sx);
      int32_t py0 = y + (int32_t)(row * sy), py1 = y + (int32_t)((row + 1) * sy);
      for (int32_t py = py0; py < py1; py++)
        for (int32_t px = px0; px < px1; px++) plot(px, py, color);
    }
  }
}

// Vertical scroll only (what text scrolling needs). dy < 0 moves content up.
void LGFXBase::scroll(int32_t, int32_t dy) {
  if (dy >= 0 || -dy >= h_) {
    if (dy != 0) fillScreen(base_);
    return;
  }
  int32_t up = -dy;
  for (int32_t y = 0; y < h_ - up; y++)
    for (int32_t x = 0; x < w_; x++) putPixel(x, y, getPixel(x, y + up));
  fillRect(0, h_ - up, w_, up, base_);
}

void LGFXBase::newline() {
  cx_ = 0;
  cy_ += charH();
  if (scroll_ && cy_ + charH() > h_) {
    int32_t over = cy_ + charH() - h_;
    scroll(0, -over);
    cy_ -= over;
  }
}

size_t LGFXBase::write(uint8_t c) {
  if (c == '\r') return 1;
  if (c == '\n') {
    newline();
    return 1;
  }
  if (wrapX_ && cx_ + charW() > w_) newline();
  if (wrapY_ && cy_ >= h_) cy_ = 0;
  glyph(cx_, cy_, c, bg_ != fg_);
  cx_ += charW();
  cardsim::note_draw();
  return 1;
}

int32_t LGFXBase::textWidth(const char* s) const { return (int32_t)strlen(s) * charW(); }

int32_t LGFXBase::drawString(const char* s, int32_t x, int32_t y) {
  int32_t tw = textWidth(s), th = charH();
  uint8_t dx = datum_ & 3, dy = datum_ & 12;
  if (dx == 1) x -= tw / 2;
  else if (dx == 2) x -= tw;
  if (dy == 4) y -= th / 2;
  else if (dy == 8) y -= th;
  if (datum_ & 16) y -= th - (int32_t)(tsy_ * fontScale_);  // baseline
  for (const char* p = s; *p; p++) {
    glyph(x, y, (uint8_t)*p, bg_ != fg_);
    x += charW();
  }
  cardsim::note_draw();
  return tw;
}

int32_t LGFXBase::drawCentreString(const char* s, int32_t x, int32_t y, uint8_t) {
  uint8_t d = datum_;
  datum_ = top_center;
  int32_t r = drawString(s, x, y);
  datum_ = d;
  return r;
}

int32_t LGFXBase::drawRightString(const char* s, int32_t x, int32_t y, uint8_t) {
  uint8_t d = datum_;
  datum_ = top_right;
  int32_t r = drawString(s, x, y);
  datum_ = d;
  return r;
}

int32_t LGFXBase::drawChar(uint16_t c, int32_t x, int32_t y) {
  glyph(x, y, (uint8_t)c, bg_ != fg_);
  return charW();
}

// ---------------------------------------------------------------- panel

M5GFX::M5GFX() : fb_(PANEL_W * PANEL_H, 0) {
  w_ = PANEL_W;
  h_ = PANEL_H;
}

void M5GFX::setRotation(uint8_t r) {
  rot_ = r & 3;
  if (rot_ & 1) {
    w_ = PANEL_W;
    h_ = PANEL_H;
  } else {
    w_ = PANEL_H;
    h_ = PANEL_W;
  }
}

void M5GFX::map(int32_t x, int32_t y, int32_t& px, int32_t& py) const {
  switch (rot_) {
    case 1: px = x; py = y; break;
    case 3: px = PANEL_W - 1 - x; py = PANEL_H - 1 - y; break;
    case 0: px = y; py = PANEL_H - 1 - x; break;
    default: px = PANEL_W - 1 - y; py = x; break;
  }
}

void M5GFX::putPixel(int32_t x, int32_t y, uint16_t c) {
  int32_t px, py;
  map(x, y, px, py);
  fb_[py * PANEL_W + px] = c;
}

uint16_t M5GFX::getPixel(int32_t x, int32_t y) {
  int32_t px, py;
  map(x, y, px, py);
  return fb_[py * PANEL_W + px];
}

// ---------------------------------------------------------------- sprite

void* M5Canvas::createSprite(int32_t w, int32_t h) {
  if (w <= 0 || h <= 0) return nullptr;
  w_ = w;
  h_ = h;
  buf_.assign((size_t)w * h, 0);
  return buf_.data();
}

void M5Canvas::pushSprite(LGFXBase* dst, int32_t x, int32_t y) {
  if (!dst) return;
  for (int32_t yy = 0; yy < h_; yy++)
    for (int32_t xx = 0; xx < w_; xx++) dst->drawPixel(x + xx, y + yy, buf_[yy * w_ + xx]);
}

void M5Canvas::pushSprite(LGFXBase* dst, int32_t x, int32_t y, uint32_t transparent) {
  if (!dst) return;
  for (int32_t yy = 0; yy < h_; yy++)
    for (int32_t xx = 0; xx < w_; xx++) {
      uint16_t c = buf_[yy * w_ + xx];
      if (c != (uint16_t)transparent) dst->drawPixel(x + xx, y + yy, c);
    }
}
