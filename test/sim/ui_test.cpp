// Drives the keypad / LCD / serial interface of SCC_EnvControl on the host
// and checks the expected screens, settings and fail-safe behaviour.
#include "stubs/firmware.h"

unsigned long g_millis = 0;
int g_pins[64], g_pwm[64];
SerialT Serial;
EEPROMClass EEPROM;
float g_tSens = 34.9, g_rhSens = 69.5;
bool g_dhtFail = false;
uint8_t g_buttons = 0;
char g_lcd[2][17];

static int failures = 0;

static void step(long ms) {
  for (long i = 0; i < ms / 10; i++) { g_millis += 10; loop(); }
}

static void press(uint8_t b, long hold = 100) {
  g_buttons = b; step(hold);
  g_buttons = 0; step(600);
}

static void expectRow(int row, const char *want, const char *what) {
  char got[17];
  memcpy(got, g_lcd[row], 16);
  got[16] = 0;
  bool ok = strncmp(got, want, strlen(want)) == 0;
  printf("%-4s %-16s [%s]\n", ok ? "ok" : "FAIL", what, got);
  if (!ok) failures++;
}

static void expect(bool cond, const char *what) {
  printf("%-4s %s\n", cond ? "ok" : "FAIL", what);
  if (!cond) failures++;
}

int main() {
  memset(g_lcd, ' ', sizeof(g_lcd));
  setup();
  step(3000);
  expectRow(0, "T 34.9/35.0C", "main T");
  expectRow(1, "H 69.5/70.0% RUN", "main RH");

  press(BUTTON_DOWN);  expectRow(0, "Heat", "outputs screen");
  press(BUTTON_DOWN);  expectRow(0, "Stabilizing", "stats screen");
  press(BUTTON_DOWN);  expectRow(0, "Log 60s", "log screen");
  press(BUTTON_DOWN);  expectRow(0, "T 34.9", "wraps to main");

  press(BUTTON_SELECT);           expectRow(0, ">Set Temperature", "menu opens");
  press(BUTTON_RIGHT);
  press(BUTTON_RIGHT);            expectRow(1, " 35.2 C", "T +0.2");
  press(BUTTON_DOWN);
  press(BUTTON_LEFT);             expectRow(1, " 69.5 %RH", "RH -0.5");
  press(BUTTON_SELECT);
  expect(fabs(cfg.tSet - 35.2f) < 0.01f && fabs(cfg.rhSet - 69.5f) < 0.01f, "SELECT saves set-points");

  press(BUTTON_SELECT);
  press(BUTTON_RIGHT);
  step(31000);                    // menu timeout discards the edit
  expect(fabs(cfg.tSet - 35.2f) < 0.01f && !inMenu, "menu timeout discards changes");

  Serial.in = "SET T 36.5\nSET RH 75\nSET T 99\n";
  step(100);
  expect(fabs(cfg.tSet - 36.5f) < 0.01f && fabs(cfg.rhSet - 75.0f) < 0.01f, "serial SET");
  expect(Serial.out.find("# ERR") != std::string::npos, "out-of-range SET rejected");

  // ROOM mode: below 20.0 C the temperature set-point becomes ROOM (no heating)
  Serial.in = "SET T 20\n";
  step(100);
  press(BUTTON_SELECT);
  press(BUTTON_LEFT);             expectRow(1, " ROOM C", "LEFT below 20 C -> ROOM");
  press(BUTTON_RIGHT);            expectRow(1, " 20.0 C", "RIGHT from ROOM -> 20 C");
  press(BUTTON_LEFT);
  press(BUTTON_SELECT);
  step(4000);
  expectRow(0, "T 34.9/ROOMC", "main shows ROOM");
  expect(heaterPct == 0 && g_pins[PIN_HEATER] == LOW, "ROOM mode: heater off");
  Serial.in = "SET T 36.5\nSET T ROOM\n";
  step(100);
  expect(roomMode(cfg.tSet), "serial SET T ROOM");

  g_dhtFail = true;
  step(10000);
  expectRow(1, "!SENSOR FAIL", "sensor failure shown");
  {
    int beeps = 0, ledToggles = 0, prevLed = g_pins[PIN_LED_ALARM];
    for (int i = 0; i < 300; i++) {               // 3 s
      step(10);
      beeps += g_pins[PIN_BUZZER] == HIGH;
      ledToggles += g_pins[PIN_LED_ALARM] != prevLed;
      prevLed = g_pins[PIN_LED_ALARM];
    }
    expect(beeps > 0 && ledToggles >= 4, "new alarm: buzzer beeps, red LED blinks");
    press(BUTTON_UP);                              // acknowledge
    beeps = 0; ledToggles = 0; prevLed = g_pins[PIN_LED_ALARM];
    for (int i = 0; i < 300; i++) {
      step(10);
      beeps += g_pins[PIN_BUZZER] == HIGH;
      ledToggles += g_pins[PIN_LED_ALARM] != prevLed;
      prevLed = g_pins[PIN_LED_ALARM];
    }
    expect(beeps == 0 && ledToggles == 0 && g_pins[PIN_LED_ALARM] == HIGH,
           "acknowledged: buzzer silent, red LED steady");
  }
  expect(heaterPct == 0 && mistPct == 0 && g_pins[PIN_HEATER] == LOW &&
         g_pins[PIN_MIST] == LOW, "fail-safe: heater and mist heads off");
  g_dhtFail = false;
  step(5000);
  expect(!(alarms & ALM_SENSOR), "alarm clears when sensor recovers");
  expect(g_pins[PIN_LED_ALARM] == LOW && g_pins[PIN_BUZZER] == LOW, "red LED and buzzer off when alarm clears");

  Serial.in = "STOP\n";
  step(3000);
  expect(g_pins[PIN_HEATER] == LOW && g_pins[PIN_MIST] == LOW && g_pins[PIN_CIRC_FAN] == LOW, "STOP switches outputs off");
  expect(g_pins[PIN_LED_RUN] == LOW && g_pins[PIN_LED_HEAT] == LOW && g_pins[PIN_LED_MIST] == LOW,
         "STOP: green, heat and mist LEDs off");

  printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
  return failures ? 1 : 0;
}
