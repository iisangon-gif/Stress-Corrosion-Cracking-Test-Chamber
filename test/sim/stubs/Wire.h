#pragma once
#include "Arduino.h"
// I2C devices present on the simulated bus (address -> present)
extern bool g_i2c[128];
struct TwoWire {
  uint8_t addr = 0;
  void begin() {}
  void beginTransmission(uint8_t a) { addr = a; }
  size_t write(uint8_t) { return 1; }
  uint8_t endTransmission() { return g_i2c[addr & 0x7F] ? 0 : 2; }  // 2 = address NACK
  uint8_t requestFrom(int, int) { return 0; }
  int read() { return -1; }
};
static TwoWire Wire;
