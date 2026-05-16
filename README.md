# Autoclicker

A small Windows autoclicker with a modern native desktop UI.

## Download

Download the latest `.exe` from the GitHub Releases page:

```text
https://github.com/TheOriUHD/asm-autoclicker/releases
```

For v1.0, download `AutoclickerModern.exe` and run it.

## Features

- Modern Windows-style UI with DPI-aware scaling.
- CPS number input and custom slider.
- CPS capped to `1..600`.
- Left-click and right-click modes.
- Start/Stop button.
- `Shift+F6` starts instantly.
- `F6` stops globally.
- Status text changes color by state.
- Click `© 2026 kantiDev` to toggle light/dark mode.
- `Ctrl+Click` `© 2026 kantiDev` opens the project author's GitHub profile.

## Usage

1. Start `AutoclickerModern.exe`.
2. Set the desired CPS with the input box or slider.
3. Choose `Left click` or `Right click`.
4. Press `Start` to start after a 1 second countdown, or press `Shift+F6` to start instantly.
5. Press `Stop` or `F6` to stop.

Clicks happen at the cursor's current position. The app does not lock to a saved coordinate.

## Build From Source

Requirements:

- Windows
- Visual Studio Build Tools with the C++ toolchain
- Windows SDK
- MASM tools if you also want to build the assembly-only version

Build the modern app:

```bat
cd modern
build-modern.bat
```

The output is:

```text
modern\AutoclickerModern.exe
```

Build the original MASM-only version:

```bat
build.bat
```

The output is:

```text
autoclicker.exe
```

## Project Layout

- `modern/AutoclickerModern.cpp`: modern native Win32 UI and clicker implementation.
- `modern/build-modern.bat`: build script for the modern app.
- `modern/mouse.ico`: embedded executable icon.
- `autoclicker.asm`: original MASM implementation.
- `build.bat`: build script for the MASM version.

## Notes

This tool uses Windows mouse input APIs for local automation. Use it only where automation is permitted.
