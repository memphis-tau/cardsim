// CardSim core: clock, host pump, keyboard queue, logging.
#include "cardsim/sim.h"

#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <random>

namespace cardsim {

void world_tick(uint32_t now);  // world.cpp

// ---------------------------------------------------------------- clock

static bool g_realtime = true;
static double g_virtual_ms = 0;
static auto g_start = std::chrono::steady_clock::now();
static Host g_host;
static bool g_quit = false;
static uint32_t g_last_present = 0;
static uint32_t g_last_yield = 0;

void set_realtime(bool rt) { g_realtime = rt; }

uint32_t now_ms() {
  if (g_realtime) {
    auto d = std::chrono::steady_clock::now() - g_start;
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  }
  // A busy-wait on millis() must still see time pass.
  g_virtual_ms += 0.01;
  return (uint32_t)g_virtual_ms;
}

void set_host(const Host& h) { g_host = h; }
bool quit_requested() { return g_quit; }
void request_quit() { g_quit = true; }

static void tick() {
  uint32_t now = now_ms();
  world_tick(now);
  if (g_host.on_tick) g_host.on_tick(now);
}

static void present_if_due(uint32_t now) {
  if (g_host.present && now - g_last_present >= 16) {
    g_last_present = now;
    g_host.present();
  }
}

void advance(uint32_t ms) {
  if (!g_realtime) {
    // Virtual time: step the world in small slices so sensors stay smooth.
    while (ms > 0 && !g_quit) {
      uint32_t slice = ms > 5 ? 5 : ms;
      g_virtual_ms += slice;
      ms -= slice;
      tick();
    }
    return;
  }
  uint32_t target = now_ms() + ms;
  do {
    if (g_host.poll) g_host.poll();
    tick();
    uint32_t now = now_ms();
    present_if_due(now);
    if (now >= target) break;
    uint32_t wait = target - now;
    if (wait > 8) wait = 8;
    if (g_host.sleep_ms) g_host.sleep_ms(wait);
    g_last_yield = now_ms();
  } while (!g_quit);
}

void service() {
  if (g_realtime) {
    if (g_host.poll) g_host.poll();
    tick();
    uint32_t now = now_ms();
    present_if_due(now);
    // A sketch that never calls delay() must still let the browser breathe.
    if (now - g_last_yield >= 16 && g_host.sleep_ms) {
      g_host.sleep_ms(0);
      g_last_yield = now_ms();
    }
  } else {
    tick();
  }
}

// ---------------------------------------------------------------- keyboard

static std::deque<KeyEvent> g_keys;
static bool g_held = false;
static KeyEvent g_cur;
static uint32_t g_press_at = 0, g_release_at = 0;
static bool g_buttons[4] = {false, false, false, false};

static bool is_shifted(char c) {
  return isupper((unsigned char)c) || (c && strchr("!@#$%^&*()_+{}|:\"<>?~", c));
}

void key_char(char c, bool fn, bool ctrl, bool alt, bool opt) {
  KeyEvent e;
  e.ch = c;
  e.fn = fn;
  e.ctrl = ctrl;
  e.alt = alt;
  e.opt = opt;
  e.shift = is_shifted(c);
  g_keys.push_back(e);
}

void key_special(int which) {
  KeyEvent e;
  e.special = which;
  g_keys.push_back(e);
}

void key_text(const std::string& text) {
  for (char c : text) {
    if (c == '\n' || c == '\r')
      key_special(KEY_SP_ENTER);
    else if (c == '\b')
      key_special(KEY_SP_DEL);
    else if (c == '\t')
      key_special(KEY_SP_TAB);
    else
      key_char(c);
  }
}

bool key_current(KeyEvent& out) {
  if (!g_held) return false;
  out = g_cur;
  return true;
}

bool key_step() {
  uint32_t now = now_ms();
  if (g_held) {
    if (now - g_press_at >= 40) {
      g_held = false;
      g_release_at = now;
      return true;
    }
    return false;
  }
  if (!g_keys.empty() && now - g_release_at >= 15) {
    g_cur = g_keys.front();
    g_keys.pop_front();
    g_held = true;
    g_press_at = now;
    return true;
  }
  return false;
}

void set_button(int id, bool pressed) {
  if (id >= 0 && id < 4) g_buttons[id] = pressed;
}
bool button(int id) { return id >= 0 && id < 4 && g_buttons[id]; }

// ---------------------------------------------------------------- files

static std::string g_data_dir =
#ifdef __EMSCRIPTEN__
    "/";
#else
    ".";
#endif

void set_data_dir(const std::string& d) { g_data_dir = d.empty() ? "." : d; }
std::string data_dir() { return g_data_dir; }
std::string sd_root() {
  std::string d = g_data_dir;
  if (d.back() != '/') d += '/';
  return d + "sdcard";
}

// ---------------------------------------------------------------- misc

static uint32_t g_frames = 0;
uint32_t frame_counter() { return g_frames; }
void note_draw() { g_frames++; }

void log(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[cardsim] ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

static std::mt19937& rng() {
  static std::mt19937 r(12345);
  return r;
}
double gauss() {
  static std::normal_distribution<double> nd(0.0, 1.0);
  return nd(rng());
}
uint32_t rand32() { return rng()(); }
void seed(uint32_t s) { rng().seed(s); }

}  // namespace cardsim
