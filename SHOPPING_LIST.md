# MiniBMS shopping list

Search terms are written so you can paste them into AliExpress. Buy a few spares
of anything small: AliExpress parts are cheap, and shipping takes 1–3 weeks.

## A. Core electronics

| Qty | Search term | Notes |
|---|---|---|
| 2 | `ESP32 DevKitC WROOM-32 38pin USB-C` | Pick the CP2102 or CH340 USB chip. A spare saves you if one dies |
| 1 | `ESP32 expansion board 38pin` (screw-terminal breakout) | Optional but handy: the ESP32 is too wide for one breadboard |
| 2 | `INA219 module I2C` | Current + pack-voltage sensor |
| 2 | `ADS1115 module 16 bit` | Precise ADC for the cell voltages |
| 10 | `IRLB8721 TO-220` | 1 load switch + 3 balancers + spares. Must be **IRLB**8721 (logic-level) |
| 20 | `PC817 DIP-4 optocoupler` | You need 3 |
| 5 | `NTC 10K 3950 thermistor epoxy` (MF52 type, ~1 m leads) | You need 3 |

## B. Passive components

| Qty | Search term | Notes |
|---|---|---|
| 1 | `1% metal film resistor kit 30 values 600pcs` | Check the listing includes 100Ω, 330Ω, 1k, 10k, 47k, 51k, 68k, 100k, 220k |
| 10 | `47 ohm 2W metal film resistor` | Balance bleed resistors (need 3) |
| 1 | `ceramic capacitor kit` (must include 100nF / 104) | |
| 1 | `3296 trimmer potentiometer kit` (or 2 × `B2K potentiometer`) | For the pack simulator |
| 3 | `10 ohm 50W aluminum shell resistor` | Test loads. They get hot: mount on scrap aluminum or a tile |

## C. Battery hardware

| Qty | Search term | Notes |
|---|---|---|
| 5 | `18650 battery holder 1 slot with wire` | Single holders, so every cell junction is reachable. Need 3 |
| 1 | `inline mini blade fuse holder 16AWG` | |
| 1 | `mini blade fuse 5A` (pack of 10) | |
| 1 | `LiPo safe bag fireproof` (large) | All real-cell testing happens inside it |

## D. Wiring and prototyping

| Qty | Search term | Notes |
|---|---|---|
| 2 | `830 point breadboard` | The ESP32 straddles two boards |
| 1 | `dupont jumper wire 40pin` M-M, M-F, F-F | |
| 1 | `22AWG solid core wire kit` | Neat breadboard wiring |
| 1 | `18AWG silicone wire` red + black | The high-current power path |
| 1 | `lever wire connector 221` (pack of 10–20) | Easy, solder-free power connections |
| 1 | `heat shrink tube kit` | |
| 1 | `kapton tape` | Holds thermistors on cells; doesn't melt |

## E. Tools (skip what you can borrow)

| Qty | Search term | Notes |
|---|---|---|
| 1 | `ANENG AN8008` or `ZOYI ZT102` multimeter | Essential |
| 1 | `Pinecil` soldering iron (or any temperature-controlled iron) + `0.8mm solder` + `flux` | Breakout boards usually arrive with headers you solder yourself |
| 1 | **Power supply.** Best: `DC lab power supply 30V 5A`. Budget: `ZK-4KX buck boost CC CV` module + a `24V 3A power adapter` | Needs adjustable voltage **and** a current limit. Lets you simulate the pack safely |
| 1 | `MP1584 buck converter` | Later: powers the ESP32 from the pack |

## F. Don't buy these on AliExpress

**18650 cells.** Most cheap AliExpress cells are fakes or rewrapped used cells,
often with a fraction of the claimed capacity ("9900mAh" is always fake). Buy
**3 matched cells (+1 spare)** from a reputable battery retailer or a vape shop:
Molicel P26A, Samsung 25R/30Q or LG HG2. Get them all at once, same model.

**Charger:** XTAR or Nitecore (e.g. XTAR VC4SL), from the brand's official store
or a local retailer.

## G. Can wait until the PCB stage
TI BQ76920 evaluation board, KiCad (free), JST-XH balance connectors.
