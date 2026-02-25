/*
 * ============================================================
 *  3-PHASE POWER FACTOR MONITOR — ESP32 Firmware
 * ============================================================
 *  Reads 3x ZMPT101B (voltage) and 3x ACS712 (current) sensors
 *  for a 3-phase AC system (R, Y, B phases).
 *  Calculates per-phase and overall power factor via zero-crossing
 *  phase detection, sends data over WiFi via HTTP POST.
 *
 *  Sensors (6 total):
 *    Phase R: ZMPT101B → GPIO 34, ACS712 → GPIO 35
 *    Phase Y: ZMPT101B → GPIO 32, ACS712 → GPIO 33
 *    Phase B: ZMPT101B → GPIO 25, ACS712 → GPIO 26
 *
 *  Board: ESP32 (required — needs 6 analog pins)
 * ============================================================
 */

#include <HTTPClient.h>
#include <WiFi.h>


// ── WiFi credentials (CHANGE THESE) ────────────────────────
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ── Server URL (IP of PC running Flask app) ─────────────────
const char *SERVER_URL = "http://192.168.1.100:5000/api/data";
// Change 192.168.1.100 to your PC's local IP address

// ── Pin Definitions — 3 Phases ──────────────────────────────
//                        Phase R   Phase Y   Phase B
const int VOLTAGE_PINS[] = {34, 32, 25};
const int CURRENT_PINS[] = {35, 33, 26};
const char *PHASE_NAMES[] = {"R", "Y", "B"};
const int NUM_PHASES = 3;

// ── ADC Constants (ESP32) ───────────────────────────────────
#define ADC_MAX 4095.0
#define ADC_VREF 3.3

// ── Sensor Calibration ──────────────────────────────────────
// ZMPT101B: Adjust per your transformer ratio
const float VOLTAGE_CALIBRATION = 234.26;

// ACS712-30A: Sensitivity = 66 mV/A, offset = VCC/2
const float ACS712_SENSITIVITY = 0.066;
const float ACS712_OFFSET = ADC_VREF / 2.0;

// ── Sampling Parameters ─────────────────────────────────────
const int SAMPLES_PER_CYCLE = 200;
const float AC_FREQUENCY = 50.0; // Hz
const float CYCLE_PERIOD_US = (1.0 / AC_FREQUENCY) * 1000000.0;
const int NUM_CYCLES = 5;

// ── Threshold ───────────────────────────────────────────────
const float PF_THRESHOLD = 0.85;

// ── LED Pin ─────────────────────────────────────────────────
#define LED_PIN 2

// ── Timing ──────────────────────────────────────────────────
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 2000; // ms

// ── Per-phase results struct ────────────────────────────────
struct PhaseData {
  float voltageRms;
  float currentRms;
  float powerFactor;
  float realPower;
  float apparentPower;
  float reactivePower;
};

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // Full 0–3.3V range

  // Set all sensor pins as input
  for (int i = 0; i < NUM_PHASES; i++) {
    pinMode(VOLTAGE_PINS[i], INPUT);
    pinMode(CURRENT_PINS[i], INPUT);
  }

  // ── Connect to WiFi ─────────────────────────────────────
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // Quick LED flash to confirm
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(150);
      digitalWrite(LED_PIN, LOW);
      delay(150);
    }
  } else {
    Serial.println("\nWiFi connection FAILED. Will retry...");
  }
}

// ════════════════════════════════════════════════════════════
//  Measure one phase
// ════════════════════════════════════════════════════════════
PhaseData measurePhase(int voltagePin, int currentPin) {
  PhaseData result = {0, 0, 0, 0, 0, 0};

  float sumVSq = 0, sumISq = 0, sumVI = 0;
  int totalSamples = SAMPLES_PER_CYCLE * NUM_CYCLES;
  float prevV = 0, prevI = 0;

  unsigned long sampleStartTime = micros();
  float sampleInterval = (CYCLE_PERIOD_US * NUM_CYCLES) / totalSamples;

  for (int i = 0; i < totalSamples; i++) {
    unsigned long targetTime =
        sampleStartTime + (unsigned long)(i * sampleInterval);
    while (micros() < targetTime) { /* busy wait for precise timing */
    }

    // Read raw ADC values
    int rawV = analogRead(voltagePin);
    int rawI = analogRead(currentPin);

    // Convert to voltage at ADC pin
    float adcV = (rawV / ADC_MAX) * ADC_VREF;
    float adcI = (rawI / ADC_MAX) * ADC_VREF;

    // Center around zero (remove DC offset)
    float v = adcV - (ADC_VREF / 2.0);
    float current = (adcI - ACS712_OFFSET) / ACS712_SENSITIVITY;

    // RMS accumulation
    sumVSq += v * v;
    sumISq += current * current;
    sumVI += v * current;

    prevV = v;
    prevI = current;
  }

  // ── RMS values ──────────────────────────────────────────
  result.voltageRms = sqrt(sumVSq / totalSamples) * VOLTAGE_CALIBRATION;
  result.currentRms = sqrt(sumISq / totalSamples);

  // ── Power Factor ────────────────────────────────────────
  if (result.currentRms > 0.05) {
    float avgVI = sumVI / totalSamples;
    result.realPower = avgVI * VOLTAGE_CALIBRATION;
    result.apparentPower = result.voltageRms * result.currentRms;

    if (result.apparentPower > 0) {
      result.powerFactor = result.realPower / result.apparentPower;
      result.powerFactor = constrain(result.powerFactor, -1.0, 1.0);
      result.powerFactor = abs(result.powerFactor);
    }

    result.reactivePower =
        sqrt(max(0.0f, result.apparentPower * result.apparentPower -
                           result.realPower * result.realPower));
  }

  return result;
}

// ════════════════════════════════════════════════════════════
//  MAIN LOOP
// ════════════════════════════════════════════════════════════
void loop() {
  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
    return;
  }

  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL)
    return;
  lastSendTime = now;

  // ── Measure all 3 phases ────────────────────────────────
  PhaseData phases[NUM_PHASES];
  float totalRealPower = 0, totalApparentPower = 0;
  bool anyLowPF = false;

  Serial.println("\n══════════════════════════════════════════");
  Serial.println("  3-PHASE POWER FACTOR READINGS");
  Serial.println("══════════════════════════════════════════");

  for (int i = 0; i < NUM_PHASES; i++) {
    phases[i] = measurePhase(VOLTAGE_PINS[i], CURRENT_PINS[i]);

    totalRealPower += phases[i].realPower;
    totalApparentPower += phases[i].apparentPower;

    if (phases[i].powerFactor > 0.01 && phases[i].powerFactor < PF_THRESHOLD) {
      anyLowPF = true;
    }

    // Print per-phase data
    Serial.printf("  Phase %s: V=%.1fV  I=%.3fA  PF=%.3f  P=%.1fW",
                  PHASE_NAMES[i], phases[i].voltageRms, phases[i].currentRms,
                  phases[i].powerFactor, phases[i].realPower);
    if (phases[i].powerFactor > 0.01 && phases[i].powerFactor < PF_THRESHOLD) {
      Serial.print("  ⚠️ LOW!");
    }
    Serial.println();
  }

  // ── Overall power factor ──────────────────────────────-
  float overallPF = 0;
  float totalReactivePower = 0;
  if (totalApparentPower > 0) {
    overallPF = totalRealPower / totalApparentPower;
    overallPF = constrain(abs(overallPF), 0.0f, 1.0f);
    totalReactivePower =
        sqrt(max(0.0f, totalApparentPower * totalApparentPower -
                           totalRealPower * totalRealPower));
  }

  Serial.println("──────────────────────────────────────────");
  Serial.printf("  OVERALL:  PF=%.3f  P=%.1fW  S=%.1fVA  Q=%.1fVAR\n",
                overallPF, totalRealPower, totalApparentPower,
                totalReactivePower);

  // ── LED alert ───────────────────────────────────────────
  if (anyLowPF) {
    for (int j = 0; j < 3; j++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }

  // ── Send data to server ─────────────────────────────────
  sendToServer(phases, overallPF, totalRealPower, totalApparentPower,
               totalReactivePower);
}

// ════════════════════════════════════════════════════════════
//  HTTP POST — send 3-phase data as JSON
// ════════════════════════════════════════════════════════════
void sendToServer(PhaseData phases[], float overallPF, float totalReal,
                  float totalApparent, float totalReactive) {
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  // Build JSON with per-phase and overall data
  String json = "{";

  // Per-phase arrays
  json += "\"phase_r\":{";
  json += "\"voltage\":" + String(phases[0].voltageRms, 2) + ",";
  json += "\"current\":" + String(phases[0].currentRms, 3) + ",";
  json += "\"power_factor\":" + String(phases[0].powerFactor, 3) + ",";
  json += "\"real_power\":" + String(phases[0].realPower, 2) + ",";
  json += "\"apparent_power\":" + String(phases[0].apparentPower, 2) + ",";
  json += "\"reactive_power\":" + String(phases[0].reactivePower, 2);
  json += "},";

  json += "\"phase_y\":{";
  json += "\"voltage\":" + String(phases[1].voltageRms, 2) + ",";
  json += "\"current\":" + String(phases[1].currentRms, 3) + ",";
  json += "\"power_factor\":" + String(phases[1].powerFactor, 3) + ",";
  json += "\"real_power\":" + String(phases[1].realPower, 2) + ",";
  json += "\"apparent_power\":" + String(phases[1].apparentPower, 2) + ",";
  json += "\"reactive_power\":" + String(phases[1].reactivePower, 2);
  json += "},";

  json += "\"phase_b\":{";
  json += "\"voltage\":" + String(phases[2].voltageRms, 2) + ",";
  json += "\"current\":" + String(phases[2].currentRms, 3) + ",";
  json += "\"power_factor\":" + String(phases[2].powerFactor, 3) + ",";
  json += "\"real_power\":" + String(phases[2].realPower, 2) + ",";
  json += "\"apparent_power\":" + String(phases[2].apparentPower, 2) + ",";
  json += "\"reactive_power\":" + String(phases[2].reactivePower, 2);
  json += "},";

  // Overall totals
  json += "\"overall_pf\":" + String(overallPF, 3) + ",";
  json += "\"total_real_power\":" + String(totalReal, 2) + ",";
  json += "\"total_apparent_power\":" + String(totalApparent, 2) + ",";
  json += "\"total_reactive_power\":" + String(totalReactive, 2);
  json += "}";

  int httpCode = http.POST(json);

  if (httpCode > 0) {
    Serial.printf("  📡 Data sent! Server: %d\n", httpCode);
  } else {
    Serial.printf("  ❌ POST failed: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}
