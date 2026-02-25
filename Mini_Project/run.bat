@echo off
echo ============================================================
echo   3-Phase Power Factor Monitor - Starting Server
echo ============================================================
echo.

:: Install dependencies
echo Installing dependencies...
pip install -r requirements.txt
echo.

:: Start the Flask server
echo Starting server on http://localhost:5000
echo Press Ctrl+C to stop.
echo.
python app.py --port 5000 --threshold 0.85

pause
