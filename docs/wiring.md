# Wiring

## 1. Pin map (Arduino UNO R3)

| UNO pin | Connected to | Type | Notes |
|---|---|---|---|
| **A4 / SDA, A5 / SCL** | RGB LCD shield (0x20) + RTC (0x68) | I²C | provided by the stacked shields |
| **D2** | DHT22 / AM2302 **DATA** | digital in/out | pull-up to 5 V is built into the AM2302. For a bare DHT22, add 10 kΩ (4.7 kΩ for cables > 3 m) |
| **D3** | **DC/DC SSR input (+)** → supply of the 4 mist-head drivers | digital out | timed bursts, 4 s window, ≥ 0.3 s. SSR input draws ~7–12 mA, which D3 can supply |
| **D5** | **heater SSR** → silicone heater pad on the finned heat sink inside the chamber (DC/DC SSR for a 24 VDC pad, SSR-25DA for 230 VAC) | digital out | time-proportioned, 2 s window |
| **D6** | MOSFET / SSR-DD → **fresh-air blower** | digital out | time-proportioned, 10 s window |
| **D7** | DC/DC SSR or MOSFET → **heater / circulation fan (IP68)** | digital out | **on whenever RUNning**, so the heater always has air flow |
| **D8** | active buzzer **or** LED of a lighted push button (5 V) | digital out | blinks on alarm; max 20 mA directly |
| D10–D13 | microSD (CS, MOSI, MISO, SCK) | SPI | on the Data Logging Shield |
| VIN | +12 V | power | |
| GND | common ground | | shared with the 12 V and 5 V supplies and all MOSFET modules |
| *free* | D4, D9, A0–A3 | | removed: level switch, door switch, DS18B20 |

## 2. Signal wiring

```
                    Arduino UNO R3 (+ logging shield + LCD shield)
                ┌──────────────────────────────────────┐
 AM2302   VCC ──┤ 5V                                   │
         DATA ──┤ D2                                   │
          GND ──┤ GND                                  │
                │ D3 ───────► (3)+  DC/DC SSR (mist)   │   SSR (4)− ── GND
                │ D5 ───────► DC+ (3) SSR-25DA (heater)│   SSR DC− (4) ── GND
                │ D6 ───────► SIG   MOSFET (fresh-air) │
                │ D7 ───────► SIG   MOSFET (circ. fan) │
                │ GND ──────► GND of all MOSFET modules│
                │ D8 ───────► Buzzer+ / button LED+    │   Buzzer− / LED− ── GND
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

   IP65 control box (outside): UNO + shields, SSRs, MOSFETs, PSUs, heater/fan connectors
```

## 5. DHT22 placement

* Mount it mid-height in the chamber, **out of the direct mist plume** and **out of the hot air stream from the heater module**, behind a mesh or baffle.
* Use the filter cap, and conformal-coat the solder joints and lead entry.
* Check it against a reference every few weeks. Salt deposits and long periods near 100 %RH make it drift.
