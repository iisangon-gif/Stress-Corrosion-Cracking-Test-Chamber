#pragma once
#include "Arduino.h"
// Simulated microSD card: g_sdCard = card fitted; data written goes to g_sdData
extern bool g_sdCard;
extern int g_sdBegins;
extern std::string g_sdData;
#define FILE_WRITE 1
struct File : Print {
  bool ok = false;
  explicit operator bool() const { return ok; }
  unsigned long size() const { return g_sdData.size(); }
  void close() { g_sdData += out; out.clear(); }
};
struct SDClass {
  bool begin(int) { g_sdBegins++; return g_sdCard; }
  void end() {}
  File open(const char *, int) { File f; f.ok = g_sdCard; return f; }
};
static SDClass SD;
