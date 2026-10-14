# SCC Chamber Environmental Control

Arduino UNO R3 controller for a small **stress corrosion cracking (SCC) test chamber**. It holds a set temperature and relative humidity within **±1 °C** and **±2 %RH** and records both.

> **สรุปภาษาไทย:** ระบบควบคุมอุณหภูมิและความชื้นสำหรับตู้ทดสอบ SCC ใช้ Arduino UNO R3, เซนเซอร์ DHT22 (AM2302),
> เครื่องทำความชื้นอัลตราโซนิก USB, ฮีตเตอร์ PTC 12 V, พัดลมระบายอากาศ, จอ LCD I²C 16×2 พร้อมปุ่มกด 5 ปุ่มสำหรับตั้งค่า
> และบันทึกข้อมูลลง microSD/คอมพิวเตอร์ ควบคุมให้อยู่ในช่วง ±1 °C และ ±2 %RH — ดูรายละเอียดในโฟลเดอร์ `docs/`

![Simulated response](docs/img/simulated-response.png)

## What's here

| Deliverable | Where |
|---|---|
| **Control system design**: architecture, PID / split-range PI loops, safety, simulation results | [docs/control-design.md](docs/control-design.md) |
| **Parts list**: electronics, actuators, power, chamber | [docs/parts-list.md](docs/parts-list.md) |
| **Wiring**: pin map, shield stack, signal and power diagrams | [docs/wiring.md](docs/wiring.md) |
| **Arduino code** | [firmware/SCC_EnvControl/](firmware/SCC_EnvControl/) |
| Operation, keypad, logging, tuning, calibration | [docs/operation.md](docs/operation.md) |
| PC tools: serial logger and plot/tolerance report | [tools/](tools/) |
| Simulation and UI tests (run the real firmware on a PC) | [test/sim/](test/sim/) |

## System at a glance

```
            ┌──────────── insulated chamber 20–40 L ────────────┐
            │  DHT22 ◄─ airflow ── circulation fan ── PTC heater │
            │  USB ultrasonic humidifier        exhaust fan ─────┼──► out
            └───────────────────────────────────────────────────┘
                 │D2        │D3          │D5        │D7      │D6
            ┌────┴──────────┴────────────┴──────────┴────────┴──┐
            │ Arduino UNO R3 + Data Logging Shield (SD + RTC)   │
            │              + RGB LCD Shield 16x2 (5 keys, I²C)  │
            └───────────────────────────────────────────────────┘
```

* **Sensor:** DHT22 / AM2302 (±2 %RH, ±0.5 °C, 0.1 resolution). The DHT11 is too coarse for this tolerance.
* **Temperature:** PID → 12 V PTC heater via PWM MOSFET, with integral zone and anti-windup.
* **Humidity:** one split-range PI. Positive output → USB humidifier (time-proportioned, 5 s window). Negative output → exhaust fan speed.
* **Input / display:** set T and RH with the keypad (or serial). Values are saved in EEPROM. The backlight turns **green** when the chamber is stable in tolerance.
* **Recording:** CSV to microSD (one file per day, RTC time stamps) and to USB serial. `tools/plot_log.py` charts a log and reports the in-tolerance %.
* **Safety:** sensor-fail and over-temperature cut-outs, an optional humidifier water-level switch, an 8 s watchdog, and a hardware 72 °C thermal fuse.

## Quick start

1. Build the hardware per [parts-list](docs/parts-list.md) and [wiring](docs/wiring.md).
2. In Arduino IDE, install the libraries **DHT sensor library**, **Adafruit Unified Sensor**, **Adafruit RGB LCD Shield Library** and **SD**.
3. Open `firmware/SCC_EnvControl/SCC_EnvControl.ino`, check `config.h`, and upload to **Arduino UNO**.
4. Serial Monitor at 115200 baud: `TIME 2026-10-14 09:00:00`, then `SET T 35` and `SET RH 80`. Or press **SELECT** on the keypad.
5. Wait for the green backlight (`OK`). The chamber is now within ±1 °C / ±2 %RH.

## Firmware facts

* Builds for the UNO at **32,008 / 32,256 bytes of flash and 1,454 / 2,048 bytes of RAM**. The RTC is driven directly and no float-formatting library code is used, so it fits with the SD logger. If you add features, comment out `USE_RTC` to free about 1 KB, or move to a Mega 2560.
* On every push, GitHub Actions compiles the sketch for `arduino:avr:uno` and runs the host-side test suite (`test/sim/run.sh`). The suite covers the keypad/LCD/serial interface, the fail-safe behaviour, and closed-loop tolerance checks on 9 simulated chamber variants.
* Control gains were chosen by closed-loop simulation over 11 chamber variants. Verify them on your chamber ([operation.md §5](docs/operation.md#5-tuning-on-the-real-chamber)).
