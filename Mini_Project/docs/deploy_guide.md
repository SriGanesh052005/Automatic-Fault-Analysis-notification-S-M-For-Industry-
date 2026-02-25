# 🚀 Deployment Guide — 3-Phase Power Factor Monitor

## Option 1: Local Deployment (Windows)

### Prerequisites
- **Python 3.9+** — [Download](https://www.python.org/downloads/)
- **Arduino IDE 2.x** — [Download](https://www.arduino.cc/en/software) (for ESP32 firmware)

### Quick Start (No Hardware)
Double-click **`run_simulate.bat`** — this installs deps and starts the server with fake 3-phase data.

Dashboard opens at: **http://localhost:5000**

### Quick Start (With ESP32 Hardware)
1. Double-click **`run.bat`** — starts the server waiting for real sensor data
2. Upload firmware to ESP32 (see below)

### Manual Steps
```bash
# 1. Install dependencies
pip install -r requirements.txt

# 2. Run server (choose one)
python app.py                     # Normal mode — waits for ESP32
python app.py --simulate          # Simulation — fake data
python app.py --port 8080         # Custom port
python app.py --threshold 0.90    # Custom PF threshold
```

---

## Option 2: ESP32 Firmware Upload

### Prerequisites
- Arduino IDE with **ESP32 board support**
  - In Arduino IDE: `File → Preferences → Additional Board Manager URLs`, add:
    ```
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    ```
  - Then: `Tools → Board → Boards Manager → search "ESP32" → Install`

### Steps
1. Open `arduino/power_factor_monitor.ino` in Arduino IDE
2. **Edit WiFi credentials** (lines 24-25):
   ```cpp
   const char *WIFI_SSID = "YOUR_WIFI_SSID";
   const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```
3. **Edit server IP** (line 28) — set to your PC's local IP:
   ```cpp
   const char *SERVER_URL = "http://192.168.1.100:5000/api/data";
   ```
   > Find your PC IP: run `ipconfig` in Command Prompt, look for `IPv4 Address`
4. Select board: `Tools → Board → ESP32 Dev Module`
5. Select port: `Tools → Port → COMx`
6. Click **Upload**

### Circuit Wiring
See `docs/circuit_diagram.md` for complete wiring details.

| Phase | Voltage Sensor (ZMPT101B) | Current Sensor (ACS712) |
|-------|--------------------------|------------------------|
| R     | GPIO 34                  | GPIO 35                |
| Y     | GPIO 32                  | GPIO 33                |
| B     | GPIO 25                  | GPIO 26                |

---

## Option 3: Cloud Deployment (Render / Railway)

> [!WARNING]
> Cloud deployment will NOT have desktop notifications (plyer) or Excel file persistence.
> Best suited for dashboard-only viewing.

### Deploy to Render (Free Tier)
1. Push project to a **GitHub repository**
2. Go to [render.com](https://render.com) → **New** → **Web Service**
3. Connect your GitHub repo
4. Settings:
   - **Build Command**: `pip install -r requirements.txt`
   - **Start Command**: `gunicorn wsgi:app --bind 0.0.0.0:$PORT`
5. Click **Deploy**

### Deploy to Railway
1. Push to GitHub
2. Go to [railway.app](https://railway.app) → **New Project** → **Deploy from GitHub**
3. Railway automatically detects the `Procfile` and deploys

---

## Troubleshooting

| Issue | Solution |
|-------|---------|
| `ModuleNotFoundError: No module named 'flask'` | Run `pip install -r requirements.txt` |
| ESP32 won't connect to WiFi | Double-check SSID/password in `.ino` file |
| Dashboard shows "Disconnected" | Ensure ESP32 and PC are on the same WiFi network |
| `plyer` notification error | Run `pip install plyer` — optional, not critical |
| Port 5000 already in use | Use `python app.py --port 8080` instead |
