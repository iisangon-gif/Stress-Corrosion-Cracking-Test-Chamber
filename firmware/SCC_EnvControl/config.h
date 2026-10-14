// =====================================================================
//  config.h — hardware pins, options and tuning for SCC_EnvControl
//  Board: Arduino UNO R3
// =====================================================================
#pragma once

// ---------------------------------------------------------------------
//  Optional modules (comment out to disable)
// ---------------------------------------------------------------------
#define USE_SD            // microSD logging (Data Logging Shield, CS = D10)
#define USE_RTC           // real-time clock on the Data Logging Shield

// RTC chip on the logging shield:
//   Adafruit Data Logging Shield rev B -> PCF8523  (set 1)
//   older Adafruit rev A / most clones -> DS1307   (set 0)
#define RTC_IS_PCF8523    1

// ---------------------------------------------------------------------
//  Pins (UNO R3)
//  A4/A5 (SDA/SCL) : LCD keypad shield + RTC (I2C bus)
//  D10..D13        : SD card (SPI) on the Data Logging Shield
// ---------------------------------------------------------------------
#define PIN_DHT           2   // DHT22 data (10k pull-up to 5V)
#define PIN_HUMIDIFIER    3   // MOSFET module -> USB ultrasonic humidifier (5V)
#define PIN_WATER_LOW     4   // optional float switch in humidifier tank (to GND)
#define PIN_HEATER        5   // MOSFET module -> 12V PTC heater (PWM)
#define PIN_EXHAUST       6   // MOSFET module -> 12V exhaust/fresh-air fan (PWM)
#define PIN_CIRC_FAN      7   // MOSFET module -> 12V circulation fan
#define PIN_BUZZER        8   // active buzzer
#define PIN_SD_CS        10

#define DHT_TYPE          DHT22   // AM2302

// Sensor calibration offsets (added to the raw reading), see docs/operation.md §6
#define T_OFFSET          0.0     // degC
#define RH_OFFSET         0.0     // %RH

// Logic level that turns a load ON (MOSFET modules = HIGH)
#define LOAD_ON           HIGH

// Float switch in the water tank: true = check it (closed to GND = water OK)
#define CHECK_WATER_LEVEL false

// ---------------------------------------------------------------------
//  Default set-points (changed from the keypad / serial, kept in EEPROM)
// ---------------------------------------------------------------------
#define DEF_T_SET        35.0   // degC
#define DEF_RH_SET       80.0   // %RH
#define DEF_LOG_SEC        60   // s between log records

// Allowed set-point ranges
#define T_SET_MIN        20.0
#define T_SET_MAX        50.0
#define RH_SET_MIN       30.0
#define RH_SET_MAX       95.0

// Required control tolerance (used for status colour, statistics, alarms)
#define T_TOL             1.0   // +/- degC
#define RH_TOL            2.0   // +/- %RH

// ---------------------------------------------------------------------
//  Control tuning (see docs/control-design.md)
//  Outputs are in percent (0..100).
// ---------------------------------------------------------------------
#define CTRL_PERIOD_MS   2000UL   // DHT22 cannot be read faster than 0.5 Hz

// Temperature PI(D) -> heater PWM
#define T_KP             20.0     // % per degC
#define T_KI              0.05    // % per (degC * s)
#define T_KD             60.0     // % per (degC / s), on measurement
#define T_IZONE           1.5     // degC: integrate only this close to set-point

// Humidity PI (split range): + -> humidifier time-proportioning, - -> exhaust fan
#define RH_KP             5.0     // % per %RH
#define RH_KI             0.01    // % per (%RH * s)
#define RH_IZONE          5.0     // %RH: integrate only this close to set-point
#define HUM_WINDOW_MS    5000UL   // time-proportioning window
#define HUM_MIN_ON_MS     500UL   // shorter pulses are carried over (module start-up time)

// Exhaust fan (D6, PWM): driven by the negative side of the humidity PI
#define EXHAUST_MIN_PCT  30.0     // lowest PWM duty at which the fan still spins
// Venting heat when too hot and heater already off (hysteresis)
#define VENT_T_ON         0.7     // degC above set-point -> exhaust >= VENT_T_PCT
#define VENT_T_OFF        0.2
#define VENT_T_PCT       50.0

// ---------------------------------------------------------------------
//  Safety
// ---------------------------------------------------------------------
#define T_ABS_MAX        60.0     // heater forced off + alarm above this
#define SENSOR_FAIL_MAX     3     // consecutive DHT failures before alarm
#define STABLE_SEC        300UL   // both in tolerance this long -> "stable"
#define OUT_OF_BAND_ALARM_SEC 600UL  // beep if out of tolerance this long after stable

#define SERIAL_BAUD    115200
