@echo off
setlocal

where cl.exe >nul 2>nul
if errorlevel 1 (
  echo cl.exe was not found. Run this from a "x64 Native Tools Command Prompt for VS".
  exit /b 1
)

rc.exe /nologo /fo AutoclickerModern.res AutoclickerModern.rc
if errorlevel 1 exit /b 1

cl.exe /nologo /std:c++20 /EHsc /O2 /DUNICODE /D_UNICODE AutoclickerModern.cpp AutoclickerModern.res /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:AutoclickerModern.manifest /OUT:AutoclickerModern.exe user32.lib gdi32.lib comctl32.lib dwmapi.lib uxtheme.lib shell32.lib
if errorlevel 1 exit /b 1

echo Built AutoclickerModern.exe
