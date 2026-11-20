// CardSim microSD card (a host folder), SPI and I2C objects, Preferences.
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
static int make_dir(const char* p) { return _mkdir(p); }
#else
static int make_dir(const char* p) { return ::mkdir(p, 0755); }
#endif

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

#include "Preferences.h"
#include "SD.h"
#include "SPI.h"
#include "Wire.h"
#include "cardsim/sim.h"

SPIClass SPI;
TwoWire Wire;
TwoWire Wire1;
SDFS SD;

// ---------------------------------------------------------------- helpers

static std::string host_path(const char* p) {
  std::string s = p ? p : "";
  if (s.empty() || s[0] != '/') s = "/" + s;
  return cardsim::sd_root() + s;
}

static bool is_dir(const std::string& hp) {
  struct stat st;
  return stat(hp.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static void mkdirs(const std::string& path) {
  std::string cur;
  std::stringstream ss(path);
  std::string part;
  if (!path.empty() && path[0] == '/') cur = "/";
  while (std::getline(ss, part, '/')) {
    if (part.empty()) continue;
    cur += part;
    make_dir(cur.c_str());
    cur += "/";
  }
}

// ---------------------------------------------------------------- File

namespace fs {

struct FileImpl {
  FILE* f = nullptr;
  std::string sdpath;  // path as the sketch sees it
  std::string name;    // last component
  bool dir = false;
  DIR* d = nullptr;
  ~FileImpl() {
    if (f) fclose(f);
    if (d) closedir(d);
  }
};

size_t File::write(uint8_t c) {
  if (!impl_ || !impl_->f) return 0;
  return fputc(c, impl_->f) == EOF ? 0 : 1;
}

size_t File::write(const uint8_t* buf, size_t n) {
  if (!impl_ || !impl_->f) return 0;
  return fwrite(buf, 1, n, impl_->f);
}

int File::available() {
  if (!impl_ || !impl_->f) return 0;
  long pos = ftell(impl_->f);
  fseek(impl_->f, 0, SEEK_END);
  long end = ftell(impl_->f);
  fseek(impl_->f, pos, SEEK_SET);
  return (int)std::max(0L, end - pos);
}

int File::read() {
  if (!impl_ || !impl_->f) return -1;
  int c = fgetc(impl_->f);
  return c == EOF ? -1 : c;
}

int File::peek() {
  if (!impl_ || !impl_->f) return -1;
  int c = fgetc(impl_->f);
  if (c != EOF) ungetc(c, impl_->f);
  return c == EOF ? -1 : c;
}

size_t File::read(uint8_t* buf, size_t n) {
  if (!impl_ || !impl_->f) return 0;
  return fread(buf, 1, n, impl_->f);
}

void File::flush() {
  if (impl_ && impl_->f) fflush(impl_->f);
}

bool File::seek(uint32_t pos, SeekMode mode) {
  if (!impl_ || !impl_->f) return false;
  int whence = mode == SeekCur ? SEEK_CUR : (mode == SeekEnd ? SEEK_END : SEEK_SET);
  return fseek(impl_->f, (long)pos, whence) == 0;
}

size_t File::position() const {
  if (!impl_ || !impl_->f) return 0;
  return (size_t)ftell(impl_->f);
}

size_t File::size() const {
  if (!impl_) return 0;
  if (impl_->f) {
    long pos = ftell(impl_->f);
    fseek(impl_->f, 0, SEEK_END);
    long end = ftell(impl_->f);
    fseek(impl_->f, pos, SEEK_SET);
    return (size_t)end;
  }
  return 0;
}

void File::close() {
  if (!impl_) return;
  if (impl_->f) {
    fclose(impl_->f);
    impl_->f = nullptr;
  }
  if (impl_->d) {
    closedir(impl_->d);
    impl_->d = nullptr;
  }
  impl_.reset();
}

File::operator bool() const { return impl_ && (impl_->f || impl_->dir); }
const char* File::name() const { return impl_ ? impl_->name.c_str() : ""; }
const char* File::path() const { return impl_ ? impl_->sdpath.c_str() : ""; }
bool File::isDirectory() const { return impl_ && impl_->dir; }

File File::openNextFile(const char* mode) {
  if (!impl_ || !impl_->dir || !impl_->d) return File();
  while (struct dirent* e = readdir(impl_->d)) {
    std::string n = e->d_name;
    if (n == "." || n == "..") continue;
    std::string child = impl_->sdpath;
    if (child.empty() || child.back() != '/') child += "/";
    child += n;
    return SD.open(child.c_str(), mode);
  }
  return File();
}

void File::rewindDirectory() {
  if (impl_ && impl_->d) rewinddir(impl_->d);
}

time_t File::getLastWrite() {
  if (!impl_) return 0;
  struct stat st;
  if (stat(host_path(impl_->sdpath.c_str()).c_str(), &st) != 0) return 0;
  return st.st_mtime;
}

// ---------------------------------------------------------------- FS

File FS::open(const char* path, const char* mode, bool create) {
  if (!mounted() || !path) return File();
  std::string hp = host_path(path);
  auto impl = std::make_shared<FileImpl>();
  impl->sdpath = path;
  std::string sp = path;
  size_t slash = sp.find_last_of('/');
  impl->name = slash == std::string::npos ? sp : sp.substr(slash + 1);

  std::string m = mode ? mode : "r";
  if (m == "r" && is_dir(hp)) {
    impl->dir = true;
    impl->d = opendir(hp.c_str());
    return File(impl);
  }
  if (m != "r" && create) {
    size_t s = hp.find_last_of('/');
    if (s != std::string::npos) mkdirs(hp.substr(0, s));
  }
  // Binary mode, plus read access for append so available()/read() work.
  std::string fm = m == "a" ? "a+b" : (m == "w" ? "w+b" : "rb");
  impl->f = fopen(hp.c_str(), fm.c_str());
  if (!impl->f) return File();
  return File(impl);
}

bool FS::exists(const char* path) {
  if (!mounted()) return false;
  struct stat st;
  return stat(host_path(path).c_str(), &st) == 0;
}

bool FS::remove(const char* path) { return mounted() && ::unlink(host_path(path).c_str()) == 0; }

bool FS::rename(const char* a, const char* b) {
  return mounted() && ::rename(host_path(a).c_str(), host_path(b).c_str()) == 0;
}

bool FS::mkdir(const char* path) {
  if (!mounted()) return false;
  return make_dir(host_path(path).c_str()) == 0 || errno == EEXIST;
}

bool FS::rmdir(const char* path) { return mounted() && ::rmdir(host_path(path).c_str()) == 0; }

}  // namespace fs

bool SDFS::begin(uint8_t, SPIClass&, uint32_t, const char*, uint8_t, bool) {
  mounted_ = cardsim::world().sd_inserted;
  if (mounted_) mkdirs(cardsim::sd_root());
  return mounted_;
}

// ---------------------------------------------------------------- Preferences

static std::map<std::string, std::map<std::string, std::string>>& store() {
  static std::map<std::string, std::map<std::string, std::string>> s;
  return s;
}
static bool g_prefs_loaded = false;

static std::string prefs_file() {
  std::string d = cardsim::data_dir();
  if (d.back() != '/') d += '/';
  return d + "nvs.txt";
}

static void prefs_load() {
  if (g_prefs_loaded) return;
  g_prefs_loaded = true;
  std::ifstream f(prefs_file());
  std::string line;
  while (std::getline(f, line)) {
    // namespace<TAB>key<TAB>value
    size_t a = line.find('\t'), b = line.find('\t', a + 1);
    if (a == std::string::npos || b == std::string::npos) continue;
    store()[line.substr(0, a)][line.substr(a + 1, b - a - 1)] = line.substr(b + 1);
  }
}

bool Preferences::begin(const char* name, bool readOnly, const char*) {
  prefs_load();
  ns_ = name ? name : "";
  ro_ = readOnly;
  open_ = true;
  store()[ns_];
  return true;
}

void Preferences::end() { open_ = false; }

void Preferences::save() {
  std::ofstream f(prefs_file());
  for (auto& ns : store())
    for (auto& kv : ns.second) f << ns.first << '\t' << kv.first << '\t' << kv.second << '\n';
}

bool Preferences::clear() {
  if (!open_ || ro_) return false;
  store()[ns_].clear();
  save();
  return true;
}

bool Preferences::remove(const char* key) {
  if (!open_ || ro_) return false;
  bool r = store()[ns_].erase(key) > 0;
  save();
  return r;
}

bool Preferences::isKey(const char* key) {
  return open_ && store()[ns_].count(key) > 0;
}

size_t Preferences::putStr(const char* k, const std::string& v) {
  if (!open_ || ro_ || !k) return 0;
  store()[ns_][k] = v;
  save();
  return v.size() ? v.size() : 1;
}

double Preferences::getNum(const char* k, double d) {
  if (!open_ || !k) return d;
  auto& m = store()[ns_];
  auto it = m.find(k);
  if (it == m.end()) return d;
  return atof(it->second.c_str());
}

String Preferences::getString(const char* k, const String& d) {
  if (!open_ || !k) return d;
  auto& m = store()[ns_];
  auto it = m.find(k);
  return it == m.end() ? d : String(it->second.c_str());
}

size_t Preferences::getString(const char* k, char* buf, size_t len) {
  String s = getString(k);
  s.toCharArray(buf, (unsigned)len);
  return s.length();
}
