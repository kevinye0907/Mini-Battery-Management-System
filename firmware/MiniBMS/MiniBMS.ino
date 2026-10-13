// MiniBMS: 3S Li-ion battery management system on an ESP32.
//
// Measures each cell (ADS1115 on the cell taps), pack current (INA219) and
// cell temperatures (NTC thermistors). Opens the discharge MOSFET on any fault,
// bleeds high cells near full charge, estimates state of charge, and serves
// a live dashboard over WiFi.
//
// Libraries (Arduino Library Manager): "Adafruit INA219", "Adafruit ADS1X15".
// Board: "ESP32 Dev Module" (esp32 by Espressif).
//
// Serial commands (115200 baud): r = reset latched fault, o = force load off,
// s = re-seed SOC from open-circuit voltage.

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>
#include <math.h>
#include "config.h"

// ---------------- Faults ----------------
enum Fault : uint16_t {
  FAULT_NONE   = 0,
  FAULT_OV     = 1 << 0,
  FAULT_UV     = 1 << 1,
  FAULT_OC     = 1 << 2,
  FAULT_OT     = 1 << 3,
  FAULT_UT     = 1 << 4,
  FAULT_SENSOR = 1 << 5,
  FAULT_MANUAL = 1 << 6,
};

// ---------------- State ----------------
Adafruit_INA219 ina219(INA219_ADDR);
Adafruit_ADS1115 ads;
WebServer server(80);

float tapV[NUM_CELLS];
float cellV[NUM_CELLS];
float cellTempC[NUM_CELLS];
float packV = 0, packA = 0;      // packA > 0 while discharging
float socPct = 0;
float mAhUsed = 0;
uint16_t faults = FAULT_NONE;     // Latched until reset
bool loadOn = false;
bool balancing[NUM_CELLS] = { false, false, false };

uint8_t ovCount = 0, uvCount = 0, ocCount = 0, otCount = 0;
unsigned long lastSampleMs = 0, lastSlowMs = 0;

const int BAL_PINS[NUM_CELLS] = { PIN_BAL_1, PIN_BAL_2, PIN_BAL_3 };
const int NTC_PINS[NUM_CELLS] = { PIN_NTC_1, PIN_NTC_2, PIN_NTC_3 };

// Approximate resting voltage -> SOC curve for NMC 18650 cells.
// Replace with a curve you measure on your own cells for better accuracy.
const float OCV_V[]   = { 3.00, 3.45, 3.55, 3.62, 3.68, 3.73, 3.79, 3.86, 3.93, 4.00, 4.08, 4.20 };
const float OCV_SOC[] = {    0,    5,   10,   20,   30,   40,   50,   60,   70,   80,   90,  100 };
const int OCV_POINTS = sizeof(OCV_V) / sizeof(OCV_V[0]);

// ---------------- Outputs ----------------
void setLoad(bool on) {
  loadOn = on && faults == FAULT_NONE;
  digitalWrite(PIN_DSG_FET, loadOn ? HIGH : LOW);
}

void setBalance(int cell, bool on) {
  balancing[cell] = on;
  digitalWrite(BAL_PINS[cell], on ? HIGH : LOW);
}

void allBalanceOff() {
  for (int i = 0; i < NUM_CELLS; i++) setBalance(i, false);
}

void tripFault(uint16_t f) {
  if ((faults & f) == 0) Serial.printf("# FAULT 0x%02X tripped\n", f);
  faults |= f;
  setLoad(false);
  allBalanceOff();
}

// ---------------- Measurements ----------------
float readThermistorC(int pin) {
  float mv = analogReadMilliVolts(pin);
  if (mv < 50 || mv > 3250) return NAN;  // Open or shorted thermistor
  float r = NTC_R_FIXED * mv / (3300.0f - mv);
  float invT = 1.0f / (NTC_T_NOMINAL_C + 273.15f) + logf(r / NTC_R_NOMINAL) / NTC_BETA;
  return 1.0f / invT - 273.15f;
}

bool readSensors() {
  // Tap n is the voltage of the top of cell n relative to pack negative.
  for (int i = 0; i < NUM_CELLS; i++) {
    int16_t raw = ads.readADC_SingleEnded(i);
    tapV[i] = ads.computeVolts(raw) / TAP_RATIO[i];
  }
  cellV[0] = tapV[0];
  for (int i = 1; i < NUM_CELLS; i++) cellV[i] = tapV[i] - tapV[i - 1];

  packV = ina219.getBusVoltage_V();
  packA = ina219.getCurrent_mA() / 1000.0f;

  bool ok = true;
  for (int i = 0; i < NUM_CELLS; i++) {
    // A disconnected tap or dead I2C bus shows up as an impossible cell voltage.
    if (cellV[i] < 0.5f || cellV[i] > 5.0f) ok = false;
    cellTempC[i] = NTC_ENABLED ? readThermistorC(NTC_PINS[i]) : 25.0f;
    if (isnan(cellTempC[i])) ok = false;
  }
  return ok;
}

// ---------------- Protection ----------------
// Counts consecutive bad samples so one noisy reading can't trip a fault.
bool debounce(bool condition, uint8_t &count, uint8_t limit) {
  count = condition ? min<uint8_t>(count + 1, limit) : 0;
  return count >= limit;
}

void checkProtection(bool sensorsOk) {
  if (!sensorsOk) { tripFault(FAULT_SENSOR); return; }

  float vMax = cellV[0], vMin = cellV[0], tMax = cellTempC[0], tMin = cellTempC[0];
  for (int i = 1; i < NUM_CELLS; i++) {
    vMax = max(vMax, cellV[i]); vMin = min(vMin, cellV[i]);
    tMax = max(tMax, cellTempC[i]); tMin = min(tMin, cellTempC[i]);
  }

  if (debounce(vMax > CELL_OV_V, ovCount, DEBOUNCE_OV)) tripFault(FAULT_OV);
  if (debounce(vMin < CELL_UV_V, uvCount, DEBOUNCE_UV)) tripFault(FAULT_UV);
  if (debounce(fabsf(packA) > PACK_OC_A, ocCount, DEBOUNCE_OC)) tripFault(FAULT_OC);
  if (debounce(tMax > TEMP_MAX_C, otCount, DEBOUNCE_OT)) tripFault(FAULT_OT);
  if (tMin < TEMP_MIN_C) tripFault(FAULT_UT);
}

// A latched fault only clears on request, and only once every condition has
// recovered past its hysteresis threshold. Auto-clearing would make the load
// chatter on and off, because cell voltage rebounds as soon as the load is cut.
bool resetFaults() {
  for (int i = 0; i < NUM_CELLS; i++) {
    if (cellV[i] > CELL_OV_CLEAR_V || cellV[i] < CELL_UV_CLEAR_V) return false;
    if (cellTempC[i] > TEMP_CLEAR_C || cellTempC[i] < TEMP_MIN_C) return false;
  }
  faults = FAULT_NONE;
  ovCount = uvCount = ocCount = otCount = 0;
  setLoad(true);
  Serial.println("# Faults cleared, load enabled");
  return true;
}

// ---------------- Balancing ----------------
void updateBalancing() {
  if (faults != FAULT_NONE) { allBalanceOff(); return; }

  float vMin = cellV[0], vMax = cellV[0];
  for (int i = 1; i < NUM_CELLS; i++) { vMin = min(vMin, cellV[i]); vMax = max(vMax, cellV[i]); }

  for (int i = 0; i < NUM_CELLS; i++) {
    float delta = cellV[i] - vMin;
    if (vMax < BAL_START_V) setBalance(i, false);
    else if (delta > BAL_DELTA_V) setBalance(i, true);
    else if (delta < BAL_STOP_DELTA_V) setBalance(i, false);
    // Between the two thresholds: keep the current state (hysteresis).
  }
}

// ---------------- State of charge ----------------
float socFromOcv(float v) {
  if (v <= OCV_V[0]) return 0;
  if (v >= OCV_V[OCV_POINTS - 1]) return 100;
  for (int i = 1; i < OCV_POINTS; i++) {
    if (v < OCV_V[i]) {
      float f = (v - OCV_V[i - 1]) / (OCV_V[i] - OCV_V[i - 1]);
      return OCV_SOC[i - 1] + f * (OCV_SOC[i] - OCV_SOC[i - 1]);
    }
  }
  return 100;
}

// Seed from the weakest cell's resting voltage: the pack is empty when it is.
void seedSocFromOcv() {
  float vMin = cellV[0];
  for (int i = 1; i < NUM_CELLS; i++) vMin = min(vMin, cellV[i]);
  socPct = socFromOcv(vMin);
  Serial.printf("# SOC seeded from OCV %.3f V -> %.1f %%\n", vMin, socPct);
}

// Coulomb counting: integrate current over time.
void updateSoc(float dtHours) {
  float mAh = packA * 1000.0f * dtHours;
  mAhUsed += mAh;
  socPct = constrain(socPct - 100.0f * mAh / CELL_CAPACITY_MAH, 0.0f, 100.0f);
}

// ---------------- Web dashboard ----------------
const char DASHBOARD_HTML[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1"><title>MiniBMS</title>
<style>body{font-family:sans-serif;margin:16px;background:#111;color:#eee}
.c{display:inline-block;margin:6px;padding:12px;border-radius:8px;background:#222;min-width:120px}
.v{font-size:1.6em}.bad{color:#f55}.ok{color:#5d5}button{padding:10px 16px;margin:6px}</style>
</head><body><h2>MiniBMS</h2><div id="s"></div><div id="cells"></div><div id="pack"></div>
<button onclick="fetch('/reset',{method:'POST'})">Reset fault</button>
<button onclick="fetch('/off',{method:'POST'})">Load off</button>
<script>
async function tick(){
 const d=await (await fetch('/api')).json();
 s.innerHTML=d.faults?'<b class=bad>FAULT 0x'+d.faults.toString(16)+'</b>':'<b class=ok>OK</b> load '+(d.load?'ON':'OFF');
 cells.innerHTML=d.cells.map((c,i)=>'<div class=c>Cell '+(i+1)+'<div class=v>'+c.v.toFixed(3)+' V</div>'
  +c.t.toFixed(1)+' &deg;C'+(c.bal?' &middot; balancing':'')+'</div>').join('');
 pack.innerHTML='<div class=c>Pack<div class=v>'+d.packV.toFixed(2)+' V</div></div>'
  +'<div class=c>Current<div class=v>'+d.packA.toFixed(3)+' A</div></div>'
  +'<div class=c>SOC<div class=v>'+d.soc.toFixed(1)+' %</div>'+d.mAh.toFixed(0)+' mAh used</div>';
}
setInterval(tick,1000);tick();
</script></body></html>)HTML";

void handleApi() {
  String json = "{\"faults\":" + String(faults) + ",\"load\":" + (loadOn ? "true" : "false") +
                ",\"packV\":" + String(packV, 3) + ",\"packA\":" + String(packA, 3) +
                ",\"soc\":" + String(socPct, 1) + ",\"mAh\":" + String(mAhUsed, 1) + ",\"cells\":[";
  for (int i = 0; i < NUM_CELLS; i++) {
    if (i) json += ",";
    json += "{\"v\":" + String(cellV[i], 3) + ",\"t\":" + String(cellTempC[i], 1) +
            ",\"bal\":" + (balancing[i] ? "true" : "false") + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void setupWeb() {
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  Serial.printf("# Dashboard: connect to WiFi '%s', open http://%s\n",
                WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
  server.on("/", [] { server.send_P(200, "text/html", DASHBOARD_HTML); });
  server.on("/api", handleApi);
  server.on("/reset", HTTP_POST, [] { server.send(resetFaults() ? 200 : 409, "text/plain", ""); });
  server.on("/off", HTTP_POST, [] { tripFault(FAULT_MANUAL); server.send(200, "text/plain", ""); });
  server.begin();
}

// ---------------- Serial ----------------
void handleSerial() {
  if (!Serial.available()) return;
  switch (Serial.read()) {
    case 'r': if (!resetFaults()) Serial.println("# Cannot reset: a condition is still out of range"); break;
    case 'o': tripFault(FAULT_MANUAL); break;
    case 's': seedSocFromOcv(); break;
  }
}

// CSV log line: paste the serial output into a spreadsheet to plot your tests.
void logCsv() {
  Serial.printf("%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.1f,%d,%d%d%d,0x%02X\n",
                millis(), cellV[0], cellV[1], cellV[2], packV, packA,
                cellTempC[0], cellTempC[1], cellTempC[2], socPct, loadOn,
                balancing[0], balancing[1], balancing[2], faults);
}

// ---------------- Main ----------------
void setup() {
  // Outputs first, so the load and balancers are off before anything else runs.
  pinMode(PIN_DSG_FET, OUTPUT);
  digitalWrite(PIN_DSG_FET, LOW);
  for (int i = 0; i < NUM_CELLS; i++) { pinMode(BAL_PINS[i], OUTPUT); digitalWrite(BAL_PINS[i], LOW); }
  pinMode(PIN_STATUS_LED, OUTPUT);

  Serial.begin(115200);
  delay(500);
  Serial.println("# MiniBMS starting");

  analogSetAttenuation(ADC_11db);  // Thermistor inputs span 0-3.3 V
  Wire.begin(PIN_SDA, PIN_SCL);

  bool hwOk = true;
  if (!ina219.begin()) { Serial.println("# INA219 not found"); hwOk = false; }
  if (!ads.begin(ADS1115_ADDR)) { Serial.println("# ADS1115 not found"); hwOk = false; }
  ads.setGain(GAIN_ONE);  // +/-4.096 V range; divider outputs stay below 3.1 V

  setupWeb();

  // The load is still off, so these readings are open-circuit voltages.
  bool sensorsOk = hwOk && readSensors();
  seedSocFromOcv();
  checkProtection(sensorsOk);
  setLoad(true);  // No-op if any fault tripped above
  Serial.println("# ms,c1,c2,c3,packV,packA,t1,t2,t3,soc,load,bal,faults");
  lastSampleMs = lastSlowMs = millis();
}

void loop() {
  unsigned long now = millis();
  server.handleClient();
  handleSerial();

  if (now - lastSampleMs >= 100) {
    float dtHours = (now - lastSampleMs) / 3600000.0f;
    lastSampleMs = now;
    checkProtection(readSensors());
    updateSoc(dtHours);
  }

  if (now - lastSlowMs >= 1000) {
    lastSlowMs = now;
    updateBalancing();
    logCsv();
  }

  // Status LED: solid when healthy, blinking on a fault.
  digitalWrite(PIN_STATUS_LED, faults ? ((now / 250) % 2) : HIGH);
}
