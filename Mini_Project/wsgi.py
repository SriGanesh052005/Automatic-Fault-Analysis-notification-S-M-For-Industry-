"""
WSGI entry point for production deployment.
Used by Gunicorn, Render, Railway, etc.
"""
from app import app, init_excel

# Initialise the Excel file on startup
init_excel()

if __name__ == '__main__':
    app.run()
