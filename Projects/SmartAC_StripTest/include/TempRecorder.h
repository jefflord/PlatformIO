#ifndef MY_TempRecorder
#define MY_TempRecorder

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

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Arduino_GFX_Library.h>

#define ONE_WIRE_BUS 19

// Forward Declaration
class MyIoTHelper;
class DisplayUpdater;

// Define a pair structure to hold data points
using DataPoint = std::pair<uint64_t, float>;

// Define a structure to hold each source's data
struct SourceData
{
    int64_t sourceId;
    std::vector<DataPoint> data;
};

using DataStorage = std::vector<SourceData>;

class TempRecorder
{
public:
    TempRecorder(MyIoTHelper *helper);
    //~TempHelper(); // Destructor

    // OneWire *oneWire;
    // DallasTemperature *sensors;
    MyIoTHelper *ioTHelper;
    void begin();

    void clearSource(String name);
    void clearSource(long long sourceId);
    void clearSource();

    void flushAllDatatoDB();
    int64_t flushDatatoDB(long long sourceId);
    String getStorageAsJson(long long sourceId);

    float temperatureC[3];

private:
    DataStorage storage;

    // Define a vector to hold multiple sources

    size_t recordTemp(String name, int64_t time, float temperatureC);
    size_t getRecordCount(String name);
    static void doTemp(void *p);
    unsigned long previousTempMillis = 0;      // Store the last time doTemp() was executed
    unsigned long previousTempFlushMillis = 0; // Store the last time flusht to Db
};

#endif