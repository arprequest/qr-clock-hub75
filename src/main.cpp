#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <qrcode.h>
#include "config.h"
#include "state.h"
#include "animations.h"
#include "web_server.h"

// ── Globals ──────────────────────────────────────────────────────
MatrixPanel_I2S_DMA *matrix = nullptr;
AppState              gState;
SemaphoreHandle_t     gMutex;
WiFiManager           wm;

// ── Matrix init ──────────────────────────────────────────────────
static void matrixInit() {
    HUB75_I2S_CFG cfg(64, 64, 1);
    cfg.gpio.r1  = R1_PIN;  cfg.gpio.g1  = G1_PIN;  cfg.gpio.b1  = B1_PIN;
    cfg.gpio.r2  = R2_PIN;  cfg.gpio.g2  = G2_PIN;  cfg.gpio.b2  = B2_PIN;
    cfg.gpio.a   = A_PIN;   cfg.gpio.b   = B_PIN;   cfg.gpio.c   = C_PIN;
    cfg.gpio.d   = D_PIN;   cfg.gpio.e   = E_PIN;
    cfg.gpio.lat = LAT_PIN; cfg.gpio.oe  = OE_PIN;  cfg.gpio.clk = CLK_PIN;
    cfg.clkphase = false;
    // Many cheap 64×64 panels use an FM6126A shift-register that needs an
    // initialisation sequence.  If the display shows only a few rows or a
    // distorted image, uncomment the line below and reflash:
    cfg.driver = HUB75_I2S_CFG::FM6126A;
    matrix = new MatrixPanel_I2S_DMA(cfg);
    matrix->begin();
    matrix->clearScreen();
    matrix->setBrightness8(DEFAULT_BRIGHTNESS);
}

// ── Show WiFi-join QR code on the matrix ─────────────────────────
// Scanning this on iOS/Android automatically joins the AP,
// then the captive portal pops up for credential entry.
static void showAPQR(const char *apName) {
    char wifiStr[64];
    snprintf(wifiStr, sizeof(wifiStr), "WIFI:S:%s;T:nopass;;", apName);
    uint8_t buf[110];   // sufficient for QR version 3 (29×29)
    QRCode qr;
    if (qrcode_initText(&qr, buf, 3, ECC_LOW, wifiStr) != 0) return;
    const int scale = 2;
    const int ox    = (64 - qr.size * scale) / 2;
    const int oy    = (64 - qr.size * scale) / 2;
    matrix->clearScreen();
    uint16_t white = matrix->color565(255, 255, 255);
    for (int y = 0; y < qr.size; y++)
        for (int x = 0; x < qr.size; x++)
            if (qrcode_getModule(&qr, x, y))
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++)
                        matrix->drawPixel(ox + x*scale + dx, oy + y*scale + dy, white);
}

// ── Animation task — pinned to Core 1 ────────────────────────────
// Core 0 handles WiFi stack + web server (default Arduino assignment).
// Core 1 is dedicated to matrix rendering, keeping them from starving each other.
static void animTask(void *) {
    Mode prevMode = (Mode)0xFF;
    for (;;) {
        stateLock();
        AppState s = gState;
        stateUnlock();

        if (s.mode != prevMode) {
            resetAnimState(s.mode);
            prevMode = s.mode;
            matrix->clearScreen();
        }

        matrix->setBrightness8(s.brightness);

        switch (s.mode) {
            case MODE_QR_CLOCK:    drawQRClock();     break;
            case MODE_SOLID:       drawSolid(s);      break;
            case MODE_RAINBOW:     drawRainbow(s);    break;
            case MODE_FIRE:        drawFire(s);       break;
            case MODE_MATRIX_RAIN: drawMatrixRain(s); break;
            case MODE_PLASMA:      drawPlasma(s);     break;
        }

        vTaskDelay(pdMS_TO_TICKS(33));  // ~30 fps
    }
}

// ── setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== QR Clock booting ===");

    stateInit();
    gState.brightness = DEFAULT_BRIGHTNESS;
    matrixInit();

    // Brief blue flash — confirms matrix is alive before WiFi blocks
    matrix->fillScreen(matrix->color565(0, 0, 60));
    delay(300);
    matrix->clearScreen();

    // WiFiManager: tries saved credentials first.
    // On failure (or first boot) it starts AP mode and calls the callback below.
    wm.setAPCallback([](WiFiManager *) {
        Serial.printf("AP mode: connect to \"%s\"\n", AP_NAME);
        showAPQR(AP_NAME);
    });
    wm.setSaveConfigCallback([]() {
        matrix->clearScreen();
    });
    wm.setConfigPortalTimeout(0);  // wait indefinitely — power-cycle to retry

    if (!wm.autoConnect(AP_NAME)) {
        delay(1000);
        ESP.restart();
    }

    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());

    configTime(TIMEZONE_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
    struct tm ti{};
    for (int i = 0; i < 20 && !getLocalTime(&ti); i++) delay(500);

    webServerInit();
    Serial.printf("UI: http://%s\n", WiFi.localIP().toString().c_str());

    xTaskCreatePinnedToCore(animTask, "anim", 12288, nullptr, 2, nullptr, 1);
}

// ── loop — Core 0, web server housekeeping ────────────────────────
void loop() {
    webServerTick();
    delay(100);
}
