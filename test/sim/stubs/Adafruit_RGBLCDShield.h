#pragma once
#include "Arduino.h"
#define BUTTON_UP     0x08
#define BUTTON_DOWN   0x04
#define BUTTON_LEFT   0x10
#define BUTTON_RIGHT  0x02
#define BUTTON_SELECT 0x01
extern uint8_t g_buttons;
extern char g_lcd[2][17];
struct Adafruit_RGBLCDShield {
  int r = 0, c = 0;
  void begin(int, int) {}
  void setBacklight(uint8_t) {}
  void setCursor(int col, int row) { c = col; r = row; }
  void print(const char *s) { for (; *s && c < 16; s++) g_lcd[r][c++] = *s; }
  uint8_t readButtons() { return g_buttons; }
  void clear() { memset(g_lcd, ' ', sizeof(g_lcd)); }
};
