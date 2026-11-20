// CardSim: M5Unified and M5Cardputer objects.
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdlib>

#include "M5Cardputer.h"
#include "cardsim/sim.h"

static M5GFX& panel() {
  static M5GFX p;
  return p;
}

namespace cardsim {
const uint16_t* framebuffer() { return panel().framebuffer(); }
}  // namespace cardsim

// ---------------------------------------------------------------- M5Unified

namespace m5 {

M5Unified::M5Unified() : Display(panel()), Lcd(panel()) {}

void M5Unified::begin(const config_t& cfg) {
  if (cfg.clear_display) Display.fillScreen(TFT_BLACK);
}

void M5Unified::update() {
  cardsim::service();
  BtnA.update();
  BtnB.update();
  BtnC.update();
  BtnPWR.update();
}

bool IMU_Class::isEnabled() const { return cardsim::world().imu; }

int IMU_Class::update() {
  if (!isEnabled()) return 0;
  const cardsim::World& w = cardsim::world();
  // Device held in front of you, screen up: gravity on z. Walking adds a
  // vertical bounce, one cycle per step.
  double bounce = w.walking ? 0.30 * std::sin(2.0 * M_PI * w.step_phase) : 0.0;
  data_.accel.x = (float)(0.02 * cardsim::gauss());
  data_.accel.y = (float)(0.02 * cardsim::gauss());
  data_.accel.z = (float)(1.0 + bounce + 0.015 * cardsim::gauss());
  data_.gyro.x = (float)(0.5 * cardsim::gauss());
  data_.gyro.y = (float)(0.5 * cardsim::gauss());
  data_.gyro.z = (float)(0.5 * cardsim::gauss());
  data_.usec = cardsim::now_ms() * 1000u;
  return 1;
}

bool IMU_Class::getAccel(float* x, float* y, float* z) const {
  *x = data_.accel.x;
  *y = data_.accel.y;
  *z = data_.accel.z;
  return isEnabled();
}

bool IMU_Class::getGyro(float* x, float* y, float* z) const {
  *x = data_.gyro.x;
  *y = data_.gyro.y;
  *z = data_.gyro.z;
  return isEnabled();
}

bool Speaker_Class::tone(float, uint32_t, int, bool) { return true; }

void Power_Class::powerOff() {
  cardsim::log("Power off requested; exiting the simulator");
  std::exit(0);
}

void Button_Class::update() {
  bool p = cardsim::button(id_);
  changed_ = (p != pressed_);
  if (changed_) since_ = millis();
  pressed_ = p;
}

}  // namespace m5

m5::M5Unified M5;

// ---------------------------------------------------------------- keyboard

void Keyboard_Class::updateKeyList() {
  changed_ = cardsim::key_step();
  cardsim::KeyEvent e;
  pressed_ = cardsim::key_current(e);
  state_.reset();
  keyList_.clear();
  if (!pressed_) return;

  keyList_.push_back({0, 0});
  state_.fn = e.fn;
  state_.ctrl = e.ctrl;
  state_.alt = e.alt;
  state_.opt = e.opt;
  state_.shift = e.shift;
  switch (e.special) {
    case cardsim::KEY_SP_ENTER: state_.enter = true; break;
    case cardsim::KEY_SP_DEL: state_.del = state_.backspace = true; break;
    case cardsim::KEY_SP_TAB: state_.tab = true; break;
    default:
      if (e.ch) {
        state_.word.push_back(e.ch);
        if (e.ch == ' ') state_.space = true;
      }
  }
  if (state_.ctrl) state_.modifiers |= 0x01;
  if (state_.shift) state_.modifiers |= 0x02;
  if (state_.alt) state_.modifiers |= 0x04;
}

bool Keyboard_Class::isKeyPressed(char c) const {
  if (!pressed_) return false;
  for (char w : state_.word)
    if (w == c) return true;
  return false;
}

// ---------------------------------------------------------------- Cardputer

M5_CARDPUTER::M5_CARDPUTER()
    : Display(panel()), Lcd(panel()), Speaker(M5.Speaker), BtnA(M5.BtnA), Power(M5.Power) {}

void M5_CARDPUTER::begin(bool enableKeyboard) { begin(M5.config(), enableKeyboard); }

void M5_CARDPUTER::begin(const m5::M5Unified::config_t& cfg, bool) {
  M5.begin(cfg);
  Display.setRotation(1);
}

void M5_CARDPUTER::update() {
  M5.update();
  Keyboard.updateKeyList();
  Keyboard.updateKeysState();
}

M5_CARDPUTER M5Cardputer;
