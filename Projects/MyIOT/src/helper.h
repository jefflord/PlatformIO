#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <chrono>

void mySetup();

wl_status_t wiFiBegin(const String &ssid, const String &passphrase);

void getConfig(const String &name);

#ifndef MY_CLASS_H // Header guard
#define MY_CLASS_H

class MyIoTHelper
{
public:
    // Public members (accessible from anywhere)
    MyIoTHelper();
    ~MyIoTHelper(); // Destructor
    void updateConfig(const String &name);

    int pressDownHoldTime = 250;
    int configUpdateSec = 60;
    std::chrono::steady_clock::time_point lastConfigUpdateTime = std::chrono::steady_clock::time_point::min();

    static void TaskFunction(void *parameter);

private:
    // Private members (accessible only within the class)
    bool configHasBeenDownloaded = false;

    void internalUpdateConfig(const String &name);

    bool hasTimePassed();
};

struct TaskParams
{
    MyIoTHelper *obj;
    String message;
};

#endif