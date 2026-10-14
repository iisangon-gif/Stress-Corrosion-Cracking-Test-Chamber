# Wiring

## 1. Pin map (Arduino UNO R3)

| UNO pin | Connected to | Type | Notes |
|---|---|---|---|
| **A4 / SDA, A5 / SCL** | RGB LCD shield (MCP23017 @0x20) + RTC (@0x68) | I²C | provided by the shields when stacked |
| **D2** | DHT22 / AM2302 **DATA** | digital in/out | 10 kΩ pull-up to 5 V (built into the AM2302 wired version) |
| **D3** | MOSFET #1 SIG → **USB humidifier** | digital out | time-proportioned on/off |
| D4 | Float switch (optional) → GND | digital in (pull-up) | closed = water OK. Enable with `CHECK_WATER_LEVEL true` |
| **D5** | MOSFET #2 SIG → **12 V PTC heater** | PWM out (980 Hz) | |
| **D6** | MOSFET #3 SIG → **exhaust fan** | PWM out (980 Hz) | speed control |
| **D7** | MOSFET #4 SIG → **circulation fan** | digital out | on while running |
| D8 | Active buzzer (+) | digital out | buzzer (−) to GND |
| D10 | microSD CS | SPI | on the Data Logging Shield |
| D11 / D12 / D13 | microSD MOSI / MISO / SCK | SPI | on the Data Logging Shield |
| VIN (barrel jack) | +12 V | power | |
| GND | common ground | | **must be shared with the 12 V supply and all MOSFET modules** |

All pins are defined in [`firmware/SCC_EnvControl/config.h`](../firmware/SCC_EnvControl/config.h).

## 2. Shield stack

```
        ┌──────────────────────────────┐
        │  Adafruit RGB LCD Shield     │  ← top: 16x2 LCD + 5 keys (I2C)
        ├──────────────────────────────┤
        │  Data Logging Shield         │  ← middle: microSD (D10-13) + RTC (I2C)
        │  [proto area: DHT header,    │     solder the I/O wiring for D2..D8 and
        │   screw terminals to MOSFETs]│     the 10k pull-up here
        ├──────────────────────────────┤
        │  Arduino UNO R3              │  ← bottom
        └──────────────────────────────┘
```

## 3. Signal wiring

```
                     Arduino UNO R3 (+ shields)
                 ┌─────────────────────────────────┐
   AM2302  VCC ──┤ 5V                              │
          DATA ──┤ D2 ──[10k]── 5V (if bare DHT22) │
           GND ──┤ GND                             │
                 │                                 │
                 │ D3 ─────────────► SIG  MOSFET#1 (humidifier)
                 │ D5 ─────────────► SIG  MOSFET#2 (heater)
                 │ D6 ─────────────► SIG  MOSFET#3 (exhaust fan)
                 │ D7 ─────────────► SIG  MOSFET#4 (circulation fan)
                 │ GND ────────────► GND  of all four MOSFET modules
                 │                                 │
                 │ D8 ───► Buzzer + , Buzzer − ──► GND
                 │ D4 ───► Float switch ─────────► GND   (optional)
                 └─────────────────────────────────┘
```

## 4. Power wiring

```
 230 VAC ──[2 A fuse]──[switch]──► 12 V / 8 A PSU
                                     │+12V                                   │0V
                                     ├──[10 A fuse]──┬────────────────────── │ ──────────── common GND ── Arduino GND
                                     │               │                       │
                                     │               ├─► Arduino VIN (barrel jack)
                                     │               │
                                     │               ├─► MOSFET#2 VIN+ ─ OUT+ ─[72 °C thermal fuse]─ PTC heater ─ OUT− 
                                     │               ├─► MOSFET#3 VIN+ ─ OUT+ ─ exhaust fan (+) ── fan (−) ─ OUT−   (diode across fan)
                                     │               ├─► MOSFET#4 VIN+ ─ OUT+ ─ circ. fan (+)   ── fan (−) ─ OUT−   (diode across fan)
                                     │               │
                                     │               └─► Buck 12V→5V IN+          Buck IN− ── 0V
                                     │                      OUT+ 5.0 V ─► MOSFET#1 VIN+ ─ OUT+ ─ USB VBUS (red)  ─┐ USB
                                     │                      OUT−       ─► MOSFET#1 VIN− ─ OUT− ─ USB GND  (black) ─┘ humidifier
                                     │
                                     └─ all MOSFET VIN− terminals ── 0V
```

* The MOSFETs switch the **low side** (OUT− goes to the load's negative side). The load's + side is permanently at supply voltage.
* Fit flyback diodes (1N5819, cathode to +) across each fan unless the module already has them.
* Connect the 12 V supply's 0 V to Arduino GND, or the MOSFETs will not switch.
* Set the buck converter to 5.0 V **before** connecting the humidifier.

## 5. USB humidifier connection

1. Cut the USB-A plug off the humidifier cable, or use a USB-A female socket breakout.
2. Red wire (VBUS) → +5 V from the buck converter.
3. Black wire (GND) → MOSFET #1 OUT−. MOSFET #1 VIN− → buck converter OUT−.
4. White/green data wires are not used. Insulate them.
5. **Test the module first.** Apply 5 V directly. If it starts misting immediately, it is suitable. If it needs a button press, either:
   - look for a solder jumper or "auto-start" option on its PCB, or
   - choose a different module (auto-start USB atomizer, or a Grove "Water Atomization" module, which has an enable input).

## 6. DHT22 placement

* Mount it in the middle of the chamber, **in the circulation airflow**, about 10 cm from walls.
* Keep it **out of the direct mist plume** and away from the heater outlet. A small mesh or baffle in front of the humidifier outlet helps.
* Run the AM2302's lead through a cable gland. Keep the cable under 5 m and route it away from the heater and fan wiring.
