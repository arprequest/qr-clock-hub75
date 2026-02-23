#pragma once

// ── WiFi provisioning ─────────────────────────────────────────
// Credentials are stored in NVS by WiFiManager after first setup.
// To clear them and re-enter AP mode, visit http://<device-ip>/reset-wifi
#define AP_NAME  "QRClock-Setup"   // AP SSID shown when WiFi is not configured

// ── Time ──────────────────────────────────────────────────────
// UTC offset in seconds:  UTC+1 = 3600,  UTC-5 = -18000
#define TIMEZONE_OFFSET_SEC  -28800
#define DAYLIGHT_OFFSET_SEC  3600

// ── Display ───────────────────────────────────────────────────
#define DEFAULT_BRIGHTNESS  100   // 0-255, can be changed via web UI

// ── HUB75E pins (ESP32-WROOM-32D 30-pin DevKit) ───────────────
#define R1_PIN   25
#define G1_PIN   26
#define B1_PIN   27
#define R2_PIN   14
#define G2_PIN   12
#define B2_PIN   13
#define A_PIN    23
#define B_PIN    19
#define C_PIN     5
#define D_PIN    17
#define E_PIN    22   // moved from 18 — GPIO18 may conflict with I2S DMA
#define LAT_PIN   4
#define OE_PIN   15
#define CLK_PIN  16
