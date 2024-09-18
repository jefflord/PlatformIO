#ifndef MY_CLASS_H // Header guard
#define MY_CLASS_H

#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <chrono>
#include <Preferences.h>
#include <vector>
#include <utility> // For std::pair
#include <cstdint> // For uint64_t
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// #include <RTClib.h>
#include <unordered_map>
#include <time.h>
#include <bmp.h>
#include "ThreadSafeSerial.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Arduino_GFX_Library.h>

#define ONE_WIRE_BUS 19

#define OLED_CS 5
#define OLED_DC 21
#define OLED_RES 22
#define OLED_SDA 23
#define OLED_SCL 18



#include "esp_mac.h" // required - exposes esp_mac_type_t values

// // Define a pair structure to hold data points
// using DataPoint = std::pair<uint64_t, float>;

// // Define a structure to hold each source's data
// struct SourceData
// {
//     int64_t sourceId;
//     std::vector<DataPoint> data;
// };

// // Define a vector to hold multiple sources
// using DataStorage = std::vector<SourceData>;

void mySetup();
void showStartReason();
String getStartReason();

// wl_status_t wiFiBegin(const String &ssid, const String &passphrase);

void getConfig(const String &name);

// Forward Declaration
class DisplayUpdater;
// class ESP_WiFiManager;
struct DisplayParameters;

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

    void resetWifi();

    std::chrono::steady_clock::time_point lastConfigUpdateTime = std::chrono::steady_clock::time_point::min();

    String formatDeviceAddress(DeviceAddress deviceAddress);

    String getFormattedTime();
    int64_t getTime();
    long getUTCOffset();

    Preferences preferences;

    wl_status_t wiFiAutoConnect();

    void wiFiBegin();
    void setSafeBoot();

    wl_status_t ___wiFiBegin(const String &ssid, const String &passphrase);

    void SetDisplay(DisplayUpdater *_displayUpdater);
    String configName;

    int64_t getSourceId(String name);
    void regDevice();
    SemaphoreHandle_t mutex;

    int64_t _timeLastCheck = millis();
    void parseConfig(const String &config);

    const String url = "https://5p9y34b4f9.execute-api.us-east-2.amazonaws.com/test";

    String wifi_ssid = "";
    String wifi_password = "";

    DisplayUpdater *displayUpdater;

    bool x_resetWifi = false;

    std::timed_mutex getTimeRefreshMutex;

private:
    // WiFiManager *wifiManager;
    // ESP_WiFiManager *wifiManager;
    int timeGuesses = 0;
    // bool hasRtc = false;
    WiFiUDP ntpUDP;
    NTPClient *timeClient = NULL;

    bool safeBoot = false;
    
    bool timeClientOk = false;

    void setTimeLastCheck(int64_t value);
    int64_t getTimeLastCheck();

    // RTC_DS3231 rtc;

    std::unordered_map<std::string, int64_t> sourceIdCache;

    int64_t lastNTPCheckTimeMs = 0;
    int64_t lastNTPTime = 0;             // Time from the last NTP update
    unsigned long lastNTPReadMillis = 0; // millis() at the time of the last NTP update

    // Private members (accessible only within the class)
    bool configHasBeenDownloaded = false;

    void internalUpdateConfig();

    bool hasTimePassed();
};

struct TaskParams
{
    MyIoTHelper *obj;
    String message;
};

// class TempRecorder
// {
// public:
//     TempRecorder(MyIoTHelper *helper);
//     //~TempHelper(); // Destructor

//     // OneWire *oneWire;
//     // DallasTemperature *sensors;
//     MyIoTHelper *ioTHelper;
//     void begin();

//     void clearSource(String name);
//     void clearSource(long long sourceId);
//     void clearSource();

//     void flushAllDatatoDB();
//     int64_t flushDatatoDB(long long sourceId);
//     String getStorageAsJson(long long sourceId);

//     float temperatureC[3];

// private:
//     bool xhasRtc = false;

//     DataStorage storage;

//     size_t recordTemp(String name, int64_t time, float temperatureC);
//     size_t getRecordCount(String name);
//     static void doTemp(void *p);
//     unsigned long previousTempMillis = 0;      // Store the last time doTemp() was executed
//     unsigned long previousTempFlushMillis = 0; // Store the last time flusht to Db
// };

#endif