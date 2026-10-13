# MiniBMS build plan: timeline and step-by-step outline

This plan splits the full BMS into four milestones. Each milestone works on its own,
and each one is a good portfolio entry if you stop there.

| Milestone | Goal | Real batteries? | Est. hours | Calendar |
|---|---|---|---|---|
| **M0. Order and prepare** | Buy parts, learn the basics | No | 4 | Week 0 |
| **M1. Battery monitor** | Read 3 "cells" and current, show them on a dashboard | No | 25 | Weeks 1–3 |
| **M2. Protection** | Cut the load on over/under-voltage and over-current | No | 15 | Weeks 4–5 |
| **M3. Full BMS** | Temperature, balancing, real cells, state of charge | Yes (last step) | 35 | Weeks 6–10 |
| **M4. Custom PCB (optional)** | Make a proper board, or use a BQ76920 | Yes | 40+ | Months later |

**Assumes about 8 hours a week.** Double the calendar if you have less time. Shipping from
AliExpress takes 1–3 weeks, so M0 is really "order now, start M1 when the parts arrive."

---

## M0. Order and prepare (Week 0, about 4 hours)

### Step 0.1. Confirm the trimmed order
- Order **only** the M1 and M2 parts (listed in the chat summary and in `SHOPPING_LIST.md`).
- Do **not** order 18650 cells, the charger or the MP1584 yet. You'll need them in M3.
- Check each listing's variant against the list (PC817 is DIP, not SOP; INA219 is the
  breakout module, not the FeatherWing).

### Step 0.2. Learn the vocabulary (2 hours)
Read through these (or ask me about any of them):
- Ohm's law, voltage dividers, series and parallel resistors
- What a MOSFET does as a switch (gate, drain, source; "logic-level" means it turns on at 3.3 V)
- I2C: two wires (SDA, SCL), addresses, pull-up resistors
- ADC: what a 16-bit reading means, and why the ESP32's built-in ADC is not precise enough

### Step 0.3. Set up the software (2 hours, can run while parts ship)
1. Install **Arduino IDE 2**.
2. *File → Preferences → Additional boards manager URLs*: add
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. *Boards Manager*: install **esp32 by Espressif** (version 2.0.x or newer 2.x).
4. *Library Manager*: install **Adafruit INA219** and **Adafruit ADS1X15** (accept
   the Adafruit BusIO dependency).
5. Board: *ESP32 Dev Module*. Port: whatever appears when you plug in the board.

**Checkpoint:** *File → Examples → 01.Basics → Blink* runs on the ESP32's onboard LED.

---

## M1. Battery monitor (Weeks 1–3, about 25 hours)

**Goal:** the ESP32 reads three simulated cell voltages and the current through a load,
and shows them on a phone or laptop. There's no switching yet.

**What you need:** ESP32, INA219, ADS1115, dividers and a breadboard, a bench power supply
(the ZK-4KX + 24 V adapter, or a school lab supply), three 1 kΩ resistors, a 10 Ω power
resistor, a multimeter.

### Step 1.1. Soldering and the first board test (2 hours, week 1)
1. Solder pin headers to the ESP32 expansion board, the INA219 and the ADS1115. Use
   the solder and iron (or do this at your school's makerspace).
2. Plug the ESP32 into the breadboard so the USB side is on one edge.
3. Plug it in via USB. Blink runs.

**Checkpoint:** the ESP32 shows up as a serial port, and Blink works.

### Step 1.2. Find the I2C devices (1 hour, week 1)
1. Wire INA219 and ADS1115: VCC → 3V3, GND → GND, SDA → GPIO21, SCL → GPIO22.
2. Upload an I2C scanner (search "ESP32 I2C scanner", or *Examples → Wire → WireScan*).
3. Open Serial Monitor at 115200 baud.

**Checkpoint:** the scanner finds `0x40` (INA219) and `0x48` (ADS1115).
**If not:** check SDA and SCL aren't swapped. Check the breakouts have power. Check for
solder bridges between pins.

### Step 1.3. Cell voltage sensing with the divider (4 hours, week 1–2)
1. Build the three dividers from `config.h`:
   - Tap 1: 47k (top) to 100k (bottom), output to A0 (ADS1115 channel 0).
   - Tap 2: 100k (top) to 51k (bottom), output to A1.
   - Tap 3: 220k (top) to 68k (bottom), output to A2.
   Put a 100 nF cap across each bottom resistor, and the 100k leg should go to GND.
2. Build the **pack simulator**: bench supply + 1 kΩ + 1 kΩ + 1 kΩ in series. The junctions
   act as taps T1 and T2, and the supply's + is T3.
3. Set `NTC_ENABLED false` in `config.h`. Upload `MiniBMS.ino`.
4. Set the supply to 11.1 V. Each "cell" should read about 3.7 V.

**Checkpoint:** each cell reads within 10 mV of your multimeter.

### Step 1.4. Calibrate (2 hours, week 2)
1. For each tap, measure to GND with the multimeter.
2. Compare to the firmware's reading for that tap. The firmware reports cells, so compute
   the tap as a running sum: T1 = c1, T2 = c1 + c2, T3 = c1 + c2 + c3.
3. Fix each ratio: `new_ratio = old_ratio × (firmware_tap / multimeter_tap)`.
4. Put the new values in `TAP_RATIO` in `config.h`. Re-upload.

**Checkpoint:** all three cells read within 10 mV after calibration.
**Note:** write down the measured ratios. They're a good line for your write-up.

### Step 1.5. Current sensing (3 hours, week 2)
1. Wire the power path for the load: supply + → INA219 Vin+, INA219 Vin− → load +,
   load − → supply −. The 10 Ω resistor is the load.
2. Add a simple **slide switch** in series with the load. Use it instead of the MOSFET
   for now.
3. Set the supply to 11.1 V. Turn the switch on.

**Checkpoint:** the INA219 reads about 1.1 A (11.1 V / 10 Ω). Switching off reads about 0 A.

**Safety:** the 10 Ω power resistor gets very hot. Mount it on a metal surface or a tile,
and don't touch it for several minutes after use.

### Step 1.6. Dashboard (4 hours, week 3)
1. Open `MiniBMS.ino` and confirm the WiFi access point is named `MiniBMS`.
2. Upload. Connect your phone to `MiniBMS` (password in `config.h`).
3. Open `http://192.168.4.1`.

**Checkpoint:** the phone shows three cell voltages, the pack voltage, the current and
the SOC value, updating about once a second.

### Step 1.7. Log a test (2 hours, week 3)
1. Keep the serial monitor open and log the CSV output for 10 minutes while you change
   the supply voltage in steps (11.1 V → 11.5 V → 11.1 V).
2. Paste the CSV into a spreadsheet and plot cell voltage against time.

**Checkpoint:** you have one good plot. Save it. It goes in your write-up.

**M1 is done. Write a half-page summary now, while it's fresh.** Include the block
diagram, the divider calibration numbers and the plot.

---

## M2. Protection (Weeks 4–5, about 15 hours)

**Goal:** the ESP32 cuts the load when a cell goes out of range, and the fault stays latched
until you reset it.

**What you need:** the IRLB8721 MOSFET, one 10k pull-down resistor, one 100 Ω gate resistor.

### Step 2.1. Replace the switch with the MOSFET (3 hours, week 4)
1. Remove the slide switch.
2. Wire the MOSFET (IRLB8721, TO-220, facing the printed side, pins left to right: G, D, S):
   - Gate ← 100 Ω ← GPIO25.
   - Gate → 10k → GND (keeps it off during reset).
   - Drain → load −.
   - Source → supply − (ESP32 GND).
3. Upload the firmware. The load should turn on at boot.

**Checkpoint:** the load runs at boot. Sending `o` over serial turns it off.
**Check:** pressing the ESP32's EN (reset) button doesn't switch the load on during the reset.

### Step 2.2. Understand the protection logic (1 hour, week 4)
Read `checkProtection()` and `resetFaults()` in `MiniBMS.ino`. Be ready to explain:
- Why faults are **debounced** (several bad readings in a row trip a fault).
- Why faults are **latched** (the load would otherwise chatter on and off).
- Why the reset requires the **clear** thresholds, which sit above the trip thresholds.

Write these three answers in your own words. They're your interview answers.

### Step 2.3. Inject faults (5 hours, week 4–5)
Use the bench supply to trigger each fault. Log the serial output every time.

| Test | How | Expected |
|---|---|---|
| Over-voltage | Set supply to 12.9 V (4.3 V per cell) | `FAULT 0x01` within 0.5 s, load off |
| Under-voltage | Set to 8.7 V (2.9 V per cell) | `FAULT 0x02`, load off |
| Latch | Raise back to 11.1 V without resetting | Load **stays off** |
| Reset blocked | Send `r` while still at 12.9 V | "Cannot reset" message |
| Reset works | Set to 11.1 V, send `r` | Load comes back on |
| Over-current | Set `PACK_OC_A 1.5` in `config.h`, then add a second 10 Ω in parallel | `FAULT 0x04` within 0.2 s |
| Sensor fault | Pull one tap wire out | `FAULT 0x20` |

**Checkpoint:** all seven tests pass. Save each log.

**Common problems:**
- Fault trips too fast or too slow: adjust the `DEBOUNCE_*` values in `config.h`.
- Load turns back on immediately: check the gate pull-down, and check that `loadOn`
  respects `faults`.

### Step 2.4. Record the result (2 hours, week 5)
Make a plot of the voltage crossing the threshold and the load dropping out. That single
plot is the strongest thing in your portfolio so far.

**M2 is done. Update your half-page summary to include protection.**

---

## M3. Full BMS (Weeks 6–10, about 35 hours)

**Goal:** temperature sensing, balancing, real cells, a full discharge test and state of charge.

**Order these now:** PC817 (×3), 47 Ω 2 W resistors (×3), 10k NTC thermistors (×3),
Kapton tape, 18650 holders (×3), fuse and holder, LiPo bag. Buy the **cells** and
**charger** last, from a local retailer.

### Step 3.1. Thermistors (3 hours, week 6)
1. Wire each thermistor as a divider: 3V3 → 10k → GPIO (34, 35, 32) → NTC → GND.
2. Set `NTC_ENABLED true` in `config.h`.
3. Upload.

**Checkpoint:** room temperature reads about 20–25 °C. Warming one NTC with your finger
raises its reading.

**Test over-temperature:** temporarily set `TEMP_MAX_C 30` and warm one sensor. It should
trip `FAULT 0x08`. Then restore 60.

**Never use a soldering iron or heat gun near lithium cells.**

### Step 3.2. Balancing circuit (6 hours, week 6–7)
This step is the hardest wiring in the project. Build one channel at a time.

For each cell, the circuit is:
```
ESP32 GPIO ──330Ω──► PC817 LED (pin 1)
GND ───────────────► PC817 LED (pin 2)
PC817 collector (pin 4) ──► Cell n+ side of the gate network
PC817 emitter (pin 3) ──► IRLB8721 gate
IRLB8721 drain ──► Cell n+ through 47 Ω 2 W
IRLB8721 source ──► Cell n−
```

1. Build **one channel** on the breadboard and test it with the simulator first.
2. Temporarily use **10 kΩ** instead of 47 Ω. A 47 Ω across a simulator "cell" would drop it
   to about 0.2 V and trip under-voltage.
3. Set the supply to 12.3 V, turn the pot so one "cell" reads about 50 mV high. The
   balancer should turn on, and the dashboard should show "balancing".
4. Repeat for cells 2 and 3.
5. Put the 47 Ω resistors back before real cells.

**Checkpoint:** each channel balances its own cell, and only that cell.
**Warning:** check that PC817 pin numbers match your datasheet before you solder.

### Step 3.3. Real cells, safely (4 hours, week 8)
1. **Buy and check the cells.** Charge each to the same voltage (within 50 mV) with the
   standalone charger. Charge outside the pack.
2. Put the holders and the board in the **LiPo bag**.
3. Fit the **fuse** on the pack + line **before** connecting anything else.
4. Connect taps **bottom to top**: T0 first, then T1, T2, T3. Check each with the multimeter
   before moving to the next.
5. Restore all test thresholds in `config.h`.

**Checkpoint:** cell voltages match the multimeter. The load turns on, and current and SOC update.

**Never:** leave a charging cell unattended, let the wire ends touch across pack + and −,
or bypass the fuse.

### Step 3.4. Full discharge test (4 hours, week 9)
1. Charge the pack fully. Use a 10 Ω load (about 1.2 A at 12 V).
2. Let it discharge. It takes about 2 hours for 2500 mAh cells.
3. The UV cutoff should stop it at 3.0 V per cell.

**Checkpoint:** the BMS stops the discharge at the right voltage, and logged mAh roughly
matches rated capacity.

### Step 3.5. State of charge and dashboard polish (6 hours, week 9–10)
1. Check how the firmware seeds SOC at boot from the lowest cell's resting voltage (`OCV_V`).
2. Compare coulomb-counted SOC to the measured capacity from Step 3.4.
3. Improve the dashboard: add a graph of voltage over time, and display faults clearly.
4. Optional: measure your own OCV curve by discharging in 10% steps and resting 30 minutes
   at each step. Replace `OCV_V` with your data.

**Checkpoint:** after a full discharge, the counted mAh is within about 10% of rated capacity.
If it's off, that's a good result to explain in your write-up.

### Step 3.6. Final write-up (4 hours, week 10)
Put these in the repo:
- Block diagram and schematic (draw it in any tool, or hand-drawn and photographed)
- Divider calibration numbers and balancing test results
- Fault injection results table and its logs
- Discharge plot from Step 3.4
- Photos of the build
- A "what I'd do differently" section

---

## M4. Custom PCB (optional, months later)

This is a separate project. Don't start it until M3 works.

1. Learn KiCad (free). Draw the M3 schematic first.
2. Consider switching to a **TI BQ76920** analog front end, which does per-cell sensing,
   balancing and protection in one chip. Buy an evaluation board first, and read its
   datasheet.
3. Add back-to-back FETs (charge and discharge protection) and a balance connector.
4. Order from JLCPCB or PCBWay (about US$10–30 for a small batch).

---

## Progress log

Keep a dated entry for each step you finish. One line each is enough. It becomes the
material for your write-up.

| Date | Step | Result | Notes |
|---|---|---|---|
| | 0.1 | | |

## Safety rules, for the whole project

1. Never test with real cells until M3, Step 3.3.
2. Fuse before anything else on the pack + line.
3. Charge cells outside the pack, and never unattended.
4. Work inside the LiPo bag whenever a real cell is connected.
5. Don't short anything. A 10 Ω load is fine. A bare wire across a cell is not.
6. If a cell gets hot, smells, swells or hisses: disconnect it, move it outdoors on a
   non-flammable surface, and don't use it again.
