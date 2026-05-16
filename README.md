# Autoclicker

Windows x64 autoclicker project in `C:\Users\Philipp\Documents\asm-autoclicker`.

The modern runnable app is:

```bat
C:\Users\Philipp\Documents\asm-autoclicker\modern\AutoclickerModern.exe
```

The original MASM version is still kept in the project root as `autoclicker.asm`.

The modern UI uses standard Windows 11-themed controls through Common Controls v6 and is per-monitor DPI aware, so it scales correctly on 4K/high-DPI displays.
The app displays keybinds under the title and `© 2026 kantiDev` at the bottom.
The executable embeds a mouse icon from `modern\mouse.ico`.

## Current UI

- `Clicks per second`: numeric input and slider, clamped to `1..600`.
- `Left click` / `Right click`: selects the click button.
- `Start`: starts clicking after a 1 second cooldown.
- `Shift+F6`: starts clicking instantly, without a cooldown.
- `F6`: stops clicking globally while it is running.
- `Exit`: closes the app.
- Click `© 2026 kantiDev`: toggles light/dark mode.
- `Ctrl+Click` `© 2026 kantiDev`: opens `https://github.com/TheOriUHD/`.

## Run Modern App

```bat
C:\Users\Philipp\Documents\asm-autoclicker\modern\AutoclickerModern.exe
```

Press `Start`, then clicking begins after a 1 second cooldown. Press `Shift+F6` to start instantly. Every click happens at the cursor's current position at that moment, not a saved position. Press `F6` to stop.

## Build Modern App

Open an `x64 Native Tools Command Prompt for VS`, then run:

```bat
cd C:\Users\Philipp\Documents\asm-autoclicker\modern
build-modern.bat
```

The click backend uses the same working `mouse_event` behavior as the MASM version. Use only where automation is permitted.
