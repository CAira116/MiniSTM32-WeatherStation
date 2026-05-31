@echo off
chcp 65001 > nul
cd /d "%~dp0"
"C:\Users\。\AppData\Local\Programs\Python\Python312\python.exe" "%~dp0tools\serial_monitor.py"
pause
