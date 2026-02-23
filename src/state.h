#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum Mode : uint8_t {
    MODE_QR_CLOCK    = 0,
    MODE_SOLID       = 1,
    MODE_RAINBOW     = 2,
    MODE_FIRE        = 3,
    MODE_MATRIX_RAIN = 4,
    MODE_PLASMA      = 5,
};

struct AppState {
    Mode    mode       = MODE_QR_CLOCK;
    uint8_t r          = 0;
    uint8_t g          = 200;
    uint8_t b          = 255;
    uint8_t brightness = 100;
    uint8_t speed      = 128;
};

extern AppState          gState;
extern SemaphoreHandle_t gMutex;

inline void stateInit()   { gMutex = xSemaphoreCreateMutex(); }
inline void stateLock()   { xSemaphoreTake(gMutex, portMAX_DELAY); }
inline void stateUnlock() { xSemaphoreGive(gMutex); }
