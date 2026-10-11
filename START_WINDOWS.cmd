@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows.ps1" %*
if errorlevel 1 (
  echo.
  echo Baslatma basarisiz. Yukaridaki hatayi ve README.md dosyasini kontrol edin.
)
pause
