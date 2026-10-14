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
#define PIN_DHT           2   // DHT22 data (pull-up to 5V, built into AM2302)
#define PIN_MIST          3   // MOSFET / SSR -> 5V supply of the 4 ultrasonic mist heads
#define PIN_HEATER        5   // SSR / MOSFET -> heater (outside the salt fog)
#define PIN_VENT          6   // SSR / MOSFET -> fresh-air blower (outside the salt fog)
#define PIN_CIRC_FAN      7   // SSR / MOSFET -> circulation fan (optional, IP68)
#define PIN_ALARM         8   // active buzzer, or LED of a lighted push button
#define PIN_SD_CS        10
// free pins: D4, D9, A0..A3

#define DHT_TYPE          DHT22   // AM2302

// Sensor calibration offsets (added to the raw reading), see docs/operation.md §6
#define T_OFFSET          0.0     // degC
#define RH_OFFSET         0.0     // %RH

// Logic level that turns a load ON (MOSFET modules and SSRs = HIGH,
// most mechanical relay boards = LOW)
#define LOAD_ON           HIGH

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

// Temperature PID -> heater (time-proportioned)
#define T_KP             15.0     // % per degC
#define T_KI              0.03    // % per (degC * s)
#define T_KD            180.0     // % per (degC / s), on measurement
#define T_IZONE           1.5     // degC: integrate only this close to set-point

// Humidity PI (split range): + -> mist heads, - -> fresh-air blower
#define RH_KP             5.0     // % per %RH
#define RH_KI             0.01    // % per (%RH * s)
#define RH_IZONE          5.0     // %RH: integrate only this close to set-point

// Time-proportioning windows (output % -> ON-time within the window).
// Shorter ON-times are carried over to the next window, not lost.
#define HEATER_WINDOW_MS 2000UL   // 2 s = 100 mains half-cycles for a zero-cross SSR
#define HEATER_MIN_ON_MS   20UL
#define MIST_WINDOW_MS   5000UL
#define MIST_MIN_ON_MS    500UL   // mist heads need ~0.3-0.5 s to start (1000 if the
                                  // SSR switches the 230 V side of the USB adapter)
#define VENT_WINDOW_MS  10000UL
#define VENT_MIN_ON_MS   1500UL   // fan spin-up time

// Venting heat when too hot and heater already off (hysteresis)
#define VENT_T_ON         1.5     // degC above set-point -> vent >= VENT_T_PCT (last resort:
                                  // venting also dries the fog)
#define VENT_T_OFF        0.5
#define VENT_T_PCT       50.0

// ---------------------------------------------------------------------
//  Safety
// ---------------------------------------------------------------------
#define T_ABS_MAX        60.0     // heater forced off + alarm above this
#define SENSOR_FAIL_MAX     3     // consecutive DHT failures before alarm
#define STABLE_SEC        300UL   // both in tolerance this long -> "stable"
#define OUT_OF_BAND_ALARM_SEC 600UL  // beep if out of tolerance this long after stable

#define SERIAL_BAUD    115200
