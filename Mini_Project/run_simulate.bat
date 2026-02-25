@echo off
echo ============================================================
echo   3-Phase Power Factor Monitor - SIMULATION MODE
echo ============================================================
echo   (No ESP32 hardware needed - uses fake 3-phase data)
echo.

:: Install dependencies
echo Installing dependencies...
pip install -r requirements.txt
echo.

:: Start the Flask server with simulation
echo Starting server with simulated data on http://localhost:5000
echo Press Ctrl+C to stop.
echo.
python app.py --simulate --port 5000 --threshold 0.85

pause
