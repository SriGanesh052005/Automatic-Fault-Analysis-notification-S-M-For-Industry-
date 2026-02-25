# Circuit Diagram — 3-Phase Power Factor Monitor

## ESP32 Wiring (6 Sensors)

```
  ┌─── 3-Phase AC Supply ───┐
  │  L1(R)   L2(Y)   L3(B)  │
  └──┬────────┬────────┬─────┘
     │        │        │
 ┌───┴───┐┌───┴───┐┌───┴───┐
 │ZMPT101B││ZMPT101B││ZMPT101B│   Voltage Sensors
 │Phase R ││Phase Y ││Phase B │
 │OUT→G34 ││OUT→G32 ││OUT→G25 │
 └───┬───┘└───┬───┘└───┬───┘
     │        │        │
 ┌───┴───┐┌───┴───┐┌───┴───┐
 │ACS712 ││ACS712 ││ACS712 │   Current Sensors
 │Phase R ││Phase Y ││Phase B │
 │OUT→G35 ││OUT→G33 ││OUT→G26 │
 └───┬───┘└───┬───┘└───┬───┘
     └────────┼────────┘
              ▼
     ┌────────────────┐
     │     ESP32      │
     │                │
     │  GPIO 34 ← V_R │
     │  GPIO 35 ← I_R │
     │  GPIO 32 ← V_Y │
     │  GPIO 33 ← I_Y │
     │  GPIO 25 ← V_B │
     │  GPIO 26 ← I_B │
     │  GPIO 2  = LED  │
     │                │
     │   WiFi ))) → PC │
     └────────────────┘
```

## Pin Mapping Table

| Phase | Measurement | Sensor | ESP32 GPIO | ADC Channel |
|-------|------------|--------|-----------|-------------|
| **R** | Voltage | ZMPT101B #1 | GPIO 34 | ADC1_CH6 |
| **R** | Current | ACS712 #1 | GPIO 35 | ADC1_CH7 |
| **Y** | Voltage | ZMPT101B #2 | GPIO 32 | ADC1_CH4 |
| **Y** | Current | ACS712 #2 | GPIO 33 | ADC1_CH5 |
| **B** | Voltage | ZMPT101B #3 | GPIO 25 | ADC2_CH8 |
| **B** | Current | ACS712 #3 | GPIO 26 | ADC2_CH9 |
| — | Alert LED | Onboard | GPIO 2 | — |

## Power Connections

| Pin | Connects To |
|-----|------------|
| ESP32 3V3 | All 3x ZMPT101B VCC |
| ESP32 Vin (5V) | All 3x ACS712 VCC |
| ESP32 GND | All sensor GND (common ground) |

## Sensor Specifications

### ZMPT101B (×3)
- Input: 0–250V AC
- Output: 0–3.3V analog (proportional)
- Isolation: Transformer-based

### ACS712-30A (×3)
- Range: 0–30A
- Sensitivity: 66 mV/A
- Output: VCC/2 at 0A ± 66mV per Amp
- Isolation: Hall-effect (non-contact)

## ⚠️ Safety Warning

> This project involves **AC mains voltage**. Work with proper insulation, use sensor modules with built-in isolation, and never touch live wires. The ZMPT101B and ACS712 provide electrical isolation between AC and the ESP32.
