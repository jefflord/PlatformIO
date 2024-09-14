#ifndef MY_TempRecorder
#define MY_TempRecorder

#include <Arduino.h>
#include <vector>
#include "ThreadSafeSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "freertos/FreeRTOS.h" 

#define ONE_WIRE_BUS 19
// Forward Declaration
class MyIoTHelper;

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
    bool xhasRtc = false;

    DataStorage storage;

    // Define a vector to hold multiple sources

    size_t recordTemp(String name, int64_t time, float temperatureC);
    size_t getRecordCount(String name);
    static void doTemp(void *p);
    unsigned long previousTempMillis = 0;      // Store the last time doTemp() was executed
    unsigned long previousTempFlushMillis = 0; // Store the last time flusht to Db
};

TempRecorder::TempRecorder(MyIoTHelper *_helper)
{
    ioTHelper = _helper;
}

void TempRecorder::doTemp(void *parameter)
{

    TempRecorder *me = static_cast<TempRecorder *>(parameter);
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);
    sensors.begin();

    auto ioTHelper = me->ioTHelper;

    long flushCount = 0;
    while (true)
    {
        auto currentMillis = millis();
        sensors.requestTemperatures();

        for (int i = 0; i < sensors.getDeviceCount(); i++)
        {
            DeviceAddress deviceAddress;
            sensors.getAddress(deviceAddress, i);
            auto sensorId = ioTHelper->formatDeviceAddress(deviceAddress);

            // safeSerial.print("Sensor ");
            // safeSerial.printf("%d, %s", i, sensorId.c_str());
            // safeSerial.print(": ");
            // safeSerial.println(sensors.getTempC(deviceAddress));

            me->temperatureC[i] = sensors.getTempC(deviceAddress);
            auto time = ioTHelper->getTime();
            auto itemCount = me->recordTemp(sensorId, time, me->temperatureC[i]);

            // safeSerial.printf("'%s': %d items, time: %lld, temp: %f\n", sensorId.c_str(), itemCount, time, me->temperatureC[i]);

            // safeSerial.printf("'%s': %d items, %s\n", sensorId.c_str(), itemCount, helper.getStorageAsJson(helper.getSourceId(sensorId)).c_str());
        }

        // if (false)
        // {
        //   safeSerial.print(temperatureC);
        //   safeSerial.print(" -- ");
        //   safeSerial.print(temperatureF);
        //   safeSerial.print(" -- ");
        //   safeSerial.print(time);
        //   safeSerial.print(" -- ");
        //   safeSerial.print(itemCount);
        //   safeSerial.println();
        // }

        if (currentMillis - me->previousTempFlushMillis >= (ioTHelper->tempFlushIntevalSec * 1000))
        {

            flushCount++;
            me->previousTempFlushMillis = currentMillis;
            // safeSerial.println(flushCount);
            // safeSerial.print("FLUSH TIME");
            // safeSerial.println(++flushCount);
            //  safeSerial.printf("FLUSH TIME %ld\n", ++flushCount);
            safeSerial.printf("FLUSH TIME %ld\n", flushCount);
            // safeSerial.println(flushCount);

            for (int i = 0; i < sensors.getDeviceCount(); i++)
            {
                DeviceAddress deviceAddress;
                sensors.getAddress(deviceAddress, i);
                auto sensorId = ioTHelper->formatDeviceAddress(deviceAddress);
                auto itemCount = me->getRecordCount(sensorId);
                safeSerial.printf("'%s': %d items\n", sensorId.c_str(), itemCount);
            }

            me->flushAllDatatoDB();
        }

        vTaskDelay(pdMS_TO_TICKS(ioTHelper->tempReadIntevalSec * 1000)); // Convert milliseconds to ticks
    }
}

void doTempX(void *parameter)
{

    // TempHelper *me = static_cast<TempHelper *>(((TaskParamsHolder *)parameter)->sharedObj);
    // MyIoTHelper *ioTHelper = static_cast<MyIoTHelper *>(((TaskParamsHolder *)parameter)->sharedObj);

    // MyIoTHelper *ioTHelper = static_cast<MyIoTHelper *>(parameter);

    TempRecorder *th = static_cast<TempRecorder *>(parameter);
    MyIoTHelper *ioTHelper = th->ioTHelper;

    while (true)
    {
        safeSerial.print("helper->getTime(): ");
        safeSerial.println(ioTHelper->_timeLastCheck);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Convert milliseconds to ticks
    }
}

// TaskParamsHolder params;

void TempRecorder::begin()
{

    // // params.sharedObj = ioTHelper;
    // params.sharedObj = new MyIoTHelper("SmartAC");

    // MyIoTHelper *p_ioTHelper = static_cast<MyIoTHelper *>(params.sharedObj);

    // safeSerial.print("1 ############################: ");
    // safeSerial.println(p_ioTHelper->_timeLastCheck);
    // delay(1000);

    // safeSerial.print("2 ############################: ");
    // safeSerial.println(ioTHelper->_timeLastCheck);
    // delay(1000);

    xTaskCreate(
        doTemp,   // Function to run on the new thread
        "doTemp", // Name of the task (for debugging)
        8192 * 2, // Stack size (in bytes) // 8192
        this,     // Parameter passed to the task
        1,        // Priority (0-24, higher number means higher priority)
        NULL      // Handle to the task (not used here)
    );
}
#endif MY_TempRecorder