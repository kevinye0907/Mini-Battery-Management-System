# Shopping list

This is everything the two-week build in `PLAN.md` needs. Check what you already own
before ordering.

## Buy

| Qty | Part | AliExpress search term | Notes |
|---|---|---|---|
| 1 | ADS1115 module | `ADS1115 module 16 bit` | Reads the three cell voltages. Pick a pre-soldered variant if one is offered |
| 5 | IRLB8721 MOSFET | `IRLB8721 TO-220` | Switches the load off (spares included). Pick the IRLB8721 variant |
| 1 | 10 Ω, 50 W resistor | `10 ohm 50W aluminum shell resistor` | The load. It gets hot: rest it on metal or a tile |
| 1 | XL4015 CC/CV buck module with voltmeter | `XL4015 CC CV buck module voltmeter` | The adjustable power supply. Pick a version with screw terminals if available |
| 1 | 24 V, 3 A power adapter, US plug | `24V 3A power adapter` | Powers the XL4015. Skip it if you have an old laptop charger (19–20 V) |
| 1 | Barrel jack to screw terminal adapter | `DC barrel jack female screw terminal` | Connects the adapter to the XL4015. Must match the plug (usually 5.5×2.1 mm). Many adapters include one |
| 1 | 22 AWG solid-core wire kit | `22AWG solid core wire kit` | Resistor leads and power wiring. 22 AWG handles the 1.3 A load |

## Only if you'll solder at home

Otherwise use your school's or your design team's soldering station.

| Qty | Part | Search term | Notes |
|---|---|---|---|
| 1 | Soldering iron | `Pinecil V2` | Buy from the official PINE64 store |
| 1 | Solder | `0.8mm flux core solder` | |
| 1 | Heat shrink kit | `heat shrink tube kit` | Optional: insulates the joints on the power resistor |

Soldering needed: about 10 header pins on the ADS1115 (unless pre-soldered) and two
wires on the power resistor's tabs.

## Only if you don't already own them

| Qty | Part | Search term | Notes |
|---|---|---|---|
| 1 | ESP32 dev board (38-pin) | `ESP32 DevKitC WROOM-32 38pin` | Check the headers are pre-soldered |
| 1 | USB cable for the ESP32 | Match the board's port (USB-C or micro-USB) | Must be a data cable, not a charge-only one |
| 1 | 1% metal-film resistor kit | `1% metal film resistor kit 30 values` | Needs 47k, 51k, 68k, 100k, 220k, 1k ×3, 10k, 100 Ω |
| 1 | Breadboard + jumper wires | `830 point breadboard`, `dupont jumper wire` | |
| 3 | 100 nF ceramic capacitors | `ceramic capacitor 104` | Optional: steadies the ADC readings |
| 1 | 2 kΩ trimmer (or a trimmer kit) | `3296 trimmer potentiometer kit` | Optional: swap in for one 1 kΩ to make an uneven "cell" |
| 1 | Multimeter | `ANENG AN8008` | Needed to calibrate the readings and set the current limit |

## Setting up the XL4015 (before connecting your circuit)

1. With nothing on the output, turn the **CV** screw until the display reads 11.1 V.
   It's a multi-turn screw, so it can take 10–20 turns before anything changes.
2. Set the multimeter to its 10 A current range, connect it across the output, and turn
   the **CC** screw until it reads about 1.5–2 A. Then disconnect it.
3. Check the voltage again, then connect your circuit.

## Only if you extend the project later

None of this is part of `PLAN.md`. Buy it only once the two weeks work.

**Week 3, current sensing (optional in the plan)**

| Qty | Part | Search term |
|---|---|---|
| 1 | INA219 module | `INA219 module I2C` |

**Real 3S lithium pack**

| Qty | Part | Where | Notes |
|---|---|---|---|
| 4 | 18650 cells, matched (3 + 1 spare) | Local battery retailer or vape shop, **not AliExpress** | Molicel P26A, Samsung 25R/30Q or LG HG2, all the same model |
| 1 | 18650 charger | XTAR or Nitecore official store | e.g. XTAR VC4SL. Charges cells one at a time |
| 3 | Single 18650 holders with wire leads | `18650 battery holder 1 slot with wire` | |
| 1 | Inline mini blade fuse holder | `inline mini blade fuse holder 16AWG` | |
| 1 | 5 A mini blade fuses | `mini blade fuse 5A` | |
| 1 | LiPo safety bag | `LiPo safe bag` | Not a "fireproof document bag" |
| 1 | 12 V LED strip (optional) | `12V LED strip` | Something real for the pack to power |

**Full BMS: balancing and temperature**

| Qty | Part | Search term |
|---|---|---|
| 3 | PC817 optocouplers | `PC817 DIP-4` (bare chips, not a module) |
| 3 | 47 Ω, 2 W resistors | `47 ohm 2W metal film resistor` |
| 3 | 10k NTC thermistors, B3950 | `NTC 10K 3950 thermistor epoxy` |
| 1 | Kapton tape | `kapton tape` |
| 1 | MP1584 buck module | `MP1584 buck converter` (runs the ESP32 off the pack) |
