#pragma once
#include "Arduino.h"
struct EEPROMClass {
  uint8_t mem[1024];
  EEPROMClass() { memset(mem, 0xFF, sizeof(mem)); }
  template <class T> void put(int a, const T &t) { memcpy(mem + a, &t, sizeof(T)); }
  template <class T> void get(int a, T &t) { memcpy(&t, mem + a, sizeof(T)); }
};
extern EEPROMClass EEPROM;
