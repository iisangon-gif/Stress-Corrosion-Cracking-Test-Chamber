# Wiring

## 1. Pin map (Arduino Mega 2560)

| Mega pin | Connected to | Type | Notes |
|---|---|---|---|
| **D20 / SDA, D21 / SCL** (also the SDA/SCL header next to AREF) | RGB LCD shield (0x20) + **DS3231 RTC module** (0x68) | I²C | the LCD shield takes them from its header; wire the DS3231 to the same two pins (see §1.1) |
| **D2** | DHT22 / AM2302 **DATA** | digital in/out | pull-up to 5 V is built into the AM2302. For a bare DHT22, add 10 kΩ (4.7 kΩ for cables > 3 m) |
| **D3** | **DC/DC SSR input (+)** → supply of the 4 mist-head drivers | digital out | timed bursts, 4 s window, ≥ 0.3 s. SSR input draws ~7–12 mA, which D3 can supply |
| **D5** | **heater SSR** → silicone heater pad on the finned heat sink inside the chamber (DC/DC SSR for a 24 VDC pad, SSR-25DA for 230 VAC) | digital out | time-proportioned, 2 s window |
| **D6** | MOSFET / SSR-DD → **fresh-air blower** | digital out | time-proportioned, 10 s window |
| **D7** | DC/DC SSR or MOSFET → **heater / circulation fan (IP68)** | digital out | **on whenever RUNning**, so the heater always has air flow |
| **D8** | **active buzzer 5 V** (+) | digital out | beeps for alarms not yet acknowledged. Drive it directly only if it draws ≤ 20 mA; otherwise (or for a loud 12 V buzzer) use a transistor or MOSFET module |
| **D22** | **green LED** "RUN / IN TOLERANCE" | digital out | steady = stable in tolerance, 1 Hz blink = settling, off = stopped |
| **D24** | **red LED** "ALARM" | digital out | fast blink = new alarm, steady = acknowledged, off = no alarm |
| **D26** | **amber LED** "HEATER" | digital out | lit while the heater output is on |
| **D28** | **blue LED** "MIST" | digital out | lit while the mist heads are on |
| **D50 / D51 / D52 / D53** | **microSD module** MISO / MOSI / SCK / CS | SPI | 6-pin module, VCC to 5 V (see §1.1) |
| VIN | +12 V | power | |
| GND | common ground | | shared with the 12 V supply and all MOSFET modules (SSR inputs are isolated) |
| *free* | D4, D9–D13, D23, D25, D27, D29–D49, A0–A15 | | float switch and door switch not fitted |

### 1.1 LCD shield, microSD module and DS3231 on the Mega

```
 microSD module (6-pin)          DS3231 module
   VCC  ── 5V                       VCC ── 5V
   GND  ── GND                      GND ── GND
   MISO ── D50                      SDA ── D20 (SDA)
   MOSI ── D51                      SCL ── D21 (SCL)
   SCK  ── D52                      (SQW, 32K: not used)
   CS   ── D53
```

* **RGB LCD Shield:** plugs straight onto the Mega. Use the R3 version, which takes I²C from the SDA/SCL pins next to AREF (= D20/D21 on a Mega). Older shields that only use A4/A5 need two jumper wires: A4 → D20 and A5 → D21. Wire the other signals (SSRs, LEDs, buzzer, DHT22, SD, DS3231) from the Mega's headers, or use a stacking-header / screw-terminal shield under the LCD.
* **microSD module:** choose one **with a 3.3 V regulator and a level shifter** (AMS1117 + 74LVC125A on the board); modules without them are 3.3 V only. Keep the SPI wires **≤ 15–20 cm**. Use a microSD card ≤ 32 GB formatted **FAT32**. This module type keeps MISO driven even when not selected, which only matters if another SPI device is added later.
* **DS3231 module:** the common ZS-042 board has a charging circuit for the backup cell. With a **non-rechargeable CR2032**, remove its charging diode/resistor, or fit a rechargeable **LIR2032**. Its EEPROM (0x57) does not clash with the LCD (0x20) or RTC (0x68).
* **Every module may be fitted later.** The controller reports at power-on what it found and looks again every minute (see [operation.md §2a](operation.md#2a-fitting-modules-later)).

### 1.2 Status LEDs and buzzer

```
 D22 ──[330 Ω]──►|── GND    green  RUN / IN TOLERANCE
 D24 ──[330 Ω]──►|── GND    red    ALARM
 D26 ──[330 Ω]──►|── GND    amber  HEATER
 D28 ──[330 Ω]──►|── GND    blue   MIST
 D8  ──────────► buzzer (+) ; buzzer (−) ── GND      (5 V active buzzer)
```

* Use 330 Ω–1 kΩ resistors (~5–10 mA), or 5 V panel-mount LEDs that already contain a resistor. **12/24 V pilot lamps** cannot be driven directly from the pins; switch them through a transistor/MOSFET module.
* Lighted push buttons can still be used as indicators: connect their LED like the LEDs above.
* At power-on all four LEDs light briefly and the buzzer gives one short beep (lamp test).

## 2. Signal wiring

```
                    Arduino Mega 2560 (+ RGB LCD shield)
                ┌──────────────────────────────────────┐
 AM2302   VCC ──┤ 5V                                   │
         DATA ──┤ D2                                   │
          GND ──┤ GND                                  │
                │ D3 ───────► (3)+  DC/DC SSR (mist)   │   SSR (4)− ── GND
                │ D5 ───────► (3)+  heater SSR         │   SSR (4)− ── GND
                │ D6 ───────► SIG   MOSFET (fresh-air) │
                │ D7 ───────► SIG   MOSFET (heater fan)│
                │ GND ──────► GND of all MOSFET modules│
                │ D8 ───────► Buzzer +                 │   Buzzer − ── GND
                │ D22/24/26/28 ─[330 Ω]─► LEDs         │   LED cathodes ── GND
                │ D50/51/52/53 ─► microSD MISO/MOSI/SCK/CS│ module VCC 5V, GND
                │ D20/D21 ────► DS3231 SDA/SCL         │   module VCC 5V, GND
                └──────────────────────────────────────┘
```

## 3. Power wiring

```
 Heater (preferred: 24 VDC pad, SELV):
 230 VAC ──[RCD 30 mA]──► 24 V PSU (+) ──[fuse]──► DC/DC SSR (1)+ ─ (2)− ──► KSD301 75 °C ─► thermal fuse 100 °C ─► heater pad (+)
                                   (−) ─────────────────────────────────────────────────────────────────────────► heater pad (−)
 Heater (alternative: 230 VAC pad):
 230 VAC L ──[RCD 30 mA]──[fuse]──► SSR-25DA (1) ~ (2) ──► KSD301 75 °C ─► thermal fuse 100 °C ─► heater pad ── N
                                                            heat sink ── protective earth (PE)

 230 VAC ──[RCD 30 mA]──[2 A fuse]──► 12 V PSU ──┬─► Arduino VIN (barrel jack)
                                                 ├─► MOSFET / SSR (fresh-air, D6) ─ blower
                                                 ├─► MOSFET / SSR (heater fan, D7) ─ IP68 fan
                                                 └─► DC/DC SSR (mist) (1)+ ─ (2)− ─► step-down 12→5 V IN+
                                                                                          OUT+ 5 V ─┬─ driver #1 +
                                                                                                    ├─ driver #2 +
                                                                                                    ├─ driver #3 +
                                                                                                    └─ driver #4 +
                                                                            step-down IN− / OUT− ── all driver −  ── 0 V
 0 V of the 12 V supply ── Arduino GND (common)

 Alternative with a separate 5 V supply:
   5 V PSU (+) ─► DC/DC SSR (1)+ ─ (2)− ─► drivers (+) ;  drivers (−) ── 5 V PSU (−)
```

* **SSR-25DA only switches AC.** Never use it on a DC load, because it will not turn off.
* **Mist heads:** the drivers have no control input, so D3 drives a **DC/DC SSR** that switches their supply, and one SSR feeds all 4 drivers.
  * **Preferred: put the SSR in the 12 V feed of the 12→5 V step-down.** The SSR's on-state drop (0.1–1.5 V depending on model) is then absorbed by the step-down, which still delivers 5 V, and the SSR only carries ~1 A. The step-down starts in milliseconds, so 0.3 s bursts still work.
  * **On the 5 V side of a separate supply:** check that the SSR's load-voltage range includes 5 V and that the drivers see **≥ 4.75 V with all 4 running**. If not, raise an adjustable supply to ~5.3 V, or move the SSR to the 12 V side.
  * DC SSR outputs are polarised: (1) = +, (2) = −. If the heads still mist weakly when off (leakage ~1–5 mA), add 1 kΩ / 0.5 W across the driver supply.
  * Both placements use the default `MIST_AC_SIDE_SWITCH 0`. Set `1` only if you switch the **230 V** input of a USB adapter instead (≥ 1 s bursts).
* **Mechanical relays:** not suitable for the heater or mist heads. They switch every few seconds, which is about 20,000 cycles per day.
* Fit a 1N5819 flyback diode (cathode to +) across each fan when it is switched by a MOSFET.
* **Heater safety (the heater is inside the salt fog):**
  * Prefer a **24 VDC** pad: a failing insulation in wet salt then cannot give a mains shock.
  * Fit the **KSD301 75 °C** bimetal (auto-reset) and a **~100 °C thermal fuse** on the heat sink, in series with the pad. They cut the heater if the fan stops, which the software cannot detect.
  * Use one continuous silicone lead from the pad out through a cable gland, with no joints inside the chamber, and a waterproof connector outside.
  * With a 230 V pad: earth the heat sink, and use an RCD/RCBO 30 mA.
* Mount the heater SSR on a heat sink (a 300 W / 24 V pad draws 12.5 A).
* Mains wiring must be done by a qualified electrician.

## 4. Physical layout (chamber 50 × 80 × 50 cm, 200 L, plastic)

```
                         sloped lid (drips run to the walls, not onto specimens)
       ┌──────────────────────────────── 80 cm ──────────────────────────────────┐
 room  │  drip shield                                                            │
 air ─►│[flap]◄─[fresh-air blower,     ┌────────┐           DHT22 (filter cap)  │
       │         outside, D6]          │ heater │           behind baffle,      │
       │                               │ module │           mid-height          │
       │   heater module ─────────────►│ fan ►►►│►► air                         │ 50 cm
       │   (pad + coated Al fins +     │ fins   │                               │ high
       │    IP68 fan, D5 + D7,         └────────┘   SCC specimens on PP rack    │
       │    on standoffs ≥ 5 cm,                                                │
       │    corner away from the mist)        ~~~ 4 mist heads in 3.5 % NaCl ~~~│──► passive outlet
       └────────────────────────────────────────────────────────────────────────┘    to water trap
         20 mm insulation outside (recommended)

   IP65 control box (outside): Mega + shields, SSRs, MOSFETs, PSUs, heater/fan connectors;
   status LEDs and buzzer on the box front panel
```

## 5. DHT22 placement

* Mount it mid-height in the chamber, **out of the direct mist plume** and **out of the hot air stream from the heater module**, behind a mesh or baffle.
* Use the filter cap, and conformal-coat the solder joints and lead entry.
* Check it against a reference every few weeks. Salt deposits and long periods near 100 %RH make it drift.
