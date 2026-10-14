# SCC Salt-Fog Chamber Control

Arduino UNO R3 controller for a small **stress corrosion cracking (SCC) test chamber**. It fills the chamber with **3.5 wt% NaCl salt fog** from 4 ultrasonic mist heads, holds a set temperature and relative humidity within **±1 °C** and **±2 %RH**, and records both.

> **สรุปภาษาไทย:** ตู้จำลองสภาวะไอน้ำทะเล พ่นหมอกน้ำเกลือ NaCl 3.5 %wt ด้วยหัวพ่น Ultrasonic 4 หัว (USB 5 V ทำงานพร้อมกัน)
> ควบคุมอุณหภูมิและความชื้นด้วย Arduino UNO R3 + DHT22 ให้อยู่ใน ±1 °C / ±2 %RH ตั้งค่าผ่านจอ LCD I²C พร้อมปุ่มกด
> และบันทึกข้อมูลลง microSD/คอมพิวเตอร์ ฮีตเตอร์ (แผ่นซิลิโคนผ่าน SSR) และพัดลมเติมอากาศติดตั้ง **นอกหมอกเกลือ** เพื่อกัน corrosion
> คำอธิบายอุปกรณ์และเหตุผลการออกแบบภาษาไทย: **[docs/design-notes-th.md](docs/design-notes-th.md)**

![Simulated response](docs/img/simulated-response.png)

## What's here

| Deliverable | Where |
|---|---|
| **Design notes (Thai)**: removed parts, pull-up, alarm/lighted button, MOSFET vs SSR, corrosion, mist heads | [docs/design-notes-th.md](docs/design-notes-th.md) |
| **Control system design**: loops, safety, simulation results | [docs/control-design.md](docs/control-design.md) |
| **Parts list** | [docs/parts-list.md](docs/parts-list.md) |
| **Wiring**: pin map, signal and power wiring, physical layout | [docs/wiring.md](docs/wiring.md) |
| **Arduino code** | [firmware/SCC_EnvControl/](firmware/SCC_EnvControl/) |
| Operation, keypad, logging, tuning | [docs/operation.md](docs/operation.md) |
| PC tools: serial logger, plot / tolerance report | [tools/](tools/) |
| Simulation and UI tests (run the real firmware on a PC) | [test/sim/](test/sim/) |

## System at a glance

```
 room air ─►[fresh-air blower D6]─► ┌──────── salt-fog chamber ────────┐ ─► passive outlet
            (outside the fog)       │  DHT22 (D2)     SCC specimens    │
                                    │  4 × ultrasonic mist heads (D3)  │
                                    │  in 3.5 wt% NaCl                 │
                                    └══════ thin Al wall ══════════════┘
                                      silicone heater pad (D5 → SSR), outside

  Arduino UNO R3 + Data Logging Shield (SD + RTC) + RGB LCD Shield (16x2, 5 keys)
  D8 → buzzer or lighted push button          free pins: D4, D9, A0–A3
```

* **Sensor:** DHT22 / AM2302 with a filter cap.
* **Temperature:** PID → silicone heater pad on the outside wall, switched by an SSR-25DA (2 s time-proportioning).
* **Humidity / fog:** one split-range PI. Positive output → the 4 mist heads, switched together on one 5 V rail (5 s window). Negative output → fresh-air blower.
* **All outputs are on/off** (time-proportioned), so MOSFET modules or SSRs both work.
* **Input / display:** set T and RH with the keypad (or serial), saved in EEPROM. The backlight turns **green** when stable in tolerance and **red** on alarm.
* **Recording:** CSV to microSD (one file per day, RTC time stamps) and to USB serial.
* **Safety:** sensor-fail and over-temperature cut-outs, an out-of-band alarm, an 8 s watchdog, and a 72 °C thermal fuse.

## Quick start

1. Build the hardware per [parts-list](docs/parts-list.md) and [wiring](docs/wiring.md).
2. Mix the solution: **35.0 g NaCl + 965.0 g DI water per kg**.
3. In Arduino IDE, install **DHT sensor library**, **Adafruit Unified Sensor**, **Adafruit RGB LCD Shield Library** and **SD**.
4. Open `firmware/SCC_EnvControl/SCC_EnvControl.ino`, check `config.h`, and upload to **Arduino UNO**.
5. Serial Monitor at 115200 baud: `TIME 2026-10-14 09:00:00`, then `SET T 35` and `SET RH 80`. Or press **SELECT** on the keypad.
6. Wait for the green backlight. Press **STOP** before opening the lid.

## Firmware facts

* UNO R3 usage: **31,870 / 32,256 bytes of flash, 1,480 / 2,048 bytes of RAM**.
* On every push, GitHub Actions compiles the sketch for `arduino:avr:uno` and runs `test/sim/run.sh`. That covers the UI, fail-safe behaviour, and closed-loop tolerance checks on 10 simulated chamber variants.
* Gains come from simulation. Verify them on your chamber ([operation.md §5](docs/operation.md#5-tuning-on-the-real-chamber)).
