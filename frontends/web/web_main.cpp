// CardSim browser front-end (Emscripten + Asyncify). The page (shell.html)
// draws the screen and the map, and sends keys and walking through these
// exported functions.
#include <emscripten.h>

#include <cstdio>
#include <string>

#include "Arduino.h"
#include "cardsim/sim.h"

using namespace cardsim;

extern "C" {

EMSCRIPTEN_KEEPALIVE const uint16_t* sim_fb() { return framebuffer(); }

EMSCRIPTEN_KEEPALIVE void sim_key_char(int c, int fn, int ctrl, int alt) {
  key_char((char)c, fn != 0, ctrl != 0, alt != 0);
}
EMSCRIPTEN_KEEPALIVE void sim_key_special(int k) { key_special(k); }
EMSCRIPTEN_KEEPALIVE void sim_button(int id, int pressed) { set_button(id, pressed != 0); }

EMSCRIPTEN_KEEPALIVE void sim_walk_to(double x, double y) { walk_to(x, y); }
EMSCRIPTEN_KEEPALIVE void sim_walk_dir(double vx, double vy) { set_walk_dir(vx, vy); }

EMSCRIPTEN_KEEPALIVE double sim_x() { return world().x; }
EMSCRIPTEN_KEEPALIVE double sim_y() { return world().y; }
EMSCRIPTEN_KEEPALIVE int sim_walking() { return world().walking ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE int sim_has_target() { return world().has_target ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE double sim_tx() { return world().tx; }
EMSCRIPTEN_KEEPALIVE double sim_ty() { return world().ty; }
EMSCRIPTEN_KEEPALIVE int sim_trail_len() { return (int)world().trail.size(); }
EMSCRIPTEN_KEEPALIVE double sim_trail_x(int i) { return world().trail[i].first; }
EMSCRIPTEN_KEEPALIVE double sim_trail_y(int i) { return world().trail[i].second; }
EMSCRIPTEN_KEEPALIVE int sim_ap_count() { return (int)world().aps.size(); }
EMSCRIPTEN_KEEPALIVE double sim_ap_rssi(int i) {
  return rssi_at(world().aps[i], world().x, world().y, false);
}
EMSCRIPTEN_KEEPALIVE uint32_t sim_millis() { return now_ms(); }

#ifndef CARDSIM_SKETCH_NAME
#define CARDSIM_SKETCH_NAME "sketch"
#endif
EMSCRIPTEN_KEEPALIVE const char* sim_sketch_name() { return CARDSIM_SKETCH_NAME; }

// World description for the page, as JSON.
EMSCRIPTEN_KEEPALIVE const char* sim_world_json() {
  static std::string s;
  const World& w = world();
  auto esc = [](const std::string& v) {
    std::string o;
    for (char c : v) {
      if (c == '"' || c == '\\') o += '\\';
      o += c;
    }
    return o;
  };
  char buf[256];
  s = "{\"name\":\"" + esc(w.name) + "\",";
  snprintf(buf, sizeof(buf),
           "\"lat0\":%.7f,\"lon0\":%.7f,\"gps\":%s,\"imu\":%s,\"sd\":%s,\"aps\":[", w.lat0,
           w.lon0, w.gps ? "true" : "false", w.imu ? "true" : "false",
           w.sd_inserted ? "true" : "false");
  s += buf;
  for (size_t i = 0; i < w.aps.size(); i++) {
    const AccessPoint& a = w.aps[i];
    snprintf(buf, sizeof(buf),
             "%s{\"name\":\"%s\",\"ssid\":\"%s\",\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
             "\"x\":%.2f,\"y\":%.2f,\"ch\":%d}",
             i ? "," : "", esc(a.name).c_str(), esc(a.ssid).c_str(), a.bssid[0], a.bssid[1],
             a.bssid[2], a.bssid[3], a.bssid[4], a.bssid[5], a.x, a.y, a.channel);
    s += buf;
  }
  s += "]}";
  return s.c_str();
}

}  // extern "C"

int main() {
  // The page may have written /world.txt before starting us.
  std::string err;
  FILE* f = fopen("/world.txt", "r");
  if (f) {
    fclose(f);
    if (!load_world("/world.txt", &err)) {
      log("world.txt: %s (using the sample world)", err.c_str());
      load_default_world();
    }
  } else {
    load_default_world();
  }
  set_data_dir("/");
  set_realtime(true);

  Host host;
  host.sleep_ms = [](uint32_t ms) { emscripten_sleep(ms); };
  set_host(host);

  EM_ASM({ if (Module.onSimReady) Module.onSimReady(); });
  setup();
  for (;;) {
    loop();
    service();
  }
}
