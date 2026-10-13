// CellMonitor: read three 18650 cell voltages through resistor dividers,
// print them, and cut the load if any cell leaves its safe range.
//
// Needs the "Adafruit ADS1X15" library (Library Manager).
// Board: "ESP32 Dev Module".
//
// Serial commands (115200 baud): send 'r' to clear a fault once all cells are back in range.

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

const int PIN_LOAD = 25;          // Gate of the IRLB8721 load switch (through 100 ohm)
const float CELL_MIN_V = 3.0;     // Cut the load below this
const float CELL_MAX_V = 4.2;     // Cut the load above this

// Divider ratios: Rbottom / (Rtop + Rbottom).
// Replace these with your multimeter-calibrated values (Phase 1, Step 1.4).
const float RATIO[3] = { 0.6803, 0.3377, 0.2361 };

Adafruit_ADS1115 ads;
float cell[3];
bool faulted = false;

void readCells() {
  float tap[3];
  for (int i = 0; i < 3; i++) {
    tap[i] = ads.computeVolts(ads.readADC_SingleEnded(i)) / RATIO[i];
  }
  // Tap 1 is cell 1 on its own. Each higher cell is the difference between taps.
  cell[0] = tap[0];
  cell[1] = tap[1] - tap[0];
  cell[2] = tap[2] - tap[1];
}

bool allCellsSafe() {
  for (int i = 0; i < 3; i++) {
    if (cell[i] < CELL_MIN_V || cell[i] > CELL_MAX_V) return false;
  }
  return true;
}

void setLoad(bool on) {
  digitalWrite(PIN_LOAD, on ? HIGH : LOW);
}

void setup() {
  pinMode(PIN_LOAD, OUTPUT);
  setLoad(false);               // Load off until the first reading says it's safe
  Serial.begin(115200);

  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 not found. Check wiring.");
    while (true) delay(100);
  }
  ads.setGain(GAIN_ONE);        // +/-4.096 V range; the dividers stay below 3.1 V

  readCells();
  faulted = !allCellsSafe();
  setLoad(!faulted);
  Serial.println("cell1_V,cell2_V,cell3_V,load");
}

void loop() {
  readCells();

  // A fault latches: the load stays off even if the voltage recovers,
  // until you send 'r'. Otherwise the load would switch on and off repeatedly.
  if (!allCellsSafe()) faulted = true;
  setLoad(!faulted);

  Serial.printf("%.3f,%.3f,%.3f,%s%s\n", cell[0], cell[1], cell[2],
                faulted ? "OFF" : "ON", faulted ? " (FAULT)" : "");

  if (Serial.available() && Serial.read() == 'r') {
    readCells();
    if (allCellsSafe()) {
      faulted = false;
      Serial.println("# Fault cleared");
    } else {
      Serial.println("# Cannot clear: a cell is still out of range");
    }
  }
  delay(500);
}
