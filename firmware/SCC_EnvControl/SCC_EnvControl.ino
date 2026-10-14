// =====================================================================
//  SCC Chamber Environmental Control  —  Arduino UNO R3
//
//  Controls temperature and relative humidity inside a small
//  stress-corrosion-cracking (SCC) test chamber:
//    * DHT22 (AM2302) temperature / humidity sensor
//    * USB ultrasonic humidifier module   -> humidity up
//    * 12 V PTC heater (PWM)              -> temperature up
//    * 12 V exhaust fan (fresh air)       -> humidity / temperature down
//    * 12 V circulation fan               -> uniform chamber air
//    * 16x2 I2C LCD keypad shield (5 buttons) for set-points
//    * CSV logging to microSD (with RTC time stamps) and USB serial
//
//  Target tolerance: +/-1.0 degC and +/-2.0 %RH (see config.h)
//
//  Libraries (Arduino Library Manager):
//    "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"
//    "Adafruit RGB LCD Shield Library"
//    SD, Wire, EEPROM (built in)
// =====================================================================
#include <Wire.h>
#include <EEPROM.h>
#include <DHT.h>
#include <Adafruit_RGBLCDShield.h>
#include "config.h"

#ifdef USE_SD
  #include <SPI.h>
  #include <SD.h>
#endif
#ifdef __AVR__
  #include <avr/wdt.h>
#endif

// ---------------------------------------------------------------------
//  Settings stored in EEPROM
// ---------------------------------------------------------------------
#define CFG_MAGIC 0xEC01

struct Settings {
  uint16_t magic;
  float    tSet;
  float    rhSet;
  uint16_t logSec;
  uint8_t  running;
};
Settings cfg;

// ---------------------------------------------------------------------
//  Alarms
// ---------------------------------------------------------------------
#define ALM_SENSOR    0x01   // DHT22 not responding
#define ALM_OVERTEMP  0x02   // above T_ABS_MAX
#define ALM_WATER     0x04   // humidifier tank empty
#define ALM_BAND      0x08   // out of tolerance too long after being stable
uint8_t alarms = 0;
uint8_t alarmsMuted = 0;     // alarms acknowledged by a key press

// ---------------------------------------------------------------------
//  Hardware
// ---------------------------------------------------------------------
DHT dht(PIN_DHT, DHT_TYPE);
Adafruit_RGBLCDShield lcd;

#ifdef USE_RTC
  bool rtcOk = false;
#endif
#ifdef USE_SD
  bool sdOk = false;
#endif

// ---------------------------------------------------------------------
//  Process values and controller state
// ---------------------------------------------------------------------
float tMeas = NAN, rhMeas = NAN;
uint8_t sensorFails = 0;

float tInt = 0, rhInt = 0;     // integral terms (%)
float rhOut = 0;               // humidity PI output, -100..+100 (split range)
float tPrev = NAN;             // for derivative on measurement
float heaterPct = 0, humPct = 0, exhaustPct = 0;
bool  humOn = false, circOn = false;
unsigned long humWindowStart = 0;

// statistics (valid after the chamber became stable)
bool     stable = false;
unsigned long inBandSinceMs = 0;
unsigned long outBandSinceMs = 0;
uint32_t statSamples = 0, statInBand = 0;
float    maxDevT = 0, maxDevRH = 0;

unsigned long lastCtrlMs = 0, lastLogMs = 0, lastLcdMs = 0, lastKeyPollMs = 0;

// ---------------------------------------------------------------------
//  UI state
// ---------------------------------------------------------------------
enum { SCR_MAIN, SCR_OUTPUTS, SCR_STATS, SCR_LOG, SCR_COUNT };
enum { MENU_TSET, MENU_RHSET, MENU_LOG, MENU_RUN, MENU_RESET, MENU_COUNT };

uint8_t screen = SCR_MAIN;
bool    inMenu = false;
uint8_t menuItem = 0;
Settings edit;                 // working copy while in the menu
bool    resetRequested = false;
unsigned long lastKeyMs = 0;
uint8_t backlight = 0xFF;

char serialBuf[40];
uint8_t serialLen = 0;

// =====================================================================
//  Helpers
// =====================================================================
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Fixed-point float formatting / parsing without dtostrf, atof or
// Print::print(float) — keeps the sketch inside the UNO's 32 KB flash.
char *fmtFixed(char *out, float v, int8_t width, uint8_t dec) {
  static const uint16_t scale[] = {1, 10, 100};
  if (dec > 2) dec = 2;
  bool neg = v < 0;
  long n = (long)((neg ? -v : v) * scale[dec] + 0.5f);
  char body[12];
  if (dec) snprintf_P(body, sizeof(body), PSTR("%s%ld.%0*ld"), neg ? "-" : "", n / scale[dec], dec, n % scale[dec]);
  else     snprintf_P(body, sizeof(body), PSTR("%s%ld"), neg ? "-" : "", n);
  snprintf_P(out, 12, PSTR("%*s"), width, body);
  return out;
}

float parseNum(const char *s) {
  float sign = 1, v = 0, div = 0;
  if (*s == '-') { sign = -1; s++; } else if (*s == '+') s++;
  if (!*s) return NAN;
  for (; *s; s++) {
    if (*s == '.' && div == 0) div = 1;
    else if (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); if (div) div *= 10; }
    else return NAN;
  }
  return sign * (div ? v / div : v);
}

// ---------------------------------------------------------------------
//  Minimal RTC driver (DS1307 or PCF8523, both at I2C 0x68, BCD registers)
// ---------------------------------------------------------------------
#ifdef USE_RTC
#define RTC_ADDR 0x68
#if RTC_IS_PCF8523
  #define RTC_TIME_REG 0x03   // seconds .. years
  #define RTC_DAY_IDX  3      // order: s m h day weekday month year
  #define RTC_WD_IDX   4
#else
  #define RTC_TIME_REG 0x00   // DS1307: s m h weekday day month year
  #define RTC_DAY_IDX  4
  #define RTC_WD_IDX   3
#endif

struct Clock { uint8_t yy, mo, dd, hh, mi, ss; };

inline uint8_t bcd2bin(uint8_t v) { return v - 6 * (v >> 4); }
inline uint8_t bin2bcd(uint8_t v) { return v + 6 * (v / 10); }

void rtcWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool rtcBegin() {
  Wire.beginTransmission(RTC_ADDR);
  if (Wire.endTransmission() != 0) return false;
#if RTC_IS_PCF8523
  rtcWriteReg(0x00, 0x00);   // Control_1: clear STOP -> oscillator running
  rtcWriteReg(0x02, 0x00);   // Control_3: battery switch-over enabled
#endif
  return true;
}

bool rtcRead(Clock &c) {
  if (!rtcOk) return false;
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(RTC_TIME_REG);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(RTC_ADDR, 7) != 7) return false;
  uint8_t r[7];
  for (uint8_t i = 0; i < 7; i++) r[i] = Wire.read();
  c.ss = bcd2bin(r[0] & 0x7F);
  c.mi = bcd2bin(r[1] & 0x7F);
  c.hh = bcd2bin(r[2] & 0x3F);
  c.dd = bcd2bin(r[RTC_DAY_IDX] & 0x3F);
  c.mo = bcd2bin(r[5] & 0x1F);
  c.yy = bcd2bin(r[6]);
  return c.mo >= 1 && c.mo <= 12 && c.dd >= 1 && c.dd <= 31;
}

// "YYYY-MM-DD HH:MM:SS"
bool rtcSetFromText(const char *t) {
  if (strlen(t) < 19 || t[4] != '-' || t[7] != '-' || t[13] != ':' || t[16] != ':') return false;
  uint8_t v[6];
  static const uint8_t pos[6] = {2, 5, 8, 11, 14, 17};   // yy mo dd hh mi ss
  for (uint8_t i = 0; i < 6; i++) {
    if (!isdigit(t[pos[i]]) || !isdigit(t[pos[i] + 1])) return false;
    v[i] = (t[pos[i]] - '0') * 10 + (t[pos[i] + 1] - '0');
  }
  if (v[1] < 1 || v[1] > 12 || v[2] < 1 || v[2] > 31 || v[3] > 23 || v[4] > 59 || v[5] > 59) return false;
  uint8_t r[7];
  r[0] = bin2bcd(v[5]);              // seconds (clears DS1307 CH / PCF8523 OS bit)
  r[1] = bin2bcd(v[4]);
  r[2] = bin2bcd(v[3]);              // 24 h mode
  r[RTC_DAY_IDX] = bin2bcd(v[2]);
  r[RTC_WD_IDX]  = 0;                // weekday not used
  r[5] = bin2bcd(v[1]);
  r[6] = bin2bcd(v[0]);
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(RTC_TIME_REG);
  for (uint8_t i = 0; i < 7; i++) Wire.write(r[i]);
  return Wire.endTransmission() == 0;
}
#endif

void loadDefaults() {
  cfg.magic   = CFG_MAGIC;
  cfg.tSet    = DEF_T_SET;
  cfg.rhSet   = DEF_RH_SET;
  cfg.logSec  = DEF_LOG_SEC;
  cfg.running = 1;
}

void saveSettings() { EEPROM.put(0, cfg); }

void loadSettings() {
  EEPROM.get(0, cfg);
  if (cfg.magic != CFG_MAGIC || isnan(cfg.tSet) || isnan(cfg.rhSet)) {
    loadDefaults();
    saveSettings();
  }
}

void resetStats() {
  stable = false;
  inBandSinceMs = 0;
  outBandSinceMs = 0;
  statSamples = statInBand = 0;
  maxDevT = maxDevRH = 0;
  alarms &= ~ALM_BAND;
}

void applySettings(const Settings &s) {
  bool spChanged = (s.tSet != cfg.tSet) || (s.rhSet != cfg.rhSet) || (s.running != cfg.running);
  cfg = s;
  cfg.magic = CFG_MAGIC;
  saveSettings();
  if (spChanged) resetStats();
}

// =====================================================================
//  Outputs
// =====================================================================
void writeLoad(uint8_t pin, bool on) {
  digitalWrite(pin, on ? LOAD_ON : !LOAD_ON);
}

void writeOutputs() {
  uint8_t pwm = (uint8_t)(heaterPct * 2.55f + 0.5f);
  analogWrite(PIN_HEATER, LOAD_ON == HIGH ? pwm : 255 - pwm);
  writeLoad(PIN_HUMIDIFIER, humOn);
  // exhaust fan is speed controlled; below its start-up duty it is switched off
  uint8_t ex = 0;
  if (exhaustPct >= 1.0f)
    ex = (uint8_t)((EXHAUST_MIN_PCT + (100 - EXHAUST_MIN_PCT) * exhaustPct / 100.0f) * 2.55f + 0.5f);
  analogWrite(PIN_EXHAUST, LOAD_ON == HIGH ? ex : 255 - ex);
  writeLoad(PIN_CIRC_FAN, circOn);
}

// Humidifier is a USB module with its own driver board: it cannot be
// PWM'd fast, so it is switched on/off inside a slow window
// (time-proportioning) according to humPct. Pulses shorter than the
// module start-up time are not dropped but carried over to the next
// window, so the average output still equals humPct.
unsigned long humOnMs = 0;
float humCarryMs = 0;

void updateHumidifier(unsigned long now) {
  if (now - humWindowStart >= HUM_WINDOW_MS) {
    humWindowStart = now;
    float want = humPct * (HUM_WINDOW_MS / 100.0f) + humCarryMs;
    if (want < HUM_MIN_ON_MS) {
      humOnMs = 0;
      humCarryMs = want;
    } else {
      humOnMs = want > HUM_WINDOW_MS ? HUM_WINDOW_MS : (unsigned long)want;
      humCarryMs = 0;
    }
    if (humPct <= 0) humCarryMs = 0;
  }
  humOn = (now - humWindowStart) < humOnMs;
}

// =====================================================================
//  Control (runs every CTRL_PERIOD_MS)
// =====================================================================
void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    if (sensorFails < 255) sensorFails++;
    if (sensorFails >= SENSOR_FAIL_MAX) { tMeas = rhMeas = NAN; }
  } else {
    sensorFails = 0;
    tMeas = t + T_OFFSET;
    rhMeas = clampf(h + RH_OFFSET, 0, 100);
  }
}

// PI with anti-windup + optional D on measurement.
// The integral only runs when the error is inside +/-izone (no wind-up during
// large set-point changes) and when it would not push further into saturation.
float piStep(float sp, float pv, float &integ, float kp, float ki, float dTerm,
             float dt, float lo, float hi, float izone) {
  float e = sp - pv;
  float candidate = clampf(integ + ki * e * dt, lo, hi);
  float out = kp * e + candidate + dTerm;
  if (fabs(e) <= izone && !((out > hi && e > 0) || (out < lo && e < 0))) integ = candidate;
  return clampf(kp * e + integ + dTerm, lo, hi);
}

void controlStep() {
  const float dt = CTRL_PERIOD_MS / 1000.0f;
  bool valid = !isnan(tMeas) && !isnan(rhMeas);

  // ---- alarms
  uint8_t a = alarms & ALM_BAND;
  if (sensorFails >= SENSOR_FAIL_MAX) a |= ALM_SENSOR;
  if (valid && tMeas >= T_ABS_MAX) a |= ALM_OVERTEMP;
  if (CHECK_WATER_LEVEL && digitalRead(PIN_WATER_LOW) == HIGH) a |= ALM_WATER;
  if ((a & ~alarms) != 0) {
    Serial.print(F("# ALARM 0x")); Serial.println(a, HEX);
  }
  alarms = a;
  alarmsMuted &= alarms;       // forget acknowledgements of cleared alarms

  if (!cfg.running) {
    heaterPct = humPct = exhaustPct = rhOut = 0;
    circOn = false;
    tInt = rhInt = 0;
    tPrev = tMeas;
    return;
  }

  circOn = true;

  if (!valid) {                // no sensor -> fail safe
    heaterPct = humPct = exhaustPct = 0;
    tPrev = NAN;
    return;
  }

  // ---- temperature loop -> heater
  float dTerm = 0;
  if (!isnan(tPrev)) dTerm = -T_KD * (tMeas - tPrev) / dt;
  tPrev = tMeas;
  heaterPct = piStep(cfg.tSet, tMeas, tInt, T_KP, T_KI, dTerm, dt, 0, 100, T_IZONE);
  if (alarms & ALM_OVERTEMP) { heaterPct = 0; tInt = 0; }

  // ---- humidity: one PI, split range
  //      rhOut > 0 -> humidifier duty, rhOut < 0 -> exhaust fan speed
  float eRH = rhMeas - cfg.rhSet;
  float eT  = tMeas - cfg.tSet;
  rhOut = piStep(cfg.rhSet, rhMeas, rhInt, RH_KP, RH_KI, 0, dt, -100, 100, RH_IZONE);
  humPct     = rhOut > 0 ? rhOut : 0;
  exhaustPct = rhOut < 0 ? -rhOut : 0;
  if (alarms & ALM_WATER) humPct = 0;

  // ---- too hot with the heater already off: vent heat (hysteresis)
  static bool ventHeat = false;
  if (eT >= VENT_T_ON && heaterPct <= 0) ventHeat = true;
  else if (eT <= VENT_T_OFF)             ventHeat = false;
  if (ventHeat && exhaustPct < VENT_T_PCT) exhaustPct = VENT_T_PCT;
  if (alarms & ALM_OVERTEMP) { exhaustPct = 100; humPct = 0; }

  // ---- tolerance statistics
  unsigned long now = millis();
  bool inBand = fabs(eT) <= T_TOL && fabs(eRH) <= RH_TOL;
  if (!stable) {
    if (!inBand) inBandSinceMs = 0;
    else if (inBandSinceMs == 0) inBandSinceMs = now;
    else if (now - inBandSinceMs >= STABLE_SEC * 1000UL) {
      stable = true;
      Serial.println(F("# STABLE: within tolerance"));
    }
  } else {
    statSamples++;
    if (inBand) statInBand++;
    if (fabs(eT)  > maxDevT)  maxDevT  = fabs(eT);
    if (fabs(eRH) > maxDevRH) maxDevRH = fabs(eRH);
    if (inBand) { outBandSinceMs = 0; alarms &= ~ALM_BAND; }
    else if (outBandSinceMs == 0) outBandSinceMs = now;
    else if (now - outBandSinceMs >= OUT_OF_BAND_ALARM_SEC * 1000UL) alarms |= ALM_BAND;
  }
}

// =====================================================================
//  Logging
// =====================================================================
void printHeader(Print &p) {
  p.println(F("timestamp,t_C,rh_pct,t_set_C,rh_set_pct,heater_pct,humid_pct,exhaust_pct,in_band,stable,alarms"));
}

void printTimestamp(Print &p) {
#ifdef USE_RTC
  Clock n;
  if (rtcRead(n)) {
    char b[20];
    snprintf_P(b, sizeof(b), PSTR("20%02u-%02u-%02u %02u:%02u:%02u"), n.yy, n.mo, n.dd, n.hh, n.mi, n.ss);
    p.print(b);
    return;
  }
#endif
  p.print(millis() / 1000UL);
}

void printNum(Print &p, float v, uint8_t dec) {
  char b[12];
  if (isnan(v)) p.print(F("NA")); else p.print(fmtFixed(b, v, 1, dec));
}

void writeRecord(Print &p) {
  bool inBand = !isnan(tMeas) && fabs(tMeas - cfg.tSet) <= T_TOL &&
                fabs(rhMeas - cfg.rhSet) <= RH_TOL;
  printTimestamp(p);            p.print(',');
  printNum(p, tMeas, 1);        p.print(',');
  printNum(p, rhMeas, 1);       p.print(',');
  printNum(p, cfg.tSet, 1);     p.print(',');
  printNum(p, cfg.rhSet, 1);    p.print(',');
  printNum(p, heaterPct, 0);    p.print(',');
  printNum(p, humPct, 0);       p.print(',');
  printNum(p, exhaustPct, 0);   p.print(',');
  p.print(inBand ? 1 : 0);      p.print(',');
  p.print(stable ? 1 : 0);      p.print(',');
  p.println(alarms, HEX);
}

#ifdef USE_SD
void logFileName(char *name) {          // one file per day: YYMMDD.CSV
#ifdef USE_RTC
  Clock n;
  if (rtcRead(n)) {
    snprintf_P(name, 13, PSTR("%02u%02u%02u.CSV"), n.yy, n.mo, n.dd);
    return;
  }
#endif
  strcpy(name, "DATALOG.CSV");
}
#endif

void logRecord() {
  writeRecord(Serial);
#ifdef USE_SD
  if (!sdOk || !cfg.running) return;
  char name[13];
  logFileName(name);
  File f = SD.open(name, FILE_WRITE);
  if (f) {
    if (f.size() == 0) printHeader(f);
    writeRecord(f);
    f.close();
  } else {
    Serial.println(F("# SD write failed"));
  }
#endif
}

// =====================================================================
//  LCD (16x2) and keypad
// =====================================================================
void lcdRow(uint8_t row, const char *s) {
  char b[17];
  snprintf_P(b, sizeof(b), PSTR("%-16s"), s);
  lcd.setCursor(0, row);
  lcd.print(b);
}

// float -> fixed width text ("--.-" when invalid)
char *fmt(char *out, float v, int8_t width, uint8_t dec) {
  if (isnan(v)) { snprintf_P(out, width + 1, PSTR("%*s"), width, "--.-"); return out; }
  return fmtFixed(out, v, width, dec);
}

void setBacklight(uint8_t c) {
  if (c != backlight) { lcd.setBacklight(c); backlight = c; }
}

const char *alarmText() {        // returns a PROGMEM string
  if (alarms & ALM_SENSOR)   return PSTR("!SENSOR FAIL");
  if (alarms & ALM_OVERTEMP) return PSTR("!OVER TEMP");
  if (alarms & ALM_WATER)    return PSTR("!WATER LOW");
  if (alarms & ALM_BAND)     return PSTR("!OUT OF BAND");
  return PSTR("");
}

void drawScreen() {
  char l0[24], l1[24], a[8], b[8];

  if (inMenu) {
    const char *name;
    switch (menuItem) {
      case MENU_TSET:  name = PSTR("Set Temperature"); break;
      case MENU_RHSET: name = PSTR("Set Humidity");    break;
      case MENU_LOG:   name = PSTR("Log interval");    break;
      case MENU_RUN:   name = PSTR("Run / Stop");      break;
      default:         name = PSTR("Reset stats");     break;
    }
    snprintf_P(l0, sizeof(l0), PSTR(">%S"), name);
    switch (menuItem) {
      case MENU_TSET:  snprintf_P(l1, sizeof(l1), PSTR(" %s C"), fmt(a, edit.tSet, 4, 1)); break;
      case MENU_RHSET: snprintf_P(l1, sizeof(l1), PSTR(" %s %%RH"), fmt(a, edit.rhSet, 4, 1)); break;
      case MENU_LOG:   snprintf_P(l1, sizeof(l1), PSTR(" %u s"), edit.logSec); break;
      case MENU_RUN:   snprintf_P(l1, sizeof(l1), PSTR(" %s"), edit.running ? "RUN" : "STOP"); break;
      default:         snprintf_P(l1, sizeof(l1), PSTR(" %s"), resetRequested ? "YES" : "no"); break;
    }
    char row[20];
    snprintf_P(row, sizeof(row), PSTR("%-12.12s<  >"), l1);   // LEFT/RIGHT change value
    lcdRow(0, l0);
    lcdRow(1, row);
    setBacklight(0x6);                  // teal
    return;
  }

  switch (screen) {
    case SCR_MAIN: {
      const char *tag = !cfg.running ? "" : (alarms ? "!!" : (stable ? "OK" : ".."));
      snprintf_P(l0, sizeof(l0), PSTR("T%s/%sC %s"), fmt(a, tMeas, 5, 1), fmt(b, cfg.tSet, 4, 1), tag);
      if (alarms) snprintf_P(l1, sizeof(l1), PSTR("%S"), alarmText());
      else snprintf_P(l1, sizeof(l1), PSTR("H%s/%s%% %s"), fmt(a, rhMeas, 5, 1), fmt(b, cfg.rhSet, 4, 1),
                    cfg.running ? "RUN" : "STP");
      break;
    }
    case SCR_OUTPUTS:
      snprintf_P(l0, sizeof(l0), PSTR("Heat%3d%% Hum%3d%%"), (int)heaterPct, (int)humPct);
      snprintf_P(l1, sizeof(l1), PSTR("Exh%3d%% Circ %s"), (int)exhaustPct, circOn ? "ON" : "OFF");
      break;
    case SCR_STATS:
      if (!stable) {
        unsigned long s = inBandSinceMs ? (millis() - inBandSinceMs) / 1000UL : 0;
        snprintf_P(l0, sizeof(l0), PSTR("Stabilizing..."));
        snprintf_P(l1, sizeof(l1), PSTR("in band %lus/%lu"), s, (unsigned long)STABLE_SEC);
      } else {
        float pct = statSamples ? 100.0f * statInBand / statSamples : 100.0f;
        snprintf_P(l0, sizeof(l0), PSTR("In band %s%%"), fmt(a, pct, 5, 1));
        snprintf_P(l1, sizeof(l1), PSTR("Max dT%s dH%s"), fmt(a, maxDevT, 3, 1), fmt(b, maxDevRH, 3, 1));
      }
      break;
    default: {                          // SCR_LOG
#ifdef USE_SD
      snprintf_P(l0, sizeof(l0), PSTR("Log %us SD %s"), cfg.logSec, sdOk ? "OK" : "--");
#else
      snprintf_P(l0, sizeof(l0), PSTR("Log %us serial"), cfg.logSec);
#endif
#ifdef USE_RTC
      Clock n;
      if (rtcRead(n)) {
        snprintf_P(l1, sizeof(l1), PSTR("%02u/%02u %02u:%02u:%02u"), n.dd, n.mo, n.hh, n.mi, n.ss);
      } else
#endif
      snprintf_P(l1, sizeof(l1), PSTR("No RTC"));
      break;
    }
  }
  lcdRow(0, l0);
  lcdRow(1, l1);

  if (!cfg.running)        setBacklight(0x7);   // white
  else if (alarms)         setBacklight(0x1);   // red
  else if (stable)         setBacklight(0x2);   // green
  else                     setBacklight(0x3);   // yellow
}

void menuAdjust(int8_t dir, uint8_t accel) {
  switch (menuItem) {
    case MENU_TSET:  edit.tSet  = clampf(edit.tSet  + dir * 0.1f * accel, T_SET_MIN, T_SET_MAX); break;
    case MENU_RHSET: edit.rhSet = clampf(edit.rhSet + dir * 0.5f * accel, RH_SET_MIN, RH_SET_MAX); break;
    case MENU_LOG: {
      int v = (int)edit.logSec + dir * (edit.logSec >= 60 ? 10 : 1) * accel;
      edit.logSec = v < 1 ? 1 : (v > 3600 ? 3600 : v);
      break;
    }
    case MENU_RUN:   edit.running = !edit.running; break;
    default:         resetRequested = !resetRequested; break;
  }
}

void handleKeys() {
  static uint8_t prev = 0;
  static unsigned long pressedAt = 0, lastRepeat = 0;
  unsigned long now = millis();
  uint8_t keys = lcd.readButtons();
  uint8_t pressed = keys & ~prev;
  prev = keys;

  if (keys) lastKeyMs = now;
  if (pressed) {
    pressedAt = lastRepeat = now;
    alarmsMuted = alarms;               // any key silences current alarms
  }

  // auto-repeat for LEFT/RIGHT in the menu (accelerates after 2 s)
  uint8_t accel = 1;
  if (!pressed && inMenu && (keys & (BUTTON_LEFT | BUTTON_RIGHT)) &&
      now - pressedAt > 500 && now - lastRepeat > 150) {
    lastRepeat = now;
    pressed = keys & (BUTTON_LEFT | BUTTON_RIGHT);
    accel = (now - pressedAt > 2000) ? 10 : 1;
    if (menuItem == MENU_RUN || menuItem == MENU_RESET) pressed = 0;   // no repeat for toggles
  }
  if (!pressed) {
    if (inMenu && now - lastKeyMs > 30000UL) inMenu = false;          // timeout: discard
    return;
  }

  if (!inMenu) {
    if (pressed & BUTTON_SELECT) {
      inMenu = true; menuItem = 0; edit = cfg; resetRequested = false;
    } else if (pressed & BUTTON_UP) {
      screen = (screen + SCR_COUNT - 1) % SCR_COUNT;
    } else if (pressed & BUTTON_DOWN) {
      screen = (screen + 1) % SCR_COUNT;
    }
  } else {
    if (pressed & BUTTON_SELECT) {      // save and leave
      applySettings(edit);
      if (resetRequested) resetStats();
      inMenu = false;
      Serial.println(F("# settings saved"));
    } else if (pressed & BUTTON_UP) {
      menuItem = (menuItem + MENU_COUNT - 1) % MENU_COUNT;
    } else if (pressed & BUTTON_DOWN) {
      menuItem = (menuItem + 1) % MENU_COUNT;
    } else if (pressed & BUTTON_RIGHT) {
      menuAdjust(+1, accel);
    } else if (pressed & BUTTON_LEFT) {
      menuAdjust(-1, accel);
    }
  }
  lastLcdMs = 0;                        // redraw immediately
}

// =====================================================================
//  Serial commands
// =====================================================================
// STATUS = one CSV record (see HEADER) + run state and tolerance statistics
void printStatus() {
  writeRecord(Serial);
  Serial.print(F("# run="));     Serial.print(cfg.running);
  Serial.print(F(" log_s="));    Serial.print(cfg.logSec);
  Serial.print(F(" stable="));   Serial.print(stable);
  Serial.print(F(" in_band%="));
  printNum(Serial, statSamples ? 100.0f * statInBand / statSamples : 0, 1);
  Serial.print(F(" max_dT="));   printNum(Serial, maxDevT, 2);
  Serial.print(F(" max_dRH="));  printNum(Serial, maxDevRH, 2);
  Serial.println();
}

void printHelp() {
  Serial.println(F("# HELP | STATUS | RUN | STOP | RESET | HEADER"));
  Serial.println(F("# SET T <C> | SET RH <%> | SET LOG <s>"));
  Serial.println(F("# TIME YYYY-MM-DD HH:MM:SS"));
}

void handleCommand(char *line) {
  char *cmd = strtok(line, " ");
  if (!cmd) return;
  Settings s = cfg;
  bool ok = true;

  if      (!strcasecmp(cmd, "HELP"))   printHelp();
  else if (!strcasecmp(cmd, "STATUS")) printStatus();
  else if (!strcasecmp(cmd, "HEADER")) printHeader(Serial);
  else if (!strcasecmp(cmd, "RESET"))  resetStats();
  else if (!strcasecmp(cmd, "RUN"))    { s.running = 1; applySettings(s); }
  else if (!strcasecmp(cmd, "STOP"))   { s.running = 0; applySettings(s); }
  else if (!strcasecmp(cmd, "SET")) {
    char *key = strtok(NULL, " ");
    char *val = strtok(NULL, " ");
    float v = val ? parseNum(val) : NAN;
    if (!key || !val) ok = false;
    else if (!strcasecmp(key, "T")  && v >= T_SET_MIN  && v <= T_SET_MAX)  s.tSet = v;
    else if (!strcasecmp(key, "RH") && v >= RH_SET_MIN && v <= RH_SET_MAX) s.rhSet = v;
    else if (!strcasecmp(key, "LOG") && v >= 1 && v <= 3600)               s.logSec = (uint16_t)v;
    else ok = false;
    if (ok) applySettings(s);
  }
#ifdef USE_RTC
  else if (!strcasecmp(cmd, "TIME")) {
    char *rest = strtok(NULL, "");
    ok = rest && rtcOk && rtcSetFromText(rest);
  }
#endif
  else ok = false;

  Serial.println(ok ? F("# OK") : F("# ERR (type HELP)"));
}

void serialLoop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLen) { serialBuf[serialLen] = 0; handleCommand(serialBuf); serialLen = 0; }
    } else if (serialLen < sizeof(serialBuf) - 1) {
      serialBuf[serialLen++] = c;
    }
  }
}

// =====================================================================
//  setup / loop
// =====================================================================
void setup() {
#ifdef __AVR__
  wdt_disable();
#endif
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_HUMIDIFIER, OUTPUT);
  pinMode(PIN_EXHAUST, OUTPUT);
  pinMode(PIN_CIRC_FAN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_WATER_LOW, INPUT_PULLUP);
  writeOutputs();                       // everything off

  Serial.begin(SERIAL_BAUD);
  Serial.println(F("# SCC Chamber Environmental Control"));

  loadSettings();
  dht.begin();

  lcd.begin(16, 2);
  setBacklight(0x7);
  lcdRow(0, "SCC Env Control");
  lcdRow(1, "starting...");

#ifdef USE_RTC
  rtcOk = rtcBegin();
  if (!rtcOk) Serial.println(F("# RTC not found"));
#endif
#ifdef USE_SD
  pinMode(PIN_SD_CS, OUTPUT);
  sdOk = SD.begin(PIN_SD_CS);
  Serial.println(sdOk ? F("# SD OK") : F("# SD not found"));
#endif

  printHelp();
  printHeader(Serial);
  delay(2000);                          // DHT22 power-up time
  readSensor();
  tPrev = tMeas;
  lastCtrlMs = lastLogMs = humWindowStart = millis();

#ifdef __AVR__
  wdt_enable(WDTO_8S);
#endif
}

void loop() {
#ifdef __AVR__
  wdt_reset();
#endif
  unsigned long now = millis();

  if (now - lastCtrlMs >= CTRL_PERIOD_MS) {
    lastCtrlMs += CTRL_PERIOD_MS;
    readSensor();
    controlStep();
  }

  updateHumidifier(now);
  if (!cfg.running || isnan(tMeas)) humOn = false;
  writeOutputs();

  if (now - lastKeyPollMs >= 50) {
    lastKeyPollMs = now;
    handleKeys();
  }
  serialLoop();

  if (now - lastLogMs >= (unsigned long)cfg.logSec * 1000UL) {
    lastLogMs = now;
    logRecord();
  }

  if (now - lastLcdMs >= 500) {
    lastLcdMs = now;
    drawScreen();
  }

  // buzzer: beep pattern for un-acknowledged alarms
  bool beep = (alarms & ~alarmsMuted) && ((now / 400UL) % 3 == 0);
  digitalWrite(PIN_BUZZER, beep ? HIGH : LOW);
}
