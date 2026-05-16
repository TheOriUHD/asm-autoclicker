@echo off
setlocal

where ml64.exe >nul 2>nul
if errorlevel 1 (
  echo ml64.exe was not found. Run this from a "x64 Native Tools Command Prompt for VS".
  exit /b 1
)

ml64.exe /nologo /c /Foautoclicker.obj autoclicker.asm
if errorlevel 1 exit /b 1

link.exe /nologo /subsystem:windows /entry:main autoclicker.obj /out:autoclicker.exe
if errorlevel 1 exit /b 1

echo Built autoclicker.exe
