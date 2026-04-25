<div align="center">
  <img src="app.ico" width="64" height="64" alt="OnAir icon">
  <h1>OnAir</h1>
  <p>A lightweight Windows system tray app that shows an animated OSD overlay when your microphone or camera is in use.</p>

  [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
  [![Platform: Windows](https://img.shields.io/badge/platform-Windows%2010%2B-0078d7.svg)](https://github.com/qingyashizi/onair/releases)
  [![Release](https://img.shields.io/github/v/release/qingyashizi/onair)](https://github.com/qingyashizi/onair/releases/latest)
  [![Size](https://img.shields.io/badge/size-~41%20KB-brightgreen)](https://github.com/qingyashizi/onair/releases/latest)

  [中文文档](README.zh-CN.md)
</div>

---

## What is OnAir?

OnAir is a minimal Windows system tray utility that monitors microphone and camera usage in real time. Whenever an app starts using your mic or camera, a smooth animated OSD (On-Screen Display) fades in at the bottom-right corner of your screen — showing exactly which app is listening or watching.

When the device is released, the OSD fades out gracefully. No menus to open, no panels to check — it just works in the background.

## Screenshots

| Dark Mode | Light Mode |
|-----------|------------|
| ![Dark mode — mic and camera active](dark%20two.png) | ![Light mode — mic and camera active](ligth%20two.png) |

## Features

- **Animated OSD overlay** — smooth fade in/out, rendered with GDI+ in the bottom-right corner
- **Shows app name(s)** — displays which app(s) are currently using your mic or camera
- **Simultaneous mic + camera** — side-by-side panel when both devices are active at once
- **Mic animation** — pulsing fill inside the microphone icon
- **Camera animation** — animated corner-frame bracket around the camera icon
- **Dark / Light theme** — follows Windows system theme, or set manually via the tray menu
- **Chinese / English UI** — follows system language, or set manually via the tray menu
- **Start with Windows** — one-click toggle in the tray menu (writes to HKCU Run key)
- **OSD preview mode** — right-click tray → Preview OSD, for quick visual testing
- **DPI-aware** — Per-Monitor DPI V2, looks crisp on HiDPI / 4K displays
- **~41 KB** — single `.exe`, no installer, no runtime dependencies

## Requirements

- Windows 10 or later (x64)
- Microphone/camera monitoring requires Windows 10 1903+ (CapabilityAccessManager registry)

## Installation

1. Download `onair.exe` from the [**latest release**](https://github.com/qingyashizi/onair/releases/latest)
2. Place it anywhere you like (e.g. `C:\Tools\onair.exe`)
3. Double-click to run — the **ON** icon appears in the system tray
4. Optional: right-click the tray icon → **Start with Windows** to enable auto-start

**To uninstall:** right-click the tray icon → **Exit OnAir**, then delete the `.exe` file. No registry keys are left behind (unless you enabled Start with Windows — disable it first via the tray menu).

## SmartScreen Warning

When you download and run `onair.exe` for the first time, Windows SmartScreen or your browser may show a security warning. This is expected behavior for newly published software and **does not mean the file is dangerous**.

**Why this happens:** Windows SmartScreen builds a reputation score for each signed executable based on cumulative download volume. A brand-new app — even one with a valid code signing certificate — starts with zero reputation and triggers the warning until enough downloads accumulate.

**The file is signed:** `onair.exe` is signed with an Authenticode code signing certificate issued by **Certum** (signer: *Fangmeng Liu*). You can verify the signature yourself:

```powershell
Get-AuthenticodeSignature -FilePath "path\to\onair.exe" | Select-Object Status, SignerCertificate
```

**The source code is fully open:** Every line of code is in this repository. You are welcome to review it before running, or build from source yourself.

**How to proceed past the warning:**

*Browser download warning (Chrome / Edge):*
1. Click the three-dot menu (⋮) next to the download → **Keep** → **Keep anyway**

*Windows SmartScreen popup ("Windows protected your PC"):*
1. Click **More info**
2. Click **Run anyway**

## Configuration

OnAir stores settings in `onair.ini`, located in the same directory as the `.exe`. The file is created automatically the first time you change a setting via the tray menu.

| Key | Value | Description |
|-----|-------|-------------|
| `ThemeMode` | `0` — Follow System *(default)* | Matches Windows dark/light mode |
| | `1` — Dark | Always use dark theme |
| | `2` — Light | Always use light theme |
| `LangMode` | `0` — Follow System *(default)* | Matches Windows display language |
| | `1` — Chinese | Always use Chinese UI |
| | `2` — English | Always use English UI |

Settings are saved instantly when changed via the tray menu. You can also edit `onair.ini` directly with any text editor.

## Build from Source

**Requirements:**
- CMake 3.10+
- MSVC (Visual Studio 2019+) **or** MinGW-w64
- No external libraries — only Windows built-in APIs (GDI+, Shell32, User32, Advapi32)

```bash
git clone https://github.com/qingyashizi/onair.git
cd onair
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The compiled binary will be at `build\Release\onair.exe` (MSVC) or `build\onair.exe` (MinGW).

## License

MIT — see [LICENSE](LICENSE)
