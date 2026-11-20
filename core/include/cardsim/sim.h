// CardSim internals shared by the core and the front-ends (desktop, browser,
// headless). Sketches never include this.
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cardsim {

// ---------------------------------------------------------------- clock
// Real time when a person is driving it; virtual (fast) time when headless.
void set_realtime(bool realtime);
uint32_t now_ms();
void advance(uint32_t ms);  // delay(): advances the world, pumps the host
void service();             // M5.update(): pumps the host without waiting

// ---------------------------------------------------------------- host
struct Host {
  std::function<void()> poll;              // read input events
  std::function<void()> present;           // show the frame (desktop only)
  std::function<void(uint32_t)> sleep_ms;  // wait in real time (and yield)
  std::function<void(uint32_t)> on_tick;   // after the world updates (scripts)
};
void set_host(const Host& h);
bool quit_requested();
void request_quit();

// ---------------------------------------------------------------- keyboard
// Each queued key is pressed for a short moment, then released, the way
// a fast tap on the real keyboard looks to a sketch.
enum SpecialKey { KEY_SP_ENTER = 1, KEY_SP_DEL, KEY_SP_TAB };
void key_char(char c, bool fn = false, bool ctrl = false, bool alt = false, bool opt = false);
void key_special(int which);
void key_text(const std::string& text);  // '\n' = ENTER, '\b' = DEL
void set_button(int id, bool pressed);   // BtnA = G0
bool button(int id);

struct KeyEvent {
  char ch = 0;  // printable, 0 if special
  int special = 0;
  bool fn = false, ctrl = false, alt = false, opt = false, shift = false;
};
bool key_current(KeyEvent& out);  // true while a key is held
bool key_step();                  // M5Cardputer.update(): true if the state changed

// ---------------------------------------------------------------- world
struct AccessPoint {
  std::string name;   // AP label (for the map)
  std::string ssid;   // empty = hidden network
  uint8_t bssid[6];
  double x = 0, y = 0;     // metres east / north of the origin
  double ref_dbm = -32;    // RSSI at 1 m
  double n = 2.7;          // path-loss exponent
  int channel = 1;
  int auth = 3;            // WIFI_AUTH_WPA2_PSK
};

struct World {
  std::string name = "Campground";
  double lat0 = 39.5, lon0 = -98.35;  // origin
  std::vector<AccessPoint> aps;
  double noise_db = 3.0;
  double floor_dbm = -92;  // weaker networks are not seen
  bool sd_inserted = true;
  bool imu = true;          // Cardputer ADV has a BMI270
  bool gps = true;          // Cap LoRa-1262 attached
  uint32_t gps_baud = 115200;
  uint32_t gps_fix_after_ms = 8000;
  double walk_speed = 1.3;  // m/s
  double stride_m = 0.75;

  // Live state
  double x = 0, y = 0;
  bool has_target = false;
  double tx = 0, ty = 0;
  double vx = 0, vy = 0;  // arrow keys, in units of walk_speed
  bool walking = false;
  double step_phase = 0;
  std::vector<std::pair<double, double>> trail;
};

World& world();
bool load_world(const std::string& path, std::string* error = nullptr);
void load_default_world();
void walk_to(double x, double y);
void set_walk_dir(double vx, double vy);
void stop_walking();
double rssi_at(const AccessPoint& ap, double x, double y, bool noisy);
void to_latlon(double x, double y, double& lat, double& lon);

// ---------------------------------------------------------------- files
void set_data_dir(const std::string& dir);  // contains sdcard/ and prefs
std::string data_dir();
std::string sd_root();

// ---------------------------------------------------------------- frame
const uint16_t* framebuffer();  // 240 x 135 RGB565, landscape
uint32_t frame_counter();
void note_draw();

// ---------------------------------------------------------------- misc
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
double gauss();  // standard normal, for noise

}  // namespace cardsim
