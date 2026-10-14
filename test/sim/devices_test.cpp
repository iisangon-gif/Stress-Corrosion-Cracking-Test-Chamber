// Starts the controller with modules missing, then fits them one by one
// (as when the chamber is built up step by step) and checks that it keeps
// running, reports what it finds and picks the modules up without a reset.
#include "stubs/firmware.h"

unsigned long g_millis = 0;
int g_pins[64], g_pwm[64];
SerialT Serial;
EEPROMClass EEPROM;
float g_tSens = 30.0, g_rhSens = 60.0;
bool g_dhtFail = false;
uint8_t g_buttons = 0;
char g_lcd[2][17];
bool g_i2c[128] = {false};
bool g_sdCard = false;
int g_sdBegins = 0;
std::string g_sdData;

static int failures = 0;

static void step(long ms) {
  for (long i = 0; i < ms / 10; i++) { g_millis += 10; loop(); }
}

static void expect(bool cond, const char *what) {
  printf("%-4s %s\n", cond ? "ok" : "FAIL", what);
  if (!cond) failures++;
}

static bool said(const char *text) { return Serial.out.find(text) != std::string::npos; }

int main() {
  memset(g_lcd, ' ', sizeof(g_lcd));

  // ---- power-on with no LCD, no SD card (and no RTC in the host build)
  setup();
  expect(said("# LCD not found") && said("SD not found") && said("DHT22 OK"),
         "power-on report lists what is missing");
  expect(!lcdOk && !sdOk, "runs without LCD and SD card");

  Serial.in = "SET T 35\nSET RH 70\nSET LOG 5\n";
  int heatOn = 0, mistOnCount = 0;
  for (int i = 0; i < 3000; i++) {              // 30 s
    step(10);
    heatOn += g_pins[PIN_HEATER] == HIGH;
    mistOnCount += g_pins[PIN_MIST] == HIGH;
  }
  expect(heatOn > 0 && mistOnCount > 0, "control works without LCD / SD (heater and mist switch)");
  expect(said("\n") && Serial.out.find(",35.0,70.0,") != std::string::npos, "records still go to USB serial");

  // ---- fit the SD card: found at the next device check, no reset needed
  int beginsBefore = g_sdBegins;
  g_sdCard = true;
  step(DEVICE_RETRY_MS + 1000);
  expect(sdOk && g_sdBegins > beginsBefore && said("# SD card found"), "SD card fitted later is found");
  step(6000);
  expect(g_sdData.find("timestamp,") == 0 && g_sdData.find(",35.0,70.0,") != std::string::npos,
         "records written to the SD card with header");

  // ---- fit the LCD shield: found and drawn
  g_i2c[0x20] = true;
  step(DEVICE_RETRY_MS + 1000);
  expect(lcdOk && said("# LCD found"), "LCD fitted later is found");
  expect(strncmp(g_lcd[0], "T 30.0/35.0C", 12) == 0, "LCD shows the main screen");

  // ---- pull the SD card: write fails, controller keeps running, retries
  g_sdCard = false;
  step(6000);
  expect(!sdOk && said("SD write failed"), "removed SD card is detected on the next write");
  g_sdCard = true;
  step(DEVICE_RETRY_MS + 1000);
  expect(sdOk, "re-inserted SD card is found again");

  // ---- DEVICES command
  Serial.out.clear();
  Serial.in = "DEVICES\n";
  step(100);
  expect(said("# LCD OK") && said("SD OK") && said("DHT22 OK"), "DEVICES command reports all fitted");

  // ---- reset-statistics menu item (restored on the Mega)
  g_buttons = BUTTON_SELECT; step(100); g_buttons = 0; step(600);
  for (int i = 0; i < 4; i++) { g_buttons = BUTTON_DOWN; step(100); g_buttons = 0; step(600); }
  expect(strncmp(g_lcd[0], ">Reset stats", 12) == 0, "menu has Reset stats");

  printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
  return failures ? 1 : 0;
}
