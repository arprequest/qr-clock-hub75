# QR Clock — ESP32 + 64×64 HUB75E LED Matrix

Displays the current time as a live QR code on a 64×64 RGB LED matrix panel.
Includes a WiFi setup portal and a browser-based control panel with animations.

![ESP32-WROOM-32D + HUB75E P2.5 64×64](https://img.shields.io/badge/ESP32-WROOM--32D-blue) ![PlatformIO](https://img.shields.io/badge/PlatformIO-6.x-orange) ![License: MIT](https://img.shields.io/badge/License-MIT-green)

## Features

- **QR Clock** — scans to reveal the current time (`HH:MM`), updates every minute
- **5 animations** — Rainbow, Fire, Matrix Rain, Plasma, Solid colour
- **Web UI** — mobile-friendly control panel (mode, colour, speed, brightness)
- **WiFi provisioning** — captive portal on first boot; scan QR code on matrix to join setup AP
- **Persistent settings** — WiFi credentials stored in NVS, survives power loss

## Hardware

| Part | Notes |
|---|---|
| ESP32-WROOM-32D (38-pin DevKit) | Any 38-pin DevKit works |
| 64×64 HUB75**E** P2.5 RGB LED matrix | Must be HUB75**E** for 64 rows |
| 5 V / 4 A power supply | Powers the matrix directly |
| 16-pin IDC ribbon cable | Usually included with the panel |

## Wiring

Open **`wiring.html`** in any browser for the full interactive wiring guide,
including pinout diagrams, connection table, and power supply diagram.

Quick reference — ESP32 → HUB75E:

| Signal | ESP32 GPIO | HUB75E Pin |
|--------|-----------|-----------|
| R1 | 25 | 1 |
| G1 | 26 | 2 |
| B1 | 27 | 3 |
| R2 | 14 | 5 |
| G2 | 12 | 6 |
| B2 | 13 | 7 |
| A  | 23 | 9 |
| B  | 19 | 10 |
| C  | 5  | 11 |
| D  | 17 | 12 |
| **E** | **18** | **16** |
| CLK | 16 | 13 |
| LAT | 4  | 14 |
| OE  | 15 | 15 |
| GND | GND | 4, 8 |

## Software Setup (Windows)

1. Install USB driver: **CP210x** (Silicon Labs) or **CH340** (WCH) depending on your board
2. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension
3. Open the `64x64/` folder in VS Code
4. Edit `include/config.h` — set your timezone offset
5. Flash the filesystem (web UI): **PlatformIO sidebar → Upload Filesystem Image**
   ```
   pio run --target uploadfs
   ```
6. Flash the firmware: click **→ Upload** or:
   ```
   pio run --target upload
   ```
7. Open the serial monitor — the device IP is printed on connect

## First Boot

The panel flashes **blue**, then shows a **QR code** for the setup WiFi network.
Scan it → captive portal opens → enter your WiFi credentials → device reboots.
Open `http://<device-ip>` in a browser for the control panel.

To change WiFi network: web UI → **Reset WiFi credentials**.

## Project Structure

```
64x64/
├── platformio.ini        # build config + library dependencies
├── include/
│   └── config.h          # pins, AP name, timezone
├── src/
│   ├── state.h           # shared AppState + FreeRTOS mutex
│   ├── animations.h      # all 6 animation functions
│   ├── web_server.h      # AsyncWebServer + WebSocket
│   └── main.cpp          # WiFiManager + matrix init + tasks
├── data/
│   └── index.html        # web UI (served from LittleFS)
└── wiring.html           # interactive wiring guide (open in browser)
```

## Architecture

- **Core 0** — WiFi stack + AsyncWebServer + WebSocket (default Arduino assignment)
- **Core 1** — matrix animation loop at ~30 fps (FreeRTOS task, priority 2)
- **Shared state** protected by a FreeRTOS mutex

## License

MIT
