# Operation Guide

## 1. Install and upload

1. Install **Arduino IDE 2.x**.
2. In **Library Manager**, install:
   * `DHT sensor library` (Adafruit), and accept `Adafruit Unified Sensor` when asked
   * `Adafruit RGB LCD Shield Library`
   * `SD`
   (`Wire`, `SPI` and `EEPROM` come with the IDE. The RTC is driven directly, so RTClib is not needed.)
3. Open `firmware/SCC_EnvControl/SCC_EnvControl.ino`.
4. Check `config.h`:
   * `RTC_IS_PCF8523 0` for the DS3231 module (default; also right for DS1307). Use `1` only for a PCF8523 (Adafruit Data Logging Shield rev B)
   * `PIN_SD_CS 53` for the microSD module (default)
   * `USE_SD` / `USE_RTC` can stay on even before the modules are fitted (see §2a)
5. Select **Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560** (Processor: ATmega2560) and upload. Choosing UNO stops the build with an error, because the LED pins exist only on the Mega.
6. Open the Serial Monitor at **115200 baud**, line ending **Newline**, and set the clock once:
   ```
   TIME 2026-10-14 09:00:00
   ```

## 2. Keypad and display

| Key | Normal view | In menu |
|---|---|---|
| **UP / DOWN** | change screen | previous / next setting |
| **LEFT / RIGHT** | – | decrease / increase value (hold to repeat, ×10 after 2 s) |
| **SELECT** | open menu | **save** and leave the menu |
| any key | acknowledge alarm: the buzzer stops, and the red LED turns steady | – |

If no key is pressed for 30 s, the menu closes **without saving**.

**Screens** (UP/DOWN):

```
 Main                Outputs             Stats                Log
┌────────────────┐  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐
│T 34.9/35.0C OK │  │Heat 23% Mst 12%│  │In band  99.8%  │  │Log 60s SD OK   │
│H 80.3/80.0% RUN│  │Vent  0% Fan ON │  │Max dT0.3 dH1.4 │  │14/10 09:15:02  │
└────────────────┘  └────────────────┘  └────────────────┘  └────────────────┘
  measured/set         actuator outputs    after "stable"      SD + clock
```

Status tag on the main screen: `..` = settling, `OK` = stable (in tolerance for 5 min), `!!` = alarm.
**Backlight colour:** yellow = settling, **green = stable in tolerance**, red = alarm, white = stopped, teal = menu.

**Status LEDs and buzzer:**

| Indicator | Off | Blinking | Steady |
|---|---|---|---|
| Green RUN (D22) | stopped | settling / out of tolerance | stable within ±1 °C / ±2 %RH |
| Red ALARM (D24) | no alarm | new alarm (fast blink) | alarm acknowledged, still active |
| Amber HEATER (D26) | heater off | – (pulses with the 2 s heater cycle) | – |
| Blue MIST (D28) | mist off | – (pulses with each mist burst) | – |
| Buzzer (D8) | silent | beeps 0.4 s every 1.2 s for new alarms; press any key to silence | – |

At power-on all LEDs light and the buzzer beeps once (lamp test).

**Menu items:** Set Temperature (20.0–40.0 °C in 0.1 steps; LEFT below 20.0 gives **ROOM** = no heating, temperature only logged), Set Humidity (20–80 %RH, 0.5 steps), Log interval (1–3600 s), Run / Stop, Reset stats. Changing a set-point or sending `RESET` also resets the statistics.

The lowest reachable RH depends on the room air. See [design-notes-th.md §0.4](design-notes-th.md) for a table, and use a silica-gel cartridge on the fresh-air inlet for lower RH.

## 2a. Fitting modules later

The controller runs with any of these modules missing, so the chamber can be built up step by step:

| Module missing | What happens |
|---|---|
| microSD module / card | records still go to USB serial; LCD Log screen shows `SD --` |
| DS3231 | time stamps are seconds since power-on; the SD file is `DATALOG.CSV` instead of one file per day |
| LCD shield | control runs; set it up over USB serial |
| DHT22 | `!SENSOR FAIL`: heater and mist stay **off** (fail-safe), buzzer beeps |
| SSRs / heater / mist / fans / LEDs / buzzer | the outputs switch as usual, with no feedback; watch the amber/blue LEDs to check the logic before connecting loads |

At power-on the serial monitor shows one line like:

```
# LCD OK | RTC OK 2026-10-14 09:00:00 | SD not found | DHT22 OK
```

and `DEVICES` prints it again at any time. Missing modules are looked for **every minute**, so an SD card, DS3231 or LCD fitted later is picked up **without a reset** ("# SD card found", "# RTC found", "# LCD found"). While the SD card is missing, each search can take ~2 s, and the heater and mist are held off during it. If a card is pulled out, the next write fails, and the controller keeps running and searches again.

After fitting the DS3231 for the first time, set the clock once: `TIME 2026-10-14 09:00:00`.

**Always connect or remove modules with the power off.** Hot-plugging can damage the SD card or the I²C bus.

Suggested bring-up order:

1. Mega + DHT22 + LCD: check T/RH on the screen and the keypad.
2. LEDs + buzzer: lamp test at power-on; unplug the DHT22 and the buzzer should beep.
3. DS3231 + microSD: `TIME …`, then check that `YYMMDD.CSV` appears on the card.
4. SSRs with a test lamp or a small load: they should follow the amber/blue LEDs.
5. Real loads: heater (with KSD301 + thermal fuse), mist heads, fans.

## 3. Serial commands (115200 baud)

| Command | Action |
|---|---|
| `SET T 35.0` | temperature set-point, 20–40 °C |
| `SET T ROOM` | no heating; temperature only logged |
| `SET RH 70` | humidity set-point, 20–80 %RH |
| `SET LOG 60` | log interval (s) |
| `RUN` / `STOP` | start / stop control (STOP switches all outputs off) |
| `RESET` | reset stability statistics |
| `STATUS` | one CSV record plus run state, in-band %, max deviations |
| `DEVICES` | which modules are fitted: LCD, RTC (with its time), SD card, DHT22 |
| `HEADER` | print the CSV header |
| `TIME YYYY-MM-DD HH:MM:SS` | set the RTC |
| `HELP` | list commands |

Lines starting with `#` are messages. All other lines are CSV data.

### Before opening the lid / refilling salt solution

There is no door switch and no level switch (not fitted):

1. Press **SELECT**, **UP** ×1 to *Run / Stop*, **RIGHT** to show STOP, then **SELECT** (or type `STOP`). The mist heads, heater and fans switch off.
2. Open the lid, inspect the specimens, and top up the 3.5 wt% NaCl solution. **Never let the mist heads run dry.**
3. Close the lid, then **RUN**. The chamber re-stabilises, and the statistics restart.

## 4. Recording

Each record contains:

```
timestamp,t_C,rh_pct,t_set_C,rh_set_pct,heater_pct,mist_pct,vent_pct,in_band,stable,alarms
2026-10-14 09:15:00,35.0,80.3,35.0,80.0,23,12,0,1,1,0
```

* **microSD:** one file per day named `YYMMDD.CSV` (e.g. `261014.CSV`), written only while running. Without an RTC it writes to `DATALOG.CSV`.
* **USB serial:** the same records, every log interval. To save them on a PC:
  ```
  pip install pyserial matplotlib
  python tools/serial_logger.py --port COM3 --out run1.csv      # Windows
  python tools/serial_logger.py --port /dev/ttyACM0 --out run1.csv
  ```
  The logger opens the port without toggling DTR, so connecting does not reset the Mega.
* **Plot and check the tolerance** of any log, from SD or PC:
  ```
  python tools/plot_log.py run1.csv          # shows chart + prints in-band statistics
  python tools/plot_log.py 261014.CSV --png chart.png
  ```
* `alarms` is a hex bit field: 1 = sensor fail, 2 = over-temperature, 8 = out of band.

## 5. Tuning on the real chamber

The default gains come from simulation (see [control-design.md §7](control-design.md#7-verification-by-simulation)). Verify them on the real chamber:

1. Set a typical point, e.g. `SET T 35` and `SET RH 80`, and log at 10 s (`SET LOG 10`) for 2–3 hours.
2. Plot with `tools/plot_log.py` and check:

| Symptom | Change in `config.h` |
|---|---|
| T oscillates (period minutes), heater swings 0↔100 % | lower `T_KP` (e.g. 15 → 10) and/or `T_KI` |
| T settles slowly, stays offset | raise `T_KI` (e.g. 0.03 → 0.05) |
| T overshoots after the lid is opened | raise `T_KD` (180 → 250), or improve heater-to-air coupling (thin Al wall) |
| RH oscillates, mist heads and blower alternate | lower `RH_KP` (3 → 2) and `RH_KI` (0.006 → 0.004) |
| RH sits 1–2 % below set-point for long | raise `RH_KI` slightly |
| Blower pulses too short to spin up | raise `VENT_MIN_ON_MS` |
| Mist heads switched on the 230 V side of a USB adapter (instead of the DC/DC SSR) | set `MIST_AC_SIDE_SWITCH 1` (longer bursts: expect more RH ripple with strong mist) |
| RH ripples with each mist burst | switch the mist on the DC side (DC/DC SSR or MOSFET), use a bigger box or fewer heads; lower `RH_KP` |
| RH ripple follows T ripple | improve T first: add thermal mass and insulation (1 °C ≈ 4.3 %RH at 35 °C / 80 %RH) |

Change one parameter at a time and re-upload. Settings in EEPROM (set-points, log interval) are kept.

## 6. Sensor check (recommended before testing)

Put the DHT22 in a sealed jar above a **saturated NaCl slurry**, which is 75.3 %RH at 25 °C, for at least 12 h. Compare the reading with 75.3 %. If the offset is more than 2 %, enter the correction in `config.h` and re-upload:

```cpp
#define RH_OFFSET  -1.8   // reading was 77.1 % in 75.3 % jar
#define T_OFFSET    0.0
```

## 7. Troubleshooting

| Problem | Check |
|---|---|
| `!SENSOR FAIL` | DHT22 wiring, pull-up, cable length; condensation on the sensor |
| T does not reach the set-point | set-point less than 3 °C above room temperature? heater power, insulation, thermal fuse |
| RH does not reach the set-point | voltage at the mist drivers ≥ 4.75 V while all 4 run (DC SSR drop), salt-solution level, mist heads start without a button press, crusted heads (rinse with DI water), blower stuck on, box leaking |
| RH cannot be lowered | room air is too humid. The fresh-air blower can only bring RH down to the room's moisture content |
| LCD blank | contrast pot on the shield; I²C address conflicts |
| `SD not found` | card ≤ 32 GB, FAT32; wiring D50–D53 + 5 V; SPI wires short; module has a level shifter. A card inserted later is found within 1 minute |
| Clock wrong / `No RTC` / `time not set` | DS3231 on D20/D21 + 5 V; backup cell fitted; `RTC_IS_PCF8523 0`; set the time with `TIME` |
| Program restarts every few seconds | look at the power-on report: a short on the I²C bus or a missing 5 V is reset by the I²C timeout, but fix the wiring |
