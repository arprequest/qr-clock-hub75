#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include "state.h"

extern WiFiManager wm;

static AsyncWebServer httpServer(80);
static AsyncWebSocket ws("/ws");

static String stateJson() {
    StaticJsonDocument<128> doc;
    stateLock();
    doc["mode"]       = (int)gState.mode;
    doc["r"]          = gState.r;
    doc["g"]          = gState.g;
    doc["b"]          = gState.b;
    doc["speed"]      = gState.speed;
    doc["brightness"] = gState.brightness;
    stateUnlock();
    String out;
    serializeJson(doc, out);
    return out;
}

static void onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *client,
                      AwsEventType type, void *, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        client->text(stateJson());
    } else if (type == WS_EVT_DATA) {
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, data, len)) return;
        stateLock();
        if (doc.containsKey("mode"))       gState.mode       = (Mode)(int)doc["mode"];
        if (doc.containsKey("r"))          gState.r          = doc["r"];
        if (doc.containsKey("g"))          gState.g          = doc["g"];
        if (doc.containsKey("b"))          gState.b          = doc["b"];
        if (doc.containsKey("speed"))      gState.speed      = doc["speed"];
        if (doc.containsKey("brightness")) gState.brightness = doc["brightness"];
        stateUnlock();
        ws.textAll(stateJson());
    }
}

inline void webServerInit() {
    if (!LittleFS.begin(true)) { Serial.println("LittleFS mount failed"); return; }
    ws.onEvent(onWsEvent);
    httpServer.addHandler(&ws);
    httpServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    httpServer.on("/reset-wifi", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/plain", "WiFi cleared. Rebooting...");
        delay(500);
        wm.resetSettings();
        ESP.restart();
    });
    httpServer.begin();
    Serial.println("Web server ready");
}

inline void webServerTick() { ws.cleanupClients(); }
