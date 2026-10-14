# Parts List (BOM)

## A. Control electronics

| # | Part | Qty | Recommended model / spec | Notes |
|---|---|---|---|---|
| 1 | Microcontroller | 1 | **Arduino UNO R3** (ATmega328P) | as specified |
| 2 | Temperature / humidity sensor | 1 | **DHT22 – AM2302 wired version** (with cable and built-in pull-up) | 0–100 %RH ±2 %, −40–80 °C ±0.5 °C, 0.1 resolution. If you use a bare 4-pin DHT22, add a 10 kΩ pull-up |
| 3 | LCD with input keys | 1 | **Adafruit RGB LCD Shield Kit 16×2** (#714 positive or #716 negative), or a compatible "I²C 1602 RGB LCD keypad shield" based on MCP23017 | I²C only (A4/A5), 5 buttons (SELECT, UP, DOWN, LEFT, RIGHT). The backlight colour shows status |
| 4 | Data logging (recording) | 1 | **Adafruit Data Logging Shield rev B** (#1141: microSD + PCF8523 RTC). Clones with DS1307 also work: set `RTC_IS_PCF8523 0` | SD uses D10–D13. Stack it **under** the LCD shield |
| 5 | microSD card | 1 | 2–32 GB, FAT32 | one CSV file per day |
| 6 | RTC backup cell | 1 | CR1220 (Adafruit shield) / CR2032 (DS1307 clones) | keeps time when power is off |
| 7 | MOSFET switch modules | 4 | **Logic-level** N-MOSFET modules, e.g. **D4184 / AOD4184 "MOS trigger" module** or IRLZ44N boards | heater, humidifier, exhaust, circulation fan. *Avoid IRF520 modules*: they do not switch fully at 5 V |
| 8 | Flyback diodes | 2 | 1N5819 / 1N4007 | across each fan if the MOSFET module has none |
| 9 | Active buzzer | 1 | 5 V active buzzer | alarm |
| 10 | Float switch (optional) | 1 | small vertical PP float switch | protects the humidifier from running dry |
| 11 | Stackable headers / proto area | – | – | the logging shield has a prototyping area for the wiring |

## B. Actuators

| # | Part | Qty | Recommended model / spec | Notes |
|---|---|---|---|---|
| 12 | **USB ultrasonic humidifier module** | 1 | 5 V USB atomizer module with driver PCB, 108–113 kHz, Ø16–20 mm disc, about 20–40 mL/h, about 2 W | Choose a module that **starts misting as soon as it is powered**. Many modules use a push button to toggle on/off; with those, power-switching leaves them OFF (see wiring §5) |
| 13 | Water reservoir for humidifier | 1 | 100–300 mL PP cup with lid, plus the module's cotton wick or float | distilled water only |
| 14 | Heater | 1 | **12 V PTC heater element with aluminium fins, 50 W** (e.g. 77×35 mm PTC heater) | size for 20–40 L insulated box. Use 80–100 W for 40–60 L |
| 15 | Thermal fuse | 1 | **72 °C, 10 A**, in series with the heater | hardware over-temperature protection |
| 16 | Circulation fan | 1 | 12 V 60×60 mm or 80×80 mm DC fan (2-wire) | blows across the heater |
| 17 | Exhaust fan | 1 | 12 V 40×40 mm or 50×50 mm DC fan (2-wire, speed controlled) | mount in the chamber wall. Add a passive inlet hole with a mesh filter on the opposite side |

## C. Power

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 18 | Power supply | 1 | **12 V DC, 8 A (100 W)**, enclosed type with IEC/terminal input | heater 4.2 A + fans 0.3 A + humidifier/logic about 0.7 A |
| 19 | DC-DC step-down | 1 | **LM2596 or MP1584 buck, set to 5.0 V, ≥ 2 A** | powers the USB humidifier |
| 20 | USB-A female socket / cut USB cable | 1 | – | connects the humidifier to the 5 V buck through its MOSFET |
| 21 | DC barrel plug 5.5×2.1 mm | 1 | – | 12 V to the Arduino VIN jack |
| 22 | Fuse + holder | 1 | 10 A blade fuse on 12 V output, 2 A on 230 V input | |
| 23 | Mains switch, cable, RCD outlet | 1 | – | mains wiring must be done by a qualified person |

## D. Chamber

| # | Part | Qty | Notes |
|---|---|---|---|
| 24 | Insulated box 20–40 L | 1 | EPS/PU cooler box, or acrylic/PP box wrapped in 20 mm foam |
| 25 | Specimen rack | 1 | PP / PTFE / glass — no bare metal that could corrode and contaminate |
| 26 | Thermal mass | 1 | sealed 0.5–1 L water bottle (helps keep T within ±0.3 °C) |
| 27 | Drip tray | 1 | under the humidifier outlet |
| 28 | Cable glands | 3–4 | sensor, fans, heater and humidifier wires through the wall |
| 29 | Mesh / baffle | 1 | keeps the mist plume off the sensor |

## E. Software (free)

| Item | Source |
|---|---|
| Arduino IDE 2.x | arduino.cc |
| Libraries (Library Manager) | **DHT sensor library** (Adafruit), **Adafruit Unified Sensor**, **Adafruit RGB LCD Shield Library**, **SD** |
| PC logger / plot (optional) | Python 3 + `pyserial`, `matplotlib` (see `tools/`) |
