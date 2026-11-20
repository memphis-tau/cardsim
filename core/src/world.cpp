// CardSim world: access points, the person walking around, and the GPS
// module that reports where they are.
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

#include "HardwareSerial.h"
#include "cardsim/sim.h"

namespace cardsim {

static World g_world;
World& world() { return g_world; }

// The same layout as the repo's examples/sample_aps_simulated.csv, so the
// analysis script's aps.example.csv lines up with logs from the simulator.
static const char* kDefaultWorld = R"(# CardSim world file
# Positions are metres east (x) and north (y) of the origin.
name, Sample Campground
origin, 39.5000000, -98.3500000
start, -20, -20

# ap, label, ssid, bssid, x, y, channel [, rssi_at_1m_dbm, path_loss_n]
ap, AP1, Guest, A4:2B:B0:00:01:10, 0, 0, 1
ap, AP1, Staff, A4:2B:B0:00:01:11, 0, 0, 1
ap, AP2, Guest, A4:2B:B0:00:02:10, -60, -50.35, 6
ap, AP2, Staff, A4:2B:B0:00:02:11, -60, -50.35, 6
ap, AP3, Guest, A4:2B:B0:00:03:10, 55, -45.3, 11, -34, 3.0
ap, AP3, Staff, A4:2B:B0:00:03:11, 55, -45.3, 11, -34, 3.0
ap, Store, "Camp Store", 02:00:00:00:0A:20, 25, 70, 11, -36, 2.9
ap, Neighbor, "Neighbor ""5G""", 11:22:33:44:55:66, 150, 60, 1, -28, 2.6
ap, Hidden, "", 02:00:00:00:0A:01, -120, 40, 6

noise_db, 3
floor_dbm, -92
gps, on
gps_fix_after_s, 8
imu, on
sd, on
)";

// ---------------------------------------------------------------- parsing

static std::vector<std::string> split_csv(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool inq = false, was_quoted = false;
  for (size_t i = 0; i < line.size(); i++) {
    char c = line[i];
    if (inq) {
      if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
        cur += '"';
        i++;
      } else if (c == '"') {
        inq = false;
      } else {
        cur += c;
      }
    } else if (c == '"') {
      inq = true;
      was_quoted = true;
      cur.clear();  // drop leading spaces before the quote
    } else if (c == ',') {
      out.push_back(cur);
      cur.clear();
      was_quoted = false;
    } else if (!(was_quoted)) {
      cur += c;
    }
  }
  out.push_back(cur);
  for (auto& s : out) {
    size_t a = s.find_first_not_of(" \t\r");
    size_t b = s.find_last_not_of(" \t\r");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
  }
  return out;
}

static bool parse_bssid(const std::string& s, uint8_t out[6]) {
  unsigned v[6];
  if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6)
    return false;
  for (int i = 0; i < 6; i++) out[i] = (uint8_t)v[i];
  return true;
}

static bool on(const std::string& s) { return s == "on" || s == "1" || s == "yes" || s == "true"; }

static bool parse_world(std::istream& in, World& w, std::string* err) {
  w = World();
  std::string line;
  int ln = 0;
  while (std::getline(in, line)) {
    ln++;
    size_t first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') continue;
    auto f = split_csv(line);
    const std::string& k = f[0];
    auto need = [&](size_t n) {
      if (f.size() < n) {
        if (err) *err = "line " + std::to_string(ln) + ": '" + k + "' needs " +
                        std::to_string(n - 1) + " values";
        return false;
      }
      return true;
    };
    if (k == "name") {
      if (!need(2)) return false;
      w.name = f[1];
    } else if (k == "origin") {
      if (!need(3)) return false;
      w.lat0 = atof(f[1].c_str());
      w.lon0 = atof(f[2].c_str());
    } else if (k == "start") {
      if (!need(3)) return false;
      w.x = atof(f[1].c_str());
      w.y = atof(f[2].c_str());
    } else if (k == "ap") {
      if (!need(7)) return false;
      AccessPoint ap;
      ap.name = f[1];
      ap.ssid = f[2];
      if (!parse_bssid(f[3], ap.bssid)) {
        if (err) *err = "line " + std::to_string(ln) + ": bad BSSID '" + f[3] + "'";
        return false;
      }
      ap.x = atof(f[4].c_str());
      ap.y = atof(f[5].c_str());
      ap.channel = atoi(f[6].c_str());
      if (f.size() > 7 && !f[7].empty()) ap.ref_dbm = atof(f[7].c_str());
      if (f.size() > 8 && !f[8].empty()) ap.n = atof(f[8].c_str());
      if (f.size() > 9 && !f[9].empty()) ap.auth = atoi(f[9].c_str());
      w.aps.push_back(ap);
    } else if (k == "noise_db") {
      if (!need(2)) return false;
      w.noise_db = atof(f[1].c_str());
    } else if (k == "floor_dbm") {
      if (!need(2)) return false;
      w.floor_dbm = atof(f[1].c_str());
    } else if (k == "gps") {
      if (!need(2)) return false;
      w.gps = on(f[1]);
    } else if (k == "gps_baud") {
      if (!need(2)) return false;
      w.gps_baud = (uint32_t)atol(f[1].c_str());
    } else if (k == "gps_fix_after_s") {
      if (!need(2)) return false;
      w.gps_fix_after_ms = (uint32_t)(atof(f[1].c_str()) * 1000);
    } else if (k == "imu") {
      if (!need(2)) return false;
      w.imu = on(f[1]);
    } else if (k == "sd") {
      if (!need(2)) return false;
      w.sd_inserted = on(f[1]);
    } else if (k == "walk_speed") {
      if (!need(2)) return false;
      w.walk_speed = atof(f[1].c_str());
    } else {
      if (err) *err = "line " + std::to_string(ln) + ": unknown setting '" + k + "'";
      return false;
    }
  }
  w.trail.clear();
  w.trail.push_back({w.x, w.y});
  return true;
}

bool load_world(const std::string& path, std::string* err) {
  std::ifstream f(path);
  if (!f) {
    if (err) *err = "cannot open " + path;
    return false;
  }
  World w;
  if (!parse_world(f, w, err)) return false;
  g_world = w;
  return true;
}

void load_default_world() {
  std::istringstream in(kDefaultWorld);
  std::string err;
  if (!parse_world(in, g_world, &err)) log("default world: %s", err.c_str());
}

// ---------------------------------------------------------------- radio

double rssi_at(const AccessPoint& ap, double x, double y, bool noisy) {
  double d = std::hypot(x - ap.x, y - ap.y);
  if (d < 1.0) d = 1.0;
  double r = ap.ref_dbm - 10.0 * ap.n * std::log10(d);
  if (noisy) r += g_world.noise_db * gauss();
  return r;
}

void to_latlon(double x, double y, double& lat, double& lon) {
  const double m_per_deg = 111320.0;
  lat = g_world.lat0 + y / m_per_deg;
  lon = g_world.lon0 + x / (m_per_deg * std::cos(g_world.lat0 * M_PI / 180.0));
}

// ---------------------------------------------------------------- walking

void walk_to(double x, double y) {
  g_world.has_target = true;
  g_world.tx = x;
  g_world.ty = y;
}
void set_walk_dir(double vx, double vy) {
  g_world.vx = vx;
  g_world.vy = vy;
  if (vx != 0 || vy != 0) g_world.has_target = false;
}
void stop_walking() {
  g_world.has_target = false;
  g_world.vx = g_world.vy = 0;
}

// ---------------------------------------------------------------- GPS

static uint32_t g_last_tick = 0;
static uint32_t g_next_nmea = 0;
static double g_jx = 0, g_jy = 0;  // GPS position error (random walk)
static time_t g_utc0 = 0;

static void nmea_send(HardwareSerial* port, const char* body) {
  uint8_t cs = 0;
  for (const char* p = body; *p; p++) cs ^= (uint8_t)*p;
  char line[200];
  snprintf(line, sizeof(line), "$%s*%02X\r\n", body, cs);
  port->injectRx(line);
}

static void nmea_coord(double v, bool is_lat, char* out, size_t n, char& hemi) {
  hemi = is_lat ? (v >= 0 ? 'N' : 'S') : (v >= 0 ? 'E' : 'W');
  v = std::fabs(v);
  int deg = (int)v;
  double minutes = (v - deg) * 60.0;
  snprintf(out, n, is_lat ? "%02d%08.5f" : "%03d%08.5f", deg, minutes);
}

static void emit_gps(uint32_t now) {
  World& w = g_world;
  if (!g_utc0) g_utc0 = time(nullptr);
  time_t t = g_utc0 + now / 1000;
  struct tm tmv;
#ifdef _WIN32
  gmtime_s(&tmv, &t);
#else
  gmtime_r(&t, &tmv);
#endif
  char hhmmss[16], ddmmyy[16];
  snprintf(hhmmss, sizeof(hhmmss), "%02d%02d%02d.00", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  snprintf(ddmmyy, sizeof(ddmmyy), "%02d%02d%02d", tmv.tm_mday, tmv.tm_mon + 1,
           tmv.tm_year % 100);

  bool fix = now >= w.gps_fix_after_ms;
  char gga[160], rmc[160];
  if (fix) {
    g_jx = 0.85 * g_jx + 0.6 * gauss();
    g_jy = 0.85 * g_jy + 0.6 * gauss();
    double lat, lon;
    to_latlon(w.x + g_jx, w.y + g_jy, lat, lon);
    char la[24], lo[24], ns, ew;
    nmea_coord(lat, true, la, sizeof(la), ns);
    nmea_coord(lon, false, lo, sizeof(lo), ew);
    double knots = w.walking ? w.walk_speed * 1.943844 : 0.0;
    snprintf(gga, sizeof(gga), "GPGGA,%s,%s,%c,%s,%c,1,09,0.9,58.0,M,-33.0,M,,", hhmmss, la,
             ns, lo, ew);
    snprintf(rmc, sizeof(rmc), "GPRMC,%s,A,%s,%c,%s,%c,%.2f,0.00,%s,,,A", hhmmss, la, ns, lo,
             ew, knots, ddmmyy);
  } else {
    snprintf(gga, sizeof(gga), "GPGGA,%s,,,,,0,03,99.99,,,,,,", hhmmss);
    snprintf(rmc, sizeof(rmc), "GPRMC,%s,V,,,,,,,%s,,,N", hhmmss, ddmmyy);
  }
  for (HardwareSerial* port : allSerialPorts()) {
    if (port->uartNum() != 0 && port->isOpenAt(w.gps_baud)) {
      nmea_send(port, gga);
      nmea_send(port, rmc);
    }
  }
}

// ---------------------------------------------------------------- tick

void world_tick(uint32_t now) {
  World& w = g_world;
  uint32_t dt_ms = now - g_last_tick;
  if (dt_ms > 250) dt_ms = 250;  // after a stall, don't teleport
  g_last_tick = now;
  double dt = dt_ms / 1000.0;

  double mx = 0, my = 0;
  if (w.has_target) {
    double dx = w.tx - w.x, dy = w.ty - w.y;
    double d = std::hypot(dx, dy);
    if (d < 0.2) {
      w.has_target = false;
    } else {
      double step = std::min(d, w.walk_speed * dt);
      mx = dx / d * step;
      my = dy / d * step;
    }
  } else if (w.vx != 0 || w.vy != 0) {
    double len = std::hypot(w.vx, w.vy);
    mx = w.vx / len * w.walk_speed * dt;
    my = w.vy / len * w.walk_speed * dt;
  }
  // Walking is the intent to move, so it holds between ticks of zero length.
  w.walking = w.has_target || w.vx != 0 || w.vy != 0;
  if (mx != 0 || my != 0) {
    w.x += mx;
    w.y += my;
    // One step per cycle of the accelerometer bounce.
    w.step_phase += std::hypot(mx, my) / w.stride_m;
    auto& last = w.trail.back();
    if (std::hypot(w.x - last.first, w.y - last.second) > 1.0) {
      w.trail.push_back({w.x, w.y});
      if (w.trail.size() > 4000) w.trail.erase(w.trail.begin());
    }
  }

  if (w.gps) {
    if (g_next_nmea == 0 || now >= g_next_nmea) {
      bool any_open = false;
      for (HardwareSerial* p : allSerialPorts())
        if (p->uartNum() != 0 && p->isOpenAt(w.gps_baud)) any_open = true;
      if (any_open) {
        emit_gps(now);
        g_next_nmea = now + 1000;
      } else {
        g_next_nmea = 0;  // emit as soon as a port opens
      }
    }
  }
}

}  // namespace cardsim
