# MiniBMS: a 3S Li-ion battery management system on an ESP32

A breadboard-first BMS for three 18650 cells in series (3S, 9.0 to 12.6 V).
It measures every cell, pack current and cell temperatures. It cuts the load on
over-voltage, under-voltage, over-current or over-temperature, bleeds high cells
to balance the pack, estimates state of charge, and serves a live dashboard over WiFi.

Firmware: [`firmware/MiniBMS/`](firmware/MiniBMS/). Edit [`config.h`](firmware/MiniBMS/config.h) for pins, thresholds and calibration.

> **Safety first.** Li-ion cells can deliver tens of amps into a short and can
> catch fire. Until Phase 6 you work with a simulated pack, not real cells.
> When real cells come in: fuse first, LiPo bag always, never leave it charging
> unattended, and never let the bare ends of a wire touch pack + and −.

---

## 1. How it works

```
                 ┌──────────── ESP32 ─────────────┐
   cell taps ──► │ ADS1115 (I2C)   → cell voltages│
   shunt     ──► │ INA219  (I2C)   → pack current │──► WiFi dashboard + serial CSV log
   NTCs      ──► │ ADC1 pins       → temperatures │
                 │                                │
                 │ GPIO25 ──► discharge MOSFET    │  cuts the load on a fault
                 │ GPIO16-18 ──► balance circuits │  bleeds high cells near full
                 └────────────────────────────────┘
```

Every 100 ms: read all sensors → check limits → update state of charge.
Every 1 s: update balancing and log a CSV line.
Any fault **latches**: the load stays off until the condition has cleared *and*
you press reset. Without that, the load chatters: cutting it lets the cell
voltage bounce back up, which would clear an under-voltage fault, reconnect the
load, and trip again.

---

## 2. Shopping list (version 1, breadboard)

For exact AliExpress search terms, quantities and buying tips, see [`SHOPPING_LIST.md`](SHOPPING_LIST.md).

Changes from the original list: three **single-cell** holders instead of one 3×
holder (you need access to the junctions between cells), a **logic-level**
MOSFET instead of the FDPF55N06, and the plain INA219 **breakout** instead of the FeatherWing.

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32-WROOM-32 DevKit (38-pin) | |
| 1 | INA219 breakout (Adafruit #904 or clone) | Current + pack voltage, I2C 0x40 |
| 1 | ADS1115 breakout | 16-bit ADC for the cell taps, I2C 0x48 |
| 3 | Single 18650 holders with wire leads | Wired in series, so every junction is reachable |
| 3 | 18650 cells, matched (same model, same batch) | Samsung 25R/30Q, Molicel P26A, LG HG2. Phase 6 only |
| 4 | IRLB8721 N-MOSFET (TO-220) | 1 for load switching, 3 for balancing. Fully on at 3.3–4 V gate |
| 3 | PC817 optocoupler | Drives the balance FETs (explained in Phase 5) |
| 3 | 47 Ω, 2 W resistor | Balance bleed resistors (~90 mA per cell) |
| 3 | 10k NTC thermistor, B = 3950 | Epoxy-bead type with leads |
| 1 | Inline fuse holder + 5 A fuses | Blade or 5×20 mm glass |
| 3 | 10 Ω, 50 W aluminum-housed resistors | Test load: 1 alone ≈ 1.2 A, 2 in series ≈ 0.6 A, 2 in parallel ≈ 2.5 A. They get hot |
| — | 1% metal-film resistors: 47k, 100k ×5, 51k, 220k, 68k, 10k ×7, 100 Ω, 330 Ω ×3 | Dividers, pull-downs, LED resistors (all in a standard 30-value kit) |
| 4 | 100 nF ceramic capacitors | On the ADS1115 inputs, plus one decoupling cap |
| 1 | 1k–10k potentiometer, 3 × 1 kΩ resistors | Pack simulator |
| — | Breadboard, jumpers, 18 AWG silicone wire, screw terminals | Power path is **not** on the breadboard |
| 1 | Multimeter | Essential |
| 1 | LiPo safety bag | Essential |
| 1 | 18650 charger (XTAR/Nitecore) | Charges cells one at a time, outside the pack |
| — | Bench power supply (0–15 V, current limit) | Borrow one from your school lab if you need to; it makes Phases 2–4 much safer |

Later: an MP1584 or LM2596 buck module (12 V → 5 V) to run the ESP32 off the pack.

---

## 3. Wiring

### Pin map

| ESP32 pin | Connects to |
|---|---|
| 3V3, GND | INA219 VCC/GND, ADS1115 VDD/GND, NTC dividers |
| GPIO21 / GPIO22 | SDA / SCL (INA219 and ADS1115 share the bus) |
| GPIO25 | Discharge MOSFET gate (via 100 Ω, 10k gate-to-GND pull-down) |
| GPIO16 / 17 / 18 | Balance optocoupler LEDs, cells 1 / 2 / 3 (via 330 Ω) |
| GPIO34 / 35 / 32 | Thermistors, cells 1 / 2 / 3 |
| GPIO2 | Onboard LED: solid = OK, blinking = fault |

**One ground.** ESP32 GND, INA219 GND, ADS1115 GND and pack − (T0) are all the same node.

### Cells and taps

Cell 1 is at the bottom (connected to pack −). A "tap" is a junction between cells:

```
 T3 (pack +, 12.6 V max) ─── Cell 3 + 
                             Cell 3 −  ──┐
 T2 ( 8.4 V max) ────────────────────────┤
                             Cell 2 + ───┘
                             Cell 2 −  ──┐
 T1 ( 4.2 V max) ────────────────────────┤
                             Cell 1 + ───┘
 T0 (pack −, GND) ─────────── Cell 1 −
```

### Cell voltage sensing (ADS1115)

The ADS1115 can only read 0–3.3 V, so each tap goes through a divider:

```
 Tap ──[R top]──┬──► ADS1115 Ax        Tap  R top  R bottom  ratio   at max V
                │                      T1   47k    100k      0.680   2.86 V → A0
           [R bottom]  [100nF]         T2   100k   51k       0.338   2.84 V → A1
                │         │            T3   220k   68k       0.236   2.98 V → A2
 GND ───────────┴─────────┘
```

Firmware: `cell1 = T1`, `cell2 = T2 − T1`, `cell3 = T3 − T2`.
The 100 nF cap gives the ADC a low-impedance source to sample from.

> **Divider drain:** these dividers pull 30–150 µA, unevenly, which slowly
> unbalances the cells (~2 mAh/day). Unplug the taps when you're not using the
> board. Version 2 fix: switch the dividers with a MOSFET, or use a BQ76920.

### Power path: thick wire and screw terminals, not the breadboard

```
 T3 (pack +) ──[5 A FUSE]──► INA219 Vin+ ═shunt═ Vin− ──► LOAD +
                                                          LOAD −
                                                            │
                                                         D (drain)
         GPIO25 ──[100 Ω]──────────────────────────────── G   IRLB8721
                                         [10k]──────────    (G to S)
                                                         S (source)
 T0 (pack −) ───────────────────────────────────────────────┘── ESP32 GND
```

- IRLB8721 pins, facing the printed side, left to right: **G, D, S**.
- The 10k pull-down keeps the FET **off** while the ESP32 boots or resets. That makes it fail-safe.
- This is a **low-side** switch: it disconnects the load's negative wire.
- A single N-FET only blocks **discharge** current. Its body diode still lets
  charge current through, so in version 1 you charge cells outside the pack with
  the 18650 charger. Version 2 adds a second, back-to-back FET for charge protection.

### Temperature sensing

```
 3V3 ──[10k]──┬──► GPIO34 (35, 32)
              │
            [NTC]   ← taped to the cell
              │
 GND ─────────┘
```

---

## 4. Build phases

Every phase ends with a **checkpoint**. Don't move on until it passes.
Photograph or screenshot each checkpoint for your portfolio.

### Phase 0: Software setup

1. Install the Arduino IDE 2, then add the ESP32 board package
   (*Boards Manager → "esp32" by Espressif*).
2. Library Manager: install **Adafruit INA219** and **Adafruit ADS1X15**
   (accept the Adafruit BusIO dependency).
3. Board: *ESP32 Dev Module*. Upload the *Examples → Basics → Blink* sketch.

✅ The onboard LED blinks.

### Phase 1: I2C bring-up

Wire the INA219 and ADS1115 to 3V3, GND, SDA (21) and SCL (22). Upload an
I2C scanner sketch (*Examples → Wire → WireScan*, or search "ESP32 I2C scanner").

✅ The scanner finds `0x40` (INA219) and `0x48` (ADS1115).
If not: check that SDA and SCL aren't swapped, and that the breakouts have pull-ups (Adafruit's do).

### Phase 2: Cell voltage sensing on a simulated pack

Build a **pack simulator**: bench supply → three 1 kΩ resistors in series.
The junctions behave like cell taps T1 and T2, and the supply's + is T3. At 11.1 V, each "cell" reads 3.7 V.

1. Build the three dividers and wire them to A0–A2.
2. In `config.h`, set `NTC_ENABLED false` (or the missing thermistors will show
   as a sensor fault). Upload `MiniBMS.ino` and open Serial Monitor at 115200.
3. **Calibrate.** Measure each tap (T1, T2, T3 to GND) with your multimeter. The
   firmware's tap voltages are running sums of the logged cells: T1 = c1,
   T2 = c1 + c2, T3 = c1 + c2 + c3. Correct each ratio:
   `new_ratio = old_ratio × (firmware_tap_V / multimeter_tap_V)`.
   Put the corrected ratios in `TAP_RATIO` in `config.h`.

✅ All three cells read within ±10 mV of the multimeter.
Swap one 1k resistor for the pot: changing it should change exactly one or two "cells".

### Phase 3: Current sensing and the load switch

Wire the power path above, using the simulator supply (current limit ~3 A)
and a 10 Ω power resistor as the load.

✅ On boot the load turns on, and the INA219 reads about `V / 10 Ω` (≈1.1 A at 11.1 V).
✅ Send `o` over serial: the load turns off and current reads ~0.
✅ Press the ESP32's EN (reset) button: the load stays off during the reset, thanks to the pull-down.

### Phase 4: Protection, with deliberate faults

Use the simulator to **inject each fault** and confirm the BMS responds.
Log the serial CSV during each test. A plot of the voltage crossing the threshold
and the load dropping out is great portfolio material.

| Test | How | Expected |
|---|---|---|
| Over-voltage | Raise the supply to 12.9 V (4.3 V per cell) | `FAULT 0x01` within 0.5 s, load off |
| Under-voltage | Lower it to 8.7 V (2.9 V per cell) | `FAULT 0x02`, load off |
| UV latch | Raise back to 11.1 V without resetting | Load **stays off** until you send `r` |
| Over-current | Temporarily set `PACK_OC_A 1.5`, then add a second 10 Ω load in parallel (≈2.2 A; set the supply's current limit above that) | `FAULT 0x04` within 0.2 s |
| Sensor fault | Pull the T2 tap wire out | `FAULT 0x20` |
| Reset blocked | Send `r` while the supply is still at 12.9 V | "Cannot reset" message |

Fault codes: `0x01` OV, `0x02` UV, `0x04` OC, `0x08` over-temp, `0x10` under-temp, `0x20` sensor, `0x40` manual off.

### Phase 5: Temperature and balancing

**Thermistors:** wire the three dividers and set `NTC_ENABLED true`.
✅ Room temperature reads ~20–25 °C. A warm finger on one NTC raises it.
To test over-temperature, temporarily set `TEMP_MAX_C 30` and hold the NTC, or
use a hair dryer. Never use a soldering iron near cells.

**Balancing:** each cell gets a 47 Ω resistor that a MOSFET switches across it.
Cells 2 and 3 sit at 4–12 V above ground, so the ESP32 can't drive their FET
gates directly. An optocoupler bridges the gap: the ESP32 lights the LED, and
the isolated phototransistor connects the FET gate to that cell's own +.

```
 ESP32 GPIO ──[330 Ω]──► PC817 LED (pin 1)        Cell n + ──────────────┐
 GND ───────────────────  PC817 LED (pin 2)           │                [47 Ω 2 W]
                                                PC817 collector (pin 4)  │
                                                PC817 emitter (pin 3)──► G   IRLB8721
                                                      │                  D ─┘
                                                   [100k]                S
                                                      │                  │
                                                 Cell n − ───────────────┘
```

Repeat for all three cells. Cell n − is T(n−1); cell n + is T(n).
Balancing only runs when the highest cell is above 4.00 V. It bleeds any cell
more than 20 mV above the lowest, and stops at 5 mV.

✅ On the simulator, **temporarily use 10 kΩ instead of 47 Ω** as the bleed
resistors. A 47 Ω across a 1 kΩ simulator "cell" would collapse it to ~0.2 V
and trip under-voltage. Set the supply to ~12.3 V and turn the pot so one "cell"
reads ~50 mV higher. Its balancer turns on (the dashboard shows "balancing") and
that cell's voltage dips ~3%. It may toggle on and off every second; that's the
control loop working against a simulator, not a bug. Fit the 47 Ω resistors
before Phase 6.

### Phase 6: Real cells

1. Charge each cell to the same voltage (within 50 mV) with the standalone charger.
2. Put the holders and the board in the LiPo bag. Fit the **fuse** at T3 before connecting anything else.
3. Connect taps T0 → T3 **in order, bottom to top**. Check each tap with the multimeter first.
4. Restore any thresholds you lowered for testing.

✅ Cell voltages match the multimeter. The load runs, and current and SOC update.
Run one full discharge with a 10 Ω load (~2 hours for 2500 mAh cells) and confirm the UV cutoff stops it at 3.0 V per cell.

### Phase 7: State of charge and the dashboard

- At boot, the load is off, so the firmware reads each cell's **resting voltage**
  and looks up an initial SOC (the `OCV_V` table). From then on it **counts
  coulombs**: SOC falls by `current × time / capacity`.
- Connect your phone or laptop to WiFi `MiniBMS` (password in `config.h`) and open `http://192.168.4.1`.
- **Improve it:** the OCV table is a generic approximation. Measure your own: discharge
  in 10% steps, rest for 30 minutes, record the voltage. Then compare
  coulomb-counted SOC with the measured capacity, and report the error.

✅ After a full discharge, the counted mAh roughly matches the cells' rated capacity.

### Phase 8: Custom PCB (version 2)

Once the breadboard version is solid, move it to KiCad. Good upgrades:
- **TI BQ76920 analog front end:** dedicated per-cell measurement, coulomb counter,
  balancing and protection-FET drivers, read over I2C. This is how production BMSs are built.
- **Back-to-back FETs** for charge *and* discharge protection, plus a charge port.
- Switched cell-tap dividers (if you don't use the BQ76920) and an on-board buck converter.
- A JST-XH balance connector, so the board works with standard battery packs.

---

## 5. Going further (resume value)

- **Hardware watchdog:** if the firmware hangs, the load must turn off.
- **Kalman-filter SOC:** fuse coulomb counting with a voltage model.
- **Internal resistance estimation:** ΔV/ΔI at each load step tracks cell health (state of health, SOH).
- **Data logging** to SD or the cloud, with plots of a full cycle.
- **Write-up:** a block diagram, the schematic, a fault-injection results table,
  and discharge plots. That's the project page for your portfolio.
