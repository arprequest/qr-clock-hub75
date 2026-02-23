#pragma once
#include <Arduino.h>
#include <time.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <qrcode.h>
#include "state.h"

extern MatrixPanel_I2S_DMA *matrix;

// ── Row-offset correction ────────────────────────────────────────
// Some FM6126A panels have a fixed 16-row vertical shift in their
// DMA mapping.  Compensate by adding 48 (= 64-16) to every Y coord.
static inline void px(int x, int y, uint16_t c) {
    matrix->drawPixel(x, (y + 32) & 63, c);
}

// ── HSV → color565 ──────────────────────────────────────────────
static uint16_t hsv565(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    if (s == 0) { r = g = b = v; }
    else {
        uint8_t region = h / 43;
        uint8_t remain = (h - region * 43) * 6;
        uint8_t p = (uint16_t)v * (255 - s) >> 8;
        uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * remain >> 8)) >> 8;
        uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - remain) >> 8)) >> 8;
        switch (region) {
            case 0: r=v; g=t; b=p; break;
            case 1: r=q; g=v; b=p; break;
            case 2: r=p; g=v; b=t; break;
            case 3: r=p; g=q; b=v; break;
            case 4: r=t; g=p; b=v; break;
            default: r=v; g=p; b=q; break;
        }
    }
    return matrix->color565(r, g, b);
}

// ── QR Clock ────────────────────────────────────────────────────
static int qrLastMin = -1;

static void drawQRClock() {
    struct tm ti{};
    if (!getLocalTime(&ti)) return;
    if (ti.tm_min == qrLastMin) return;
    qrLastMin = ti.tm_min;

    uint8_t buf[57];   // qrcode_getBufferSize(1) == 57
    QRCode qr;
    char ts[6];
    strftime(ts, sizeof(ts), "%H:%M", &ti);
    if (qrcode_initText(&qr, buf, 1, ECC_LOW, ts) != 0) return;

    const int scale = 2;
    const int off   = (64 - qr.size * scale) / 2;
    matrix->clearScreen();
    uint16_t white = matrix->color565(255, 255, 255);
    for (int y = 0; y < qr.size; y++)
        for (int x = 0; x < qr.size; x++)
            if (qrcode_getModule(&qr, x, y))
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++)
                        px(off + x*scale + dx, off + y*scale + dy, white);
}

// ── Solid ───────────────────────────────────────────────────────
static void drawSolid(const AppState &s) {
    matrix->fillScreen(matrix->color565(s.r, s.g, s.b));
}

// ── Rainbow ─────────────────────────────────────────────────────
// DIAGNOSTIC: 8 coloured bands of 8 rows each — raw drawPixel, no offset.
// Row 0-7=Red  8-15=Orange  16-23=Yellow  24-31=Green
// Row 32-39=Blue  40-47=Purple  48-55=Cyan  56-63=White
static void drawRainbow(const AppState &) {
    static bool done = false;
    if (done) return;
    done = true;
    const uint16_t bands[8] = {
        matrix->color565(255,   0,   0),  // 0-7   red
        matrix->color565(255, 128,   0),  // 8-15  orange
        matrix->color565(255, 255,   0),  // 16-23 yellow
        matrix->color565(  0, 255,   0),  // 24-31 green
        matrix->color565(  0,   0, 255),  // 32-39 blue
        matrix->color565(128,   0, 255),  // 40-47 purple
        matrix->color565(  0, 255, 255),  // 48-55 cyan
        matrix->color565(255, 255, 255),  // 56-63 white
    };
    matrix->clearScreen();
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            matrix->drawPixel(x, y, bands[y / 8]);
}

// ── Fire ────────────────────────────────────────────────────────
static uint8_t fireHeat[64][64];  // [row][col], row 0 = top
static bool    fireReady = false;

static void drawFire(const AppState &s) {
    if (!fireReady) { memset(fireHeat, 0, sizeof(fireHeat)); fireReady = true; }

    // seed bottom row
    for (int x = 0; x < 64; x++)
        fireHeat[63][x] = 200 + random(55);

    // diffuse upward — higher speed = less cooling = taller flames
    uint8_t cooling = (uint8_t)map(s.speed, 0, 255, 8, 1);
    for (int y = 0; y < 63; y++) {
        for (int x = 0; x < 64; x++) {
            uint16_t sum = (uint16_t)fireHeat[y+1][(x+63)%64]
                         + fireHeat[y+1][x]
                         + fireHeat[y+1][(x+1)%64]
                         + (y+2 < 64 ? fireHeat[y+2][x] : 200u);
            int16_t v = (int16_t)(sum / 4) - cooling;
            fireHeat[y][x] = (uint8_t)(v < 0 ? 0 : v);
        }
    }

    // palette: black → red → orange → yellow → white
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            uint8_t v = fireHeat[y][x];
            uint8_t r, g, b;
            if      (v < 64)  { r = v*3;          g = 0;           b = 0; }
            else if (v < 128) { r = 192+(v-64);   g = (v-64)*2;    b = 0; }
            else if (v < 192) { r = 255;           g = 128+(v-128); b = 0; }
            else              { r = 255;           g = 255;         b = (v-192)*4; }
            px(x, y, matrix->color565(r, g, b));
        }
    }
}

// ── Matrix Rain ─────────────────────────────────────────────────
struct RainDrop { int8_t head; uint8_t len; uint8_t timer; uint8_t spd; };
static RainDrop rainCols[64];
static uint8_t  rainBuf[64][64];
static bool     rainReady = false;

static void drawMatrixRain(const AppState &s) {
    if (!rainReady) {
        for (int x = 0; x < 64; x++) {
            rainCols[x] = { (int8_t)random(64), (uint8_t)(4 + random(20)),
                            (uint8_t)random(10), (uint8_t)(1 + random(4)) };
        }
        memset(rainBuf, 0, sizeof(rainBuf));
        rainReady = true;
    }

    // fade all pixels
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            rainBuf[y][x] = rainBuf[y][x] > 24 ? rainBuf[y][x] - 24 : 0;

    uint8_t step = 1 + (s.speed >> 6);
    for (int x = 0; x < 64; x++) {
        RainDrop &d = rainCols[x];
        if (d.timer > 0) { d.timer--; continue; }
        d.timer = d.spd;
        d.head  = (d.head + step) % 64;
        if (d.head >= 0) rainBuf[d.head][x] = 255;
        if (d.head >= 60) {
            d.head = -(int8_t)d.len;
            d.len  = 4 + random(20);
            d.spd  = 1 + random(4);
        }
    }

    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            px(x, y, matrix->color565(0, rainBuf[y][x], rainBuf[y][x] >> 2));
}

// ── Plasma ──────────────────────────────────────────────────────
static void drawPlasma(const AppState &s) {
    static float t = 0.0f;
    t += 0.03f + s.speed * 0.0002f;
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            float v = sinf(x * 0.15f + t)
                    + sinf(y * 0.15f + t * 1.3f)
                    + sinf((x + y) * 0.1f + t * 0.7f);
            uint8_t hue = (uint8_t)((v + 3.0f) * 42.5f);
            px(x, y, hsv565(hue, 255, s.brightness));
        }
    }
}

// ── Reset per-animation state on mode switch ────────────────────
inline void resetAnimState(Mode m) {
    switch (m) {
        case MODE_QR_CLOCK:    qrLastMin = -1;        break;
        case MODE_FIRE:        fireReady = false;      break;
        case MODE_MATRIX_RAIN: rainReady = false;      break;
        default: break;
    }
}
