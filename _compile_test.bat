@echo off
cd /d "C:\Users\。\Desktop\MiniSTM32-WeatherStation"
"C:\Keil_v5\UV4\UV4.exe" -b TestBlink.uvprojx -j0 -o build_test.log
echo BUILD EXIT: %ERRORLEVEL%
type build_test.log
