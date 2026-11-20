// CardSim desktop front-end (SDL2), plus a headless mode for scripted tests.
//
//   cardsim_<sketch> [--world FILE] [--data DIR] [--scale N]
//   cardsim_<sketch> --headless --script "wait 3000; type A13\n; wait 9000; shot out.bmp; quit"
//
// Desktop controls
//   Type on your keyboard        Cardputer keyboard (Enter, Backspace = del, Tab)
//   Esc                          Fn + `  (Cardputer Esc)
//   Ctrl + arrow keys            Fn + ; . , /  (Cardputer arrows)
//   Arrow keys                   walk around the map
//   Click the map                walk to that spot
//   F5 (hold)                    G0 button (BtnA)
//   F12                          save a screenshot (BMP)
#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Arduino.h"
#include "cardsim/sim.h"

#include "../core/src/glcdfont.inc"

using namespace cardsim;

// ---------------------------------------------------------------- canvas

struct Canvas {
  int w = 0, h = 0;
  std::vector<uint32_t> px;  // 0xRRGGBB
  void init(int W, int H) {
    w = W;
    h = H;
    px.assign((size_t)W * H, 0);
  }
  void put(int x, int y, uint32_t c) {
    if (x >= 0 && y >= 0 && x < w && y < h) px[(size_t)y * w + x] = c;
  }
  void rect(int x, int y, int rw, int rh, uint32_t c) {
    for (int yy = y; yy < y + rh; yy++)
      for (int xx = x; xx < x + rw; xx++) put(xx, yy, c);
  }
  void frame(int x, int y, int rw, int rh, uint32_t c) {
    rect(x, y, rw, 1, c);
    rect(x, y + rh - 1, rw, 1, c);
    rect(x, y, 1, rh, c);
    rect(x + rw - 1, y, 1, rh, c);
  }
  void disc(int cx, int cy, int r, uint32_t c) {
    for (int dy = -r; dy <= r; dy++)
      for (int dx = -r; dx <= r; dx++)
        if (dx * dx + dy * dy <= r * r) put(cx + dx, cy + dy, c);
  }
  void line(int x0, int y0, int x1, int y1, uint32_t c) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
      put(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
  int text(int x, int y, const std::string& s, uint32_t c, int scale = 1) {
    for (unsigned char ch : s) {
      for (int col = 0; col < 5; col++) {
        uint8_t bits = font[ch * 5 + col];
        for (int row = 0; row < 8; row++)
          if (bits & (1 << row)) rect(x + col * scale, y + row * scale, scale, scale, c);
      }
      x += 6 * scale;
    }
    return x;
  }
};

static uint32_t rgb565_to_888(uint16_t c) {
  uint32_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
  return ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
}

// ---------------------------------------------------------------- layout

static int g_scale = 3;
static const int PAD = 20;
static int SCR_X, SCR_Y, SCR_W, SCR_H, MAP_X, MAP_Y, MAP_W, MAP_H, WIN_W, WIN_H;

static void layout() {
  SCR_X = PAD + 12;
  SCR_Y = PAD + 12;
  SCR_W = 240 * g_scale;
  SCR_H = 135 * g_scale;
  MAP_X = SCR_X + SCR_W + 12 + PAD;
  MAP_Y = PAD;
  MAP_W = 380;
  MAP_H = SCR_H + 24;
  WIN_W = MAP_X + MAP_W + PAD;
  WIN_H = MAP_Y + MAP_H + PAD + 52;
}

// World <-> map pixels
static double g_mx0, g_my0, g_mscale;
static void map_fit() {
  const World& w = world();
  double x0 = w.x, x1 = w.x, y0 = w.y, y1 = w.y;
  for (auto& ap : w.aps) {
    x0 = std::min(x0, ap.x); x1 = std::max(x1, ap.x);
    y0 = std::min(y0, ap.y); y1 = std::max(y1, ap.y);
  }
  x0 -= 25; x1 += 25; y0 -= 25; y1 += 25;
  g_mscale = std::min((MAP_W - 20) / (x1 - x0), (MAP_H - 40) / (y1 - y0));
  g_mx0 = (x0 + x1) / 2;
  g_my0 = (y0 + y1) / 2;
}
static void w2m(double x, double y, int& mx, int& my) {
  mx = MAP_X + MAP_W / 2 + (int)std::lround((x - g_mx0) * g_mscale);
  my = MAP_Y + 10 + (MAP_H - 30) / 2 - (int)std::lround((y - g_my0) * g_mscale);
}
static void m2w(int mx, int my, double& x, double& y) {
  x = g_mx0 + (mx - MAP_X - MAP_W / 2) / g_mscale;
  y = g_my0 - (my - MAP_Y - 10 - (MAP_H - 30) / 2) / g_mscale;
}

static uint32_t rssi_color(double r) {
  if (r >= -60) return 0x1a9641;
  if (r >= -67) return 0xa6d96a;
  if (r >= -72) return 0xffd92f;
  if (r >= -77) return 0xfdae61;
  return 0xd7191c;
}

static void render(Canvas& cv) {
  cv.rect(0, 0, cv.w, cv.h, 0x1b1f24);
  // Device bezel and screen
  cv.rect(PAD, PAD, SCR_W + 24, SCR_H + 24, 0xd9d4c7);
  cv.rect(SCR_X - 4, SCR_Y - 4, SCR_W + 8, SCR_H + 8, 0x111111);
  const uint16_t* fb = framebuffer();
  for (int y = 0; y < 135; y++)
    for (int x = 0; x < 240; x++)
      cv.rect(SCR_X + x * g_scale, SCR_Y + y * g_scale, g_scale, g_scale,
              rgb565_to_888(fb[y * 240 + x]));

  // Map panel
  const World& w = world();
  map_fit();
  cv.rect(MAP_X, MAP_Y, MAP_W, MAP_H, 0x22303a);
  cv.text(MAP_X + 8, MAP_Y + 6, w.name, 0xe6e6e6);
  // 10 m grid
  double gx0, gy0, gx1, gy1;
  m2w(MAP_X, MAP_Y + MAP_H, gx0, gy0);
  m2w(MAP_X + MAP_W, MAP_Y, gx1, gy1);
  for (double gx = std::floor(gx0 / 10) * 10; gx <= gx1; gx += 10) {
    int mx, my;
    w2m(gx, 0, mx, my);
    for (int y = MAP_Y + 20; y < MAP_Y + MAP_H - 4; y += 3) cv.put(mx, y, 0x2f4250);
  }
  for (double gy = std::floor(gy0 / 10) * 10; gy <= gy1; gy += 10) {
    int mx, my;
    w2m(0, gy, mx, my);
    if (my < MAP_Y + 20) continue;
    for (int x = MAP_X + 2; x < MAP_X + MAP_W - 2; x += 3) cv.put(x, my, 0x2f4250);
  }
  // Trail
  for (size_t i = 1; i < w.trail.size(); i++) {
    int ax, ay, bx, by;
    w2m(w.trail[i - 1].first, w.trail[i - 1].second, ax, ay);
    w2m(w.trail[i].first, w.trail[i].second, bx, by);
    cv.line(ax, ay, bx, by, 0x6b7d8a);
  }
  // Access points (one marker per label)
  std::vector<std::string> drawn;
  for (auto& ap : w.aps) {
    if (std::find(drawn.begin(), drawn.end(), ap.name) != drawn.end()) continue;
    drawn.push_back(ap.name);
    int mx, my;
    w2m(ap.x, ap.y, mx, my);
    double r = rssi_at(ap, w.x, w.y, false);
    cv.disc(mx, my, 6, rssi_color(r));
    cv.frame(mx - 7, my - 7, 15, 15, 0xffffff);
    cv.text(mx + 10, my - 4, ap.name, 0xffffff);
  }
  // Target and person
  if (w.has_target) {
    int tx, ty;
    w2m(w.tx, w.ty, tx, ty);
    cv.line(tx - 5, ty - 5, tx + 5, ty + 5, 0xffa500);
    cv.line(tx - 5, ty + 5, tx + 5, ty - 5, 0xffa500);
  }
  int px, py;
  w2m(w.x, w.y, px, py);
  cv.disc(px, py, 5, 0xffffff);
  cv.disc(px, py, 3, 0xff8c00);
  // Scale bar
  int s0x, s0y, s1x, s1y;
  w2m(0, 0, s0x, s0y);
  w2m(20, 0, s1x, s1y);
  int bar = s1x - s0x;
  cv.rect(MAP_X + 10, MAP_Y + MAP_H - 12, bar, 2, 0xe6e6e6);
  cv.text(MAP_X + 14 + bar, MAP_Y + MAP_H - 16, "20 m", 0xe6e6e6);

  // Status lines
  int ty = MAP_Y + MAP_H + 10;
  char buf[200];
  const AccessPoint* best = nullptr;
  double br = -999;
  for (auto& ap : w.aps) {
    double r = rssi_at(ap, w.x, w.y, false);
    if (!ap.ssid.empty() && r > br) { br = r; best = &ap; }
  }
  double lat, lon;
  to_latlon(w.x, w.y, lat, lon);
  snprintf(buf, sizeof(buf), "Pos %.1f, %.1f m  (%.6f, %.6f)   %s   Best: %s %s %.0f dBm", w.x,
           w.y, lat, lon, w.walking ? "walking" : "standing", best ? best->name.c_str() : "-",
           best ? best->ssid.c_str() : "", br);
  cv.text(PAD, ty, buf, 0xe6e6e6);
  cv.text(PAD, ty + 14,
          "Type = Cardputer keys   Arrows / click map = walk   Ctrl+arrows = Fn arrows   "
          "Esc = Fn+`",
          0x9aa7b0);
  snprintf(buf, sizeof(buf), "F5 = G0 button   F12 = screenshot   SD: %s   GPS: %s   IMU: %s",
           w.sd_inserted ? "in" : "out", w.gps ? "on" : "off", w.imu ? "on" : "off");
  cv.text(PAD, ty + 28, buf, 0x9aa7b0);
}

static bool save_bmp(const Canvas& cv, const std::string& path) {
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) return false;
  int rowbytes = (cv.w * 3 + 3) & ~3;
  uint32_t size = 54 + rowbytes * cv.h;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int off, uint32_t v) {
    for (int i = 0; i < 4; i++) hdr[off + i] = (uint8_t)(v >> (8 * i));
  };
  put32(2, size);
  put32(10, 54);
  put32(14, 40);
  put32(18, (uint32_t)cv.w);
  put32(22, (uint32_t)cv.h);
  hdr[26] = 1;
  hdr[28] = 24;
  put32(34, rowbytes * cv.h);
  fwrite(hdr, 1, 54, f);
  std::vector<uint8_t> row(rowbytes, 0);
  for (int y = cv.h - 1; y >= 0; y--) {
    for (int x = 0; x < cv.w; x++) {
      uint32_t c = cv.px[(size_t)y * cv.w + x];
      row[x * 3] = c & 0xFF;
      row[x * 3 + 1] = (c >> 8) & 0xFF;
      row[x * 3 + 2] = (c >> 16) & 0xFF;
    }
    fwrite(row.data(), 1, rowbytes, f);
  }
  fclose(f);
  return true;
}

// ---------------------------------------------------------------- script

struct Cmd {
  std::string op, arg;
};
static std::vector<Cmd> g_script;
static size_t g_sidx = 0;
static uint32_t g_next_at = 0;
static Canvas g_canvas;

static std::string unescape(const std::string& s) {
  std::string o;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      char n = s[++i];
      o += n == 'n' ? '\n' : n == 'b' ? '\b' : n == 't' ? '\t' : n;
    } else {
      o += s[i];
    }
  }
  return o;
}

static void parse_script(const std::string& s) {
  std::string cur;
  auto flush = [&]() {
    size_t a = cur.find_first_not_of(' ');
    if (a != std::string::npos) {
      std::string t = cur.substr(a);
      size_t sp = t.find(' ');
      Cmd c;
      c.op = t.substr(0, sp);
      c.arg = sp == std::string::npos ? "" : t.substr(sp + 1);
      g_script.push_back(c);
    }
    cur.clear();
  };
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == ';') {
      cur += ';';
      i++;
    } else if (s[i] == ';') {
      flush();
    } else {
      cur += s[i];
    }
  }
  flush();
}

static void run_script(uint32_t now) {
  while (g_sidx < g_script.size() && now >= g_next_at) {
    const Cmd& c = g_script[g_sidx++];
    if (c.op == "wait") {
      g_next_at = now + (uint32_t)atol(c.arg.c_str());
    } else if (c.op == "type") {
      key_text(unescape(c.arg));
    } else if (c.op == "key") {
      if (c.arg == "enter") key_special(KEY_SP_ENTER);
      else if (c.arg == "del") key_special(KEY_SP_DEL);
      else if (c.arg == "tab") key_special(KEY_SP_TAB);
    } else if (c.op == "goto") {
      double x = 0, y = 0;
      sscanf(c.arg.c_str(), "%lf %lf", &x, &y);
      walk_to(x, y);
    } else if (c.op == "teleport") {
      double x = 0, y = 0;
      sscanf(c.arg.c_str(), "%lf %lf", &x, &y);
      world().x = x;
      world().y = y;
    } else if (c.op == "waitarrive") {
      if (world().has_target) {
        g_sidx--;  // poll again
        return;
      }
    } else if (c.op == "shot") {
      render(g_canvas);
      if (save_bmp(g_canvas, c.arg)) log("saved %s", c.arg.c_str());
    } else if (c.op == "quit") {
      log("script finished at %u ms", now);
      fflush(stdout);
      std::exit(0);
    } else {
      log("unknown script command '%s'", c.op.c_str());
    }
  }
}

// ---------------------------------------------------------------- SDL

static SDL_Window* g_win;
static SDL_Renderer* g_ren;
static SDL_Texture* g_tex;
static int g_shots = 0;
static bool g_arrows_active = false;

static void sdl_present() {
  render(g_canvas);
  SDL_UpdateTexture(g_tex, nullptr, g_canvas.px.data(), g_canvas.w * 4);
  SDL_RenderClear(g_ren);
  SDL_RenderCopy(g_ren, g_tex, nullptr, nullptr);
  SDL_RenderPresent(g_ren);
}

static void sdl_poll() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        std::exit(0);
      case SDL_TEXTINPUT:
        if (!(SDL_GetModState() & KMOD_CTRL))
          for (const char* p = e.text.text; *p; p++)
            if ((unsigned char)*p < 0x80) key_char(*p);
        break;
      case SDL_KEYDOWN: {
        SDL_Keycode k = e.key.keysym.sym;
        bool ctrl = e.key.keysym.mod & KMOD_CTRL;
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) key_special(KEY_SP_ENTER);
        else if (k == SDLK_BACKSPACE || k == SDLK_DELETE) key_special(KEY_SP_DEL);
        else if (k == SDLK_TAB) key_special(KEY_SP_TAB);
        else if (k == SDLK_ESCAPE) key_char('`', true);
        else if (ctrl && k == SDLK_UP) key_char(';', true);
        else if (ctrl && k == SDLK_DOWN) key_char('.', true);
        else if (ctrl && k == SDLK_LEFT) key_char(',', true);
        else if (ctrl && k == SDLK_RIGHT) key_char('/', true);
        else if (ctrl && k >= 32 && k < 127) key_char((char)k, false, true);
        else if (k == SDLK_F5) set_button(0, true);
        else if (k == SDLK_F12) {
          char name[64];
          snprintf(name, sizeof(name), "cardsim-%03d.bmp", ++g_shots);
          render(g_canvas);
          if (save_bmp(g_canvas, name)) log("saved %s", name);
        }
        break;
      }
      case SDL_KEYUP:
        if (e.key.keysym.sym == SDLK_F5) set_button(0, false);
        break;
      case SDL_MOUSEBUTTONDOWN: {
        // SDL_RenderSetLogicalSize already maps mouse events to canvas pixels.
        int mx = e.button.x, my = e.button.y;
        if (mx >= MAP_X && mx < MAP_X + MAP_W && my >= MAP_Y && my < MAP_Y + MAP_H) {
          double x, y;
          m2w(mx, my, x, y);
          walk_to(x, y);
        }
        break;
      }
    }
  }
  // Arrow keys walk (unless Ctrl turns them into Cardputer arrows)
  const Uint8* ks = SDL_GetKeyboardState(nullptr);
  bool ctrl = SDL_GetModState() & KMOD_CTRL;
  double vx = 0, vy = 0;
  if (!ctrl) {
    vx = (ks[SDL_SCANCODE_RIGHT] ? 1 : 0) - (ks[SDL_SCANCODE_LEFT] ? 1 : 0);
    vy = (ks[SDL_SCANCODE_UP] ? 1 : 0) - (ks[SDL_SCANCODE_DOWN] ? 1 : 0);
  }
  if (vx != 0 || vy != 0) {
    set_walk_dir(vx, vy);
    g_arrows_active = true;
  } else if (g_arrows_active) {
    set_walk_dir(0, 0);
    g_arrows_active = false;
  }
}

// ---------------------------------------------------------------- main

int main(int argc, char** argv) {
  std::string world_file, data = ".", script;
  bool headless = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
    if (a == "--world") world_file = next();
    else if (a == "--data") data = next();
    else if (a == "--headless") headless = true;
    else if (a == "--script") script = next();
    else if (a == "--scale") g_scale = std::max(1, atoi(next().c_str()));
    else if (a == "--help" || a == "-h") {
      printf("usage: %s [--world FILE] [--data DIR] [--scale N] [--headless --script CMDS]\n",
             argv[0]);
      printf("script commands (separated by ';'): wait MS, type TEXT (\\n = Enter),\n"
             "  key enter|del|tab, goto X Y, waitarrive, teleport X Y, shot FILE.bmp, quit\n");
      return 0;
    }
  }

  if (world_file.empty()) {
    load_default_world();
  } else {
    std::string err;
    if (!load_world(world_file, &err)) {
      fprintf(stderr, "world: %s\n", err.c_str());
      return 1;
    }
  }
  set_data_dir(data);
  layout();
  g_canvas.init(WIN_W, WIN_H);

  Host host;
  if (headless) {
    set_realtime(false);
    parse_script(script);
    host.on_tick = run_script;
  } else {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
      return 1;
    }
    g_win = SDL_CreateWindow("CardSim - Cardputer ADV simulator", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_RESIZABLE);
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, 0);
    SDL_RenderSetLogicalSize(g_ren, WIN_W, WIN_H);
    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, WIN_W,
                              WIN_H);
    SDL_StartTextInput();
    host.poll = sdl_poll;
    host.present = sdl_present;
    host.sleep_ms = [](uint32_t ms) { SDL_Delay(ms); };
    if (!script.empty()) {
      parse_script(script);
      host.on_tick = run_script;
    }
  }
  set_host(host);

  setup();
  for (;;) {
    loop();
    service();
  }
}
