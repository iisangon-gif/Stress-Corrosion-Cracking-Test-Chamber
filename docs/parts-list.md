# Parts List (BOM)

The chamber produces a **3.5 wt% NaCl salt fog**. Everything metallic or electronic is kept **outside** the fog, except the DHT22 and the mist heads. See [design-notes-th.md](design-notes-th.md) for the reasoning (in Thai).

## A. Control electronics (in an IP65 box outside the chamber)

| # | Part | Qty | Recommended model / spec | Notes |
|---|---|---|---|---|
| 1 | Microcontroller | 1 | **Arduino UNO R3** | Flash 31,870 / 32,256 B, RAM 1,480 / 2,048 B |
| 2 | Temperature / humidity sensor | 1 | **DHT22 – AM2302 wired version** with a PTFE/sintered filter cap | lives in the fog: conformal-coat the leads and treat it as a consumable |
| 3 | LCD with input keys | 1 | **Adafruit RGB LCD Shield 16×2** (MCP23017, I²C) or a compatible clone | 5 keys. The backlight shows status (green = in tolerance, red = alarm) |
| 4 | Data logging | 1 | **Adafruit Data Logging Shield rev B** (microSD + PCF8523 RTC). DS1307 clones also work: set `RTC_IS_PCF8523 0` | stack it under the LCD shield |
| 5 | microSD card + RTC cell | 1 | 2–32 GB FAT32, CR1220/CR2032 | |
| 6 | Heater switch (230 VAC heater) | 1 | **SSR-25DA** (3–32 VDC in, 24–380 VAC out, zero-cross) + heat sink | driven by D5, time-proportioned (2 s window) |
| 7 | Mist-head switch | 1 | **logic-level MOSFET module** (AOD4184 / D4184 or IRLZ44N board) | 5 V side. *Do not use a DC SSR here: its ~1.5 V drop starves the 5 V heads.* Alternative: a second SSR-25DA on the 230 V input of the USB adapter |
| 8 | Fan switches | 2 | logic-level MOSFET modules (or SSR-DD DC SSRs) | fresh-air blower (D6), circulation fan (D7, optional) |
| 9 | Flyback diodes | 2 | 1N5819 | across each fan when switched by MOSFETs |
| 10 | Alarm indicator | 1 | 5 V active buzzer **or** lighted push button (5 V LED) | D8. A 12/24 V lamp needs a MOSFET |
| 11 | Enclosure | 1 | IP65 box, cable glands, DIN terminals | |

**Removed to stay within the UNO R3:** float switch for low salt-solution level, door reed switch, DS18B20 solution thermometer.

## B. Salt fog generation

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 12 | **Ultrasonic mist heads** | 4 | USB 5 V mist modules with their own driver boards, all powered together | must **start misting when powered** (no button press) |
| 13 | 5 V supply for mist heads | 1 | **5 V ≥ 3 A** (4 × ~0.5 A + margin): USB adapter 5 V 4 A, or a 12→5 V 5 A buck converter | |
| 14 | USB-A female sockets / 4-way splitter | 1 | – | one switched 5 V rail feeds all 4 heads |
| 15 | Salt solution reservoir | 1 | PP/PE tank with lid | holds the 3.5 wt% NaCl the heads float in or draw from |
| 16 | Reagent NaCl + DI water | – | NaCl ≥ 99.5 %, DI water ≤ 20 µS/cm | 35.0 g NaCl + 965.0 g water per kg |

## C. Heating (outside the fog)

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 17 | **Silicone rubber heater pad** | 1 | **230 VAC, 100–150 W** (e.g. 200×300 mm), self-adhesive, with built-in thermistor/fuse option | bond it to the **outside** of a thin chamber wall/floor, then insulate over it |
| 18 | Heat-spreader wall (recommended) | 1 | 2–3 mm aluminium sheet as one wall/floor, **epoxy- or PP-coated on the fog side** | good heater → air coupling. A thick plastic wall makes ±1 °C hard to hold after the lid is opened |
| 19 | Thermal fuse / bimetal cut-out | 1 | **72 °C, 10 A**, in series with the heater, mounted on the pad | hardware over-temperature protection |

## D. Air handling (outside the fog where possible)

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 20 | **Fresh-air blower** | 1 | 12 V DC 40–60 mm fan or small blower, mounted **outside**, blowing room air **into** the chamber | its motor only ever sees clean room air. Add a flap/check valve against back-flow |
| 21 | Passive outlet | 1 | PVC pipe Ø20–25 mm leading down to a water trap / drain | lets displaced fog out without pressurising the box |
| 22 | Circulation fan (optional) | 1 | **IP68 brushless 12 V fan**, leads sealed with epoxy | the only fan inside the fog. Treat it as a consumable, or leave it out |

## E. Power

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 23 | 12 V supply | 1 | 12 V 2 A | Arduino VIN + fans |
| 24 | 5 V supply | 1 | see item 13 | mist heads |
| 25 | Mains protection | 1 | RCD/RCBO 30 mA, 2 A fuse for electronics, fuse sized to heater | **mains wiring by a qualified person** |

## F. Chamber

| # | Part | Notes |
|---|---|---|
| 26 | Box 20–60 L in PP, PVC or acrylic, with a sloped lid | no bare metal inside |
| 27 | Insulation 20 mm PE/XPS foam outside | covers the heater pad too |
| 28 | Specimen rack in PP / PTFE / glass | SCC specimens electrically insulated from each other |
| 29 | Baffle / mesh | keeps the mist plume off the DHT22 and the specimens |

## G. Software

| Item | Source |
|---|---|
| Arduino IDE 2.x | arduino.cc |
| Libraries (Library Manager) | **DHT sensor library**, **Adafruit Unified Sensor**, **Adafruit RGB LCD Shield Library**, **SD** |
| PC logger / plot (optional) | Python 3 + `pyserial`, `matplotlib` (see `tools/`) |
