# MiniBMS: a simple plan

This is a small project for a second-year student. It's built in two weeks, with one checkpoint
at the end of each. Everything else (dashboard, balancing, temperature, state of charge,
custom PCB) is optional and can wait.

**Assumes about 5 hours a week.** Shipping takes 1–3 weeks, so start the build when the parts arrive.

## What you're building

A monitor for three 18650 cells in series. It reads each cell's voltage and turns the load off
if a cell goes below 3.0 V or above 4.2 V. The load stays off until you reset it.

You won't use real cells in this version. A bench power supply stands in for the battery.

## Parts

Order only these (see `SHOPPING_LIST.md` for search terms):
- ESP32 dev board with the expansion board
- ADS1115 (cell voltages)
- IRLB8721 MOSFET (load switch)
- Resistor kit, 100 nF capacitors
- 3296 trimmer kit (optional, to make a fake cell voltage)
- 10 Ω 50 W power resistor (the load)
- Breadboard, jumper wires, 18 AWG wire
- Multimeter
- Bench power supply (school lab, or the ZK-4KX + 24 V adapter)

## Week 1: Read the cell voltages

**Goal:** the ESP32 prints the three cell voltages, and they match your multimeter.

1. **Test the board.** Blink runs when you upload the example sketch from the Arduino IDE.
2. **Find the ADS1115.** Connect it to 3V3, GND, SDA (GPIO21) and SCL (GPIO22). An I2C scanner
   should show address `0x48`.
3. **Build the three dividers** (values in `config.h` of the full firmware):
   - Tap 1: 47k over 100k, output to A0.
   - Tap 2: 100k over 51k, output to A1.
   - Tap 3: 220k over 68k, output to A2.
4. **Make a fake battery.** Connect a bench supply through three 1 kΩ resistors in series.
   The junctions act as the cell taps. Set the supply to 11.1 V.
5. **Upload `firmware/CellMonitor/CellMonitor.ino`** and open the serial monitor at 115200 baud.
6. **Check.** Each cell should read about 3.7 V. Compare each with your multimeter, and if
   one is off, correct its ratio in `RATIO` and re-upload.

**Checkpoint:** all three cells match your multimeter within about 10 mV.

## Week 2: Cut the load on a fault

**Goal:** the load turns off when a cell goes out of range, and stays off until you reset it.

1. **Wire the MOSFET.** Facing the printed side, pins are G, D, S from left to right.
   - Gate through 100 Ω to GPIO25, and 10 kΩ from gate to GND.
   - Drain to the load's negative lead.
   - Source to GND.
   - Load's positive lead to the supply's positive.
2. **Check the load turns on.** It should be on at boot if all cells are in range.
3. **Test the cutoff.** Set the supply to:
   - 12.9 V (4.3 V per cell): the load should turn off.
   - 8.7 V (2.9 V per cell): the load should turn off.
   - 11.1 V again: the load should stay off until you send `r` over serial.
4. **Test the reset.** Send `r` once the cells are back in range. The load should turn on.

**Checkpoint:** the load cuts off each time, stays off until you reset it, and comes back
after `r`. Save the serial log of one test.

## Optional: week 3

Add an INA219 to measure current through the load. Its library example is the place to start.
Any further features (dashboard, balancing, temperature, state of charge) are separate projects.

## When you're done

Write a short summary (half a page) for your portfolio:
- A block diagram: supply, dividers, ADS1115, ESP32, MOSFET, load.
- The three calibrated divider ratios.
- The serial log and a photo of the cutoff test.
- Two things you'd change next.

## Safety

- This version uses a bench supply only. Don't connect real cells until you've finished a
  separate project and planned that step with someone who knows lithium cells.
- Don't short the supply's output. Keep the 10 Ω load on a metal surface, since it gets hot.

## The full version

`firmware/MiniBMS/` is a larger firmware with dashboard, balancing, temperature and state of
charge. It's for later reference, and you don't need it for these two weeks.
