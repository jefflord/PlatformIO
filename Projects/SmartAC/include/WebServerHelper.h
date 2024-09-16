#ifndef MY_WebServerHelper
#define MY_WebServerHelper

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <chrono>
#include <chrono>
#include <Preferences.h>
#include <vector>
#include <utility> // For std::pair
#include <cstdint> // For uint64_t
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <unordered_map>
#include <time.h>
#include <bmp.h>
#include "ThreadSafeSerial.h"
#include <ESPAsyncWebServer.h>
#include <FS.h>     // File System library
#include <SPIFFS.h> // SPIFFS file system

#include <Arduino_GFX_Library.h>

#define ONE_WIRE_BUS 19

// Forward Declaration
class MyIoTHelper;
class DisplayUpdater;

class WebServerHelper
{
public:
    WebServerHelper();

    void begin();

private:
    AsyncWebServer *server;

    static void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
};

#endif