@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Servidor.ps1" iniciar
if errorlevel 1 pause
