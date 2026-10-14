// =====================================================================
//  config.h — hardware pins, options and tuning for SCC_EnvControl
//  Board: Arduino Mega 2560
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
//  Pins (Arduino Mega 2560)
//  D20/D21 (SDA/SCL, also the SDA/SCL header next to AREF): LCD shield + RTC
//  D50..D52 / ICSP (SPI) + D10 (CS): SD card on the Data Logging Shield
// ---------------------------------------------------------------------
#define PIN_DHT           2   // DHT22 data (pull-up to 5V, built into AM2302)
#define PIN_MIST          3   // DC-DC SSR input -> 5 V supply of the 4 mist-head drivers
                              // (SSR in the 12 V feed of the 12->5 V step-down, or in the
                              // + line of a separate 5 V supply; a MOSFET also works)
#define PIN_HEATER        5   // SSR -> silicone heater pad on finned Al heat sink (inside
                              // the chamber; SSR-25DA for 230 VAC, DC/DC SSR for 24 VDC)
#define PIN_VENT          6   // SSR / MOSFET -> fresh-air blower (outside the salt fog)
#define PIN_CIRC_FAN      7   // SSR / MOSFET -> fan blowing through the heater fins
                              // (REQUIRED: on whenever the controller is running, so
                              // the heater never runs without air flow)
#define PIN_BUZZER        8   // active buzzer (beeps for un-acknowledged alarms)
#define PIN_SD_CS        10

// Status LEDs (each LED + 330-1k resistor to GND, or 5 V panel LEDs)
#define PIN_LED_RUN      22   // green : steady = stable in tolerance, blinking = settling
#define PIN_LED_ALARM    24   // red   : fast blink = new alarm, steady = acknowledged
#define PIN_LED_HEAT     26   // amber : heater output on
#define PIN_LED_MIST     28   // blue  : mist heads on
#define LED_ON          HIGH
// free pins: D4, D9, D11-D13 (not used by SD on the Mega), D23.., A0..A15

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
#define DEF_T_SET        35.0   // degC (0 = ROOM: no heating, temperature only logged)
#define DEF_RH_SET       70.0   // %RH
#define DEF_LOG_SEC        60   // s between log records

// Allowed set-point ranges
// Below T_SET_MIN the temperature set-point becomes "ROOM" (heater off).
// There is no cooler: a set-point must be ~3 degC above room temperature.
#define T_SET_MIN        20.0
#define T_SET_MAX        40.0
#define RH_SET_MIN       20.0   // RH below the room air's moisture needs dried
#define RH_SET_MAX       80.0   // inlet air (see docs/design-notes-th.md)

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
#define RH_KP             3.0     // % per %RH
#define RH_KI             0.006   // % per (%RH * s)
#define RH_IZONE          5.0     // %RH: integrate only this close to set-point

// Time-proportioning windows (output % -> ON-time within the window).
// Shorter ON-times are carried over to the next window, not lost.
#define HEATER_WINDOW_MS 2000UL   // 2 s = 100 mains half-cycles for a zero-cross SSR
#define HEATER_MIN_ON_MS   20UL
// Mist heads are sprayed in short timed bursts (not fast PWM). 4 heads add
// ~0.25 %RH per 0.5 s of mist in the 200 L chamber (~1 %RH in a 50 L box).
//   MIST_AC_SIDE_SWITCH 0: fast DC switch - DC-DC SSR or MOSFET in the 12 V / 5 V
//                          feed of the drivers                  -> 0.3 s bursts
//   MIST_AC_SIDE_SWITCH 1: SSR on the 230 V input of a USB adapter -> >= 1 s bursts
//                          (the adapter needs ~0.5 s to start)
#ifndef MIST_AC_SIDE_SWITCH
#define MIST_AC_SIDE_SWITCH 0
#endif
#if MIST_AC_SIDE_SWITCH
  #define MIST_WINDOW_MS  6000UL
  #define MIST_MIN_ON_MS  1000UL
#else
  #define MIST_WINDOW_MS  4000UL
  #define MIST_MIN_ON_MS   300UL
#endif
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
