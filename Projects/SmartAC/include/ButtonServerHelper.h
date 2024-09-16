#ifndef MY_ButtonServerHelper
#define MY_ButtonServerHelper

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
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

#define TOUCH_PIN 33
#define LED_ONBOARD 2
#define SERVO_PIN 32

// Forward Declaration
class MyIoTHelper;
class DisplayUpdater;

class ButtonServerHelper
{
public:
    ButtonServerHelper();

    void begin(MyIoTHelper *iotHelper, DisplayUpdater *displayUpdater);
    static void pushServoButtonX(void *pvParameters);
    static void static_pushServoButton();
    static void pushServoButton();

    void updateLoop();

    Servo myServo;
    bool servoMoving = false;
    unsigned long lastTouch = millis();
    unsigned long lastPress = millis() * 2;
    bool okToGo = false;

    MyIoTHelper *iotHelper;
    DisplayUpdater *displayUpdater;

    static ButtonServerHelper *GetButtonServerHelper();

private:
    static ButtonServerHelper *global;
    int actuationCounter = 0;
    
};

#endif