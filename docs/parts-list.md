# Parts List (BOM)

The 200 L chamber produces a **3.5 wt% NaCl salt fog**. The electronics stay **outside**. Inside are the DHT22, the mist heads and the corrosion-protected heater module. See [design-notes-th.md](design-notes-th.md) for the reasoning (in Thai).

## A. Control electronics (in an IP65 box outside the chamber)

| # | Part | Qty | Recommended model / spec | Notes |
|---|---|---|---|---|
| 1 | Microcontroller | 1 | **Arduino Mega 2560** (R3) | Flash 33 KB / 256 KB, RAM 1.5 KB / 8 KB, so there is plenty of room for additions |
| 2 | Temperature / humidity sensor | 1 | **DHT22 – AM2302 wired version** with a PTFE/sintered filter cap | lives in the fog: conformal-coat the leads and treat it as a consumable |
| 3 | LCD with input keys | 1 | **Adafruit RGB LCD Shield 16×2** (MCP23017, I²C) or a compatible clone | 5 keys. The backlight shows status (green = in tolerance, red = alarm) |
| 4 | Data logging | 1 | **Adafruit Data Logging Shield rev B** (microSD + PCF8523 RTC) | stack it under the LCD shield. Rev B reads the SD card through the ICSP header, so it works on a Mega. Clones with SPI on D11–D13 need jumpers ([wiring §1.1](wiring.md)). DS1307 clones: set `RTC_IS_PCF8523 0` |
| 5 | microSD card + RTC cell | 1 | 2–32 GB FAT32, CR1220/CR2032 | |
| 6 | Heater switch | 1 | 24 VDC pad: **DC/DC SSR ≥ 25 A**. 230 VAC pad: **SSR-25DA** (zero-cross). Mount on a heat sink | driven by D5, time-proportioned (2 s window). Same as item 19c |
| 7 | Mist-head switch | 1 | **DC/DC SSR** (DC input 3–32 V, DC output, e.g. SSR-10DD / SSR-25DD, preferably MOSFET output), driven by D3 | The heads' own drivers have no control input, so their supply is switched; one SSR feeds all 4 drivers. **Best placed in the 12 V feed of the 12→5 V step-down**, where its 0.1–1.5 V drop does not matter. On the 5 V side, the SSR's load range must include 5 V, and the drivers must still see ≥ 4.75 V with all 4 running. A logic-level MOSFET module also works |
| 8 | Fan switches | 2 | DC/DC SSRs or logic-level MOSFET modules | fresh-air blower (D6), heater/circulation fan (D7, required) |
| 9 | Flyback diodes | 2 | 1N5819 | across each fan when switched by MOSFETs |
| 10 | Buzzer | 1 | **5 V active buzzer** (≤ 20 mA, or via a transistor) | D8, beeps for alarms not yet acknowledged |
| 10a | Status LEDs | 4 | 5 mm or panel-mount LEDs: **green, red, amber, blue** + 4 × 330 Ω (or 5 V panel LEDs with a built-in resistor) | D22 RUN/in tolerance, D24 ALARM, D26 HEATER, D28 MIST. Lighted push buttons can be used as the indicators instead |
| 11 | Enclosure | 1 | IP65 box, cable glands, DIN terminals | |

**Not fitted (by choice):** float switch for low salt-solution level, door reed switch, DS18B20 solution thermometer. The Mega has free pins (D4, D9, D23–D49, A0–A15) if they are added later.

## B. Salt fog generation

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 12 | **Ultrasonic mist heads** | 4 | USB 5 V mist modules with their own driver boards, all powered together | must **start misting when powered** (no button press) |
| 13 | 5 V supply for mist heads | 1 | **12→5 V step-down ≥ 5 A** (e.g. XL4015; an "LM2596 3 A" module only manages ~2 A continuously), or a separate 5 V ≥ 3 A supply | 4 × ~0.5 A ≈ 2 A (check the heads' labels). Set the output to 5.0–5.1 V under load |
| 14 | USB-A female sockets / 4-way splitter | 1 | – | one switched 5 V rail feeds all 4 heads |
| 15 | Salt solution reservoir | 1 | PP/PE tank with lid | holds the 3.5 wt% NaCl the heads float in or draw from |
| 16 | Reagent NaCl + DI water | – | NaCl ≥ 99.5 %, DI water ≤ 20 µS/cm | 35.0 g NaCl + 965.0 g water per kg |

## C. Heating module (inside the chamber, corrosion-protected)

The heater is a silicone pad on a finned aluminium heat sink with a fan blowing through the fins, mounted **inside** the 200 L chamber. Because it sits in the salt fog, every item below is chosen or treated for corrosion and electrical safety. See [design-notes-th.md §0.5](design-notes-th.md) for the reasoning.

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 17 | **Silicone rubber heater pad** | 1 | **24 VDC (preferred, SELV) or 230 VAC; 200 W with insulated walls, 300 W without** | edge-seal it with **neutral-cure** RTV silicone (not acetoxy). Use one continuous silicone lead out through a gland, with no joints inside |
| 18 | **Finned Al heat sink** | 1 | sized for 200–300 W with forced air, **hard-anodised + sealed, or epoxy/powder coated** (316 stainless or titanium fins as a corrosion-proof alternative) | mount on standoffs ≥ 5 cm from plastic walls, in the corner away from the mist heads, under a drip shield |
| 19 | Heater fan (= circulation fan, D7) | 1 | **IP68 brushless** 12/24 V fan, potted motor/PCB | runs whenever the controller is RUNning, so the heater never heats without air flow. Treat it as a consumable |
| 19a | Bimetal thermostat | 1 | **KSD301 NC 75 °C**, rated for the heater current, screwed to the heat sink | cuts the heater if the fan stops (auto-reset) |
| 19b | Thermal fuse | 1 | **~100 °C**, rated for the heater current, on the heat sink | last-resort one-shot cut-out |
| 19c | Heater switch | 1 | 24 VDC pad: **DC/DC SSR** ≥ 25 A (with heat sink). 230 VAC pad: **SSR-25DA** | driven by D5 |
| 19d | Waterproof connector | 1 | IP67/68, outside the chamber | the heater and fan module can be removed for rinsing and replacement |

## D. Air handling

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 20 | **Fresh-air blower** | 1 | 12 V DC 40–60 mm fan or small blower, mounted **outside**, blowing room air **into** the chamber | its motor only ever sees clean room air. Add a flap/check valve against back-flow |
| 20b | Silica-gel cartridge (optional) | 1 | clear tube with indicating silica gel on the blower inlet | only for RH set-points below the room air's moisture (e.g. 20–30 %RH). Regenerate by baking |
| 21 | Passive outlet | 1 | PVC pipe Ø20–25 mm leading down to a water trap / drain | lets displaced fog out without pressurising the box |

## E. Power

| # | Part | Qty | Spec | Notes |
|---|---|---|---|---|
| 22 | 24 V supply (for a 24 V heater pad) | 1 | **24 V ≥ 15 A** (300 W pad) or ≥ 10 A (200 W) | also fine for a 24 V heater fan |
| 23 | 12 V supply | 1 | **12 V ≥ 3 A** | Arduino VIN + fans + step-down for the mist heads (~1 A at 12 V) |
| 24 | 5 V supply | 1 | see item 13 | mist heads, switched by the DC/DC SSR (item 7) |
| 25 | Mains protection | 1 | **RCD/RCBO 30 mA**, fuses per supply. With a 230 V pad, **earth the heat sink** | **mains wiring by a qualified person** |

## F. Chamber

| # | Part | Notes |
|---|---|---|
| 26 | **Plastic box 50 × 80 × 50 cm (W × L × H), 200 L**, sloped lid | **PP or acrylic** (good to ~70–100 °C). PVC softens at ~60–70 °C, so keep the heater ≥ 5 cm away. No bare metal inside |
| 27 | Insulation 20 mm PE/XPS foam outside (recommended) | cuts heat loss from ~6.6 to ~2.3 W/K, allowing a 200 W heater instead of 300 W |
| 28 | Specimen rack in PP / PTFE / glass | SCC specimens electrically insulated from each other |
| 29 | Baffles / mesh / drip shield | keep the mist plume off the DHT22, the specimens and the heater fins |

## G. Software

| Item | Source |
|---|---|
| Arduino IDE 2.x | arduino.cc |
| Libraries (Library Manager) | **DHT sensor library**, **Adafruit Unified Sensor**, **Adafruit RGB LCD Shield Library**, **SD** |
| PC logger / plot (optional) | Python 3 + `pyserial`, `matplotlib` (see `tools/`) |
