# ⚡ 3-Phase Power Factor Monitor

Real-time **3-phase** power factor monitoring using **ESP32** with wireless WiFi data transfer. Measures voltage and current on all 3 phases (R, Y, B), calculates per-phase and overall power factor, logs to Excel, and alerts when PF drops below threshold.

## 🏗️ System Overview

```
  ┌──── AC 3-Phase Supply ────┐
  │ Phase R   Phase Y   Phase B│
  └──┬──────────┬──────────┬──┘
     │          │          │
  ┌──▼──┐   ┌──▼──┐   ┌──▼──┐
  │V + I│   │V + I│   │V + I│     6 sensors total:
  │Sensor│   │Sensor│   │Sensor│     3x ZMPT101B (voltage)
  └──┬──┘   └──┬──┘   └──┬──┘     3x ACS712 (current)
     └─────────┼─────────┘
               ▼
        ┌─────────────┐
        │    ESP32     │
        │  (WiFi POST) │──── WiFi ────▶  Python Flask App
        └─────────────┘               │  ● Excel Logger
                                      │  ● Web Dashboard
                                      │  ● Notifications
```

## 📦 Hardware Required

| Qty | Component | Purpose |
|-----|-----------|---------|
| 1 | ESP32 Dev Board | Microcontroller (6 ADC pins needed) |
| 3 | ZMPT101B Voltage Sensor | AC voltage per phase |
| 3 | ACS712-30A Current Sensor | AC current per phase |
| 1 | Breadboard + Jumper Wires | Connections |

## 🔌 Circuit Connections (ESP32)

| Phase | Sensor | ESP32 Pin |
|-------|--------|-----------|
| **R** | ZMPT101B OUT | GPIO 34 |
| **R** | ACS712 OUT | GPIO 35 |
| **Y** | ZMPT101B OUT | GPIO 32 |
| **Y** | ACS712 OUT | GPIO 33 |
| **B** | ZMPT101B OUT | GPIO 25 |
| **B** | ACS712 OUT | GPIO 26 |
| All | ZMPT101B VCC | 3.3V |
| All | ACS712 VCC | 5V (Vin) |
| All | GND | GND |

## 🚀 Setup

### 1. Upload Firmware
1. Open `arduino/power_factor_monitor.ino` in Arduino IDE
2. Install ESP32 board support
3. Edit WiFi credentials and server IP in the code
4. Upload to ESP32

### 2. Run Python Server
```bash
pip install -r requirements.txt
python app.py                 # Wait for ESP32 data
python app.py --simulate      # Test without hardware
```

### 3. Open Dashboard
Go to **http://localhost:5000**

## 📊 Features

| Feature | Description |
|---------|-------------|
| 3-Phase Monitoring | Independent V, I, PF for Phase R, Y, B |
| Wireless WiFi | ESP32 sends data over HTTP (no USB needed) |
| Excel Logging | Color-coded spreadsheet with all phase data |
| Notifications | Desktop alert when any phase PF < 0.85 |
| Web Dashboard | Live gauges, charts, stats for all 3 phases |
| Simulation Mode | Full testing without hardware |

## 📂 Project Structure

```
Mini_Project/
├── arduino/power_factor_monitor.ino   # ESP32 firmware (3-phase)
├── templates/index.html               # Dashboard (3-phase)
├── static/style.css                   # Styles
├── docs/
│   ├── circuit_diagram.md             # Wiring diagram
│   └── deploy_guide.md               # Full deployment guide
├── app.py                             # Flask server
├── wsgi.py                            # WSGI entry (production)
├── requirements.txt                   # Dependencies
├── Procfile                           # Cloud deployment
├── run.bat                            # Windows launcher
├── run_simulate.bat                   # Windows launcher (simulation)
├── .gitignore
└── README.md
```
