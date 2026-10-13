// MiniBMS configuration: pins, thresholds and calibration.
// Edit this file, not MiniBMS.ino, when tuning your build.
#pragma once

// ---------------- Pins (ESP32-WROOM-32 DevKit) ----------------
#define PIN_SDA            21
#define PIN_SCL            22
#define PIN_DSG_FET        25   // Discharge MOSFET gate (100R series, 10k pull-down)
#define PIN_BAL_1          16   // Balance optocoupler LEDs (330R series)
#define PIN_BAL_2          17
#define PIN_BAL_3          18
#define PIN_NTC_1          34   // Thermistor dividers (ADC1 only: ADC2 conflicts with WiFi)
#define PIN_NTC_2          35
#define PIN_NTC_3          32
#define PIN_STATUS_LED      2   // Onboard LED: solid = OK, blinking = fault

// ---------------- I2C addresses ----------------
#define INA219_ADDR      0x40
#define ADS1115_ADDR     0x48

// ---------------- Pack ----------------
#define NUM_CELLS           3
#define CELL_CAPACITY_MAH 2500.0f  // Rated capacity of your cells

// ---------------- Cell-tap voltage dividers ----------------
// Divider ratio = Rbottom / (Rtop + Rbottom). Start with the nominal values,
// then replace them with measured ratios (see the calibration step in the guide).
//   Tap 1: 47k / 100k  -> 100/147 = 0.6803
//   Tap 2: 100k / 56k  ->  56/156 = 0.3590
//   Tap 3: 150k / 47k  ->  47/197 = 0.2386
const float TAP_RATIO[NUM_CELLS] = { 0.6803f, 0.3590f, 0.2386f };

// ---------------- Protection thresholds ----------------
#define CELL_OV_V          4.25f   // Over-voltage fault
#define CELL_OV_CLEAR_V    4.15f
#define CELL_UV_V          3.00f   // Under-voltage fault
#define CELL_UV_CLEAR_V    3.20f
#define PACK_OC_A          2.50f   // Over-current fault (INA219 tops out at ~3.2 A)
#define TEMP_MAX_C         60.0f   // Over-temperature fault
#define TEMP_CLEAR_C       50.0f
#define TEMP_MIN_C        -10.0f   // Under-temperature fault

// Number of consecutive 100 ms samples a condition must persist before tripping.
// Over-current trips fastest; a hard short is the fuse's job, not firmware's.
#define DEBOUNCE_OV          5
#define DEBOUNCE_UV          5
#define DEBOUNCE_OC          2
#define DEBOUNCE_OT         10

// ---------------- Balancing ----------------
#define BAL_START_V        4.00f   // Only balance near the top of charge
#define BAL_DELTA_V        0.020f  // Start bleeding a cell this far above the lowest
#define BAL_STOP_DELTA_V   0.005f  // Stop when within this of the lowest

// ---------------- Thermistors (10k NTC, B=3950, 10k pull-up to 3.3 V) ----------------
#define NTC_R_FIXED     10000.0f
#define NTC_R_NOMINAL   10000.0f
#define NTC_BETA         3950.0f
#define NTC_T_NOMINAL_C    25.0f
#define NTC_ENABLED         true   // Set false until thermistors are wired

// ---------------- WiFi dashboard (ESP32 creates its own access point) ----------------
#define WIFI_AP_SSID    "MiniBMS"
#define WIFI_AP_PASS    "bms12345"   // 8+ characters; change it
