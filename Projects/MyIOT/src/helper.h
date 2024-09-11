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
#include <RTClib.h>
#include <unordered_map>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "esp_mac.h" // required - exposes esp_mac_type_t values




// Define a pair structure to hold data points
using DataPoint = std::pair<uint64_t, float>;

// Define a structure to hold each source's data
struct SourceData
{
    int64_t sourceId;
    std::vector<DataPoint> data;
};

// Define a vector to hold multiple sources
using DataStorage = std::vector<SourceData>;

void mySetup();

wl_status_t wiFiBegin(const String &ssid, const String &passphrase);

void getConfig(const String &name);

#ifndef MY_CLASS_H // Header guard
#define MY_CLASS_H

class MyIoTHelper
{
public:
    // Public members (accessible from anywhere)
    MyIoTHelper(const String &name);        
    ~MyIoTHelper(); // Destructor
    void updateConfig();
    void chaos(const String &mode);

    void Setup();

    /****/
    int servoHomeAngle = 90;
    int servoAngle = 150;
    int pressDownHoldTime = 250;
    int configUpdateSec = 60;
    int tempReadIntevalSec = 5;
    bool testJsonBeforeSend = false;
    bool sendToDb = true;
    int tempFlushIntevalSec = 30;

    /****/

    std::chrono::steady_clock::time_point lastConfigUpdateTime = std::chrono::steady_clock::time_point::min();

    String getStorageAsJson();
    int64_t flushDatatoDB();
    size_t recordTemp(String name, int64_t time, float temperatureC);

    void clearSource(String name);
    void clearSource();

    int64_t getTime();

    Preferences preferences;

    wl_status_t wiFiBegin(const String &ssid, const String &passphrase);

    String configName;

    static void TaskFunction(void *parameter);

private:
    bool hasRtc = false;
    WiFiUDP ntpUDP;
    RTC_DS3231 rtc;

    std::unordered_map<std::string, int64_t> sourceIdCache;

    int64_t lastNTPTime = 0;             // Time from the last NTP update
    unsigned long lastNTPReadMillis = 0; // millis() at the time of the last NTP update
    
    // Private members (accessible only within the class)
    bool configHasBeenDownloaded = false;

    int64_t getSourceId(String name);

    String wifi_ssid = "";
    String wifi_password = "";

    void internalUpdateConfig();

    void parseConfig(const String &config);
    bool hasTimePassed();

    DataStorage storage;
    NTPClient* timeClient;

    const String url = "https://5p9y34b4f9.execute-api.us-east-2.amazonaws.com/test";
};

struct TaskParams
{
    MyIoTHelper *obj;
    String message;
};

#endif