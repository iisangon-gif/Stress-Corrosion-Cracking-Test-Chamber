#pragma once
#include "Arduino.h"
#define DHT22 22
extern float g_tSens, g_rhSens;  // value seen by the sensor
extern bool g_dhtFail;
struct DHT {
  DHT(int, int) {}
  void begin() {}
  // DHT22 resolution is 0.1
  float readTemperature() { return g_dhtFail ? NAN : std::round(g_tSens * 10) / 10; }
  float readHumidity()    { return g_dhtFail ? NAN : std::round(g_rhSens * 10) / 10; }
};
