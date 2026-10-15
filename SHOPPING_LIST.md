# Shopping list

This is everything the two-week build in `PLAN.md` needs. Check what you already own
before ordering: if you have an ESP32, a breadboard and a resistor kit, you only need to
buy the first three items.

## Buy

| Qty | Part | AliExpress search term | Why |
|---|---|---|---|
| 1 | ADS1115 module | `ADS1115 module 16 bit` | Reads the three cell voltages |
| 5 | IRLB8721 MOSFET | `IRLB8721 TO-220` | Switches the load off (spares included) |
| 1 | 10 Ω, 50 W resistor | `10 ohm 50W aluminum shell resistor` | The load. It gets hot: rest it on metal or a tile |

## Use what you have, or buy

| Qty | Part | AliExpress search term | Needed values / notes |
|---|---|---|---|
| 1 | ESP32 dev board (38-pin) | `ESP32 DevKitC WROOM-32 38pin` | Any ESP32 dev board works |
| 1 | 1% resistor kit | `1% metal film resistor kit 30 values` | 47k, 51k, 68k, 100k, 220k, 1k ×3, 10k, 100 Ω |
| 1 | Breadboard + jumper wires | `830 point breadboard`, `dupont jumper wire` | |
| 3 | 100 nF ceramic capacitors | `ceramic capacitor 104` | Optional: steadies the ADC readings |

## Borrow if you can

| Tool | Notes |
|---|---|
| Multimeter | Needed to calibrate the readings |
| Bench power supply (0–15 V, current limit) | Stands in for the battery. A school lab or your design team will have one. Budget option to buy: `ZK-4KX buck boost CC CV` + `24V 3A power adapter` |
| Soldering iron | The ADS1115 usually arrives with loose header pins |

## Only if you extend the project later

- **Week 3, current sensing:** `INA219 module I2C`.
- **Full BMS (balancing, temperature, real cells):** PC817 optocouplers, 47 Ω 2 W resistors,
  10k NTC thermistors, single 18650 holders, a 5 A inline fuse, a LiPo safety bag, and
  matched 18650 cells plus a charger from a reputable local seller (not AliExpress).
