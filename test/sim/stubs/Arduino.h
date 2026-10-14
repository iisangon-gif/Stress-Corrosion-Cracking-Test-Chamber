// Minimal host-side stand-in for the Arduino core, enough to compile and run
// SCC_EnvControl.ino on a PC against a simulated chamber.
#pragma once
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <strings.h>
using std::isnan;

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HEX 16
#define F(s) (s)
#define PSTR(s) (s)

extern unsigned long g_millis;   // simulated clock (ms)
extern int g_pins[64];           // digital output levels
extern int g_pwm[64];            // analogWrite values

inline unsigned long millis() { return g_millis; }
inline void delay(unsigned long ms) { g_millis += ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int p, int v) { g_pins[p] = v; }
inline int digitalRead(int p) { return g_pins[p]; }
inline void analogWrite(int p, int v) { g_pwm[p] = v; }

// avr-libc's "%S" (string in flash) is a plain string on the host
inline int snprintf_P(char *b, size_t n, const char *f, ...) {
  std::string ff(f);
  for (size_t i = 0; (i = ff.find("%S", i)) != std::string::npos;) ff[i + 1] = 's';
  va_list ap;
  va_start(ap, f);
  int r = vsnprintf(b, n, ff.c_str(), ap);
  va_end(ap);
  return r;
}

struct Print {
  std::string out;
  virtual ~Print() {}
  void w(const std::string &s) { out += s; }
  void print(const char *s) { w(s); }
  void print(char c) { w(std::string(1, c)); }
  void print(unsigned long v, int base = 10) {
    char b[32];
    snprintf(b, sizeof(b), base == 16 ? "%lX" : "%lu", v);
    w(b);
  }
  void print(long v) { w(std::to_string(v)); }
  void print(int v, int base = 10) { print((unsigned long)v, base); }
  void print(unsigned v, int base = 10) { print((unsigned long)v, base); }
  void print(uint8_t v, int base = 10) { print((unsigned long)v, base); }
  void print(uint16_t v) { print((unsigned long)v); }
  template <class T> void println(T v) { print(v); w("\n"); }
  template <class T> void println(T v, int b) { print(v, b); w("\n"); }
  void println() { w("\n"); }
};

struct SerialT : Print {
  std::string in;
  void begin(long) {}
  int available() { return (int)in.size(); }
  char read() { char c = in[0]; in.erase(0, 1); return c; }
};
extern SerialT Serial;
