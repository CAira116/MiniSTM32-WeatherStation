@echo off
cd /d "C:\Users\。\Desktop\MiniSTM32-WeatherStation"
"C:\Keil_v5\UV4\UV4.exe" -b WeatherStation.uvprojx -j0 -o build.log
echo BUILD EXIT CODE: %ERRORLEVEL%
type build.log
