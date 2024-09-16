#include "TempRecorder.h"
#include "MyIoTHelper.h"
#include "DisplayUpdater.h"

void vDelay(unsigned long delayMs)
{
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}

TempRecorder::TempRecorder(MyIoTHelper *_helper)
{
    ioTHelper = _helper;
}

String TempRecorder::getStorageAsJson(long long sourceId)
{

    // Create a JSON document
    JsonDocument doc; // Adjust the size as needed

    // Create a JSON array to hold all sources
    JsonArray jsonArray = doc.to<JsonArray>();

    // Iterate through the storage and add each SourceData to the JSON array
    for (const auto &source : storage)
    {
        if (sourceId == source.sourceId)
        {
            JsonObject jsonObject = doc.to<JsonObject>();
            jsonObject["SourceId"] = source.sourceId;

            for (const auto &point : source.data)
            {
                JsonArray pointArray = jsonObject["data"].add<JsonArray>();
                pointArray.add(point.first);  // Timestamp
                pointArray.add(point.second); // Temperature
            }

            // Serialize the JSON document to a String
            String jsonString;
            serializeJson(doc, jsonString);

            return jsonString;
        }
    }

    return "";
}

void TempRecorder::clearSource()
{
    for (auto &source : storage)
    {
        source.data.clear();
    }
}

void TempRecorder::clearSource(long long sourceId)
{

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.clear();
        }
    }
}

void TempRecorder::clearSource(String name)
{

    auto sourceId = ioTHelper->getSourceId(name);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.clear();
        }
    }
}

void TempRecorder::flushAllDatatoDB()
{

    DisplayParameters params2 = {-1, 150, true, epd_bitmap_icons8_wifi_13, 80, 0, NULL};

    if (ioTHelper->displayUpdater != NULL)
    {
        ioTHelper->displayUpdater->flashIcon(&params2);
    }

    for (auto &source : storage)
    {

        flushDatatoDB(source.sourceId);
    }

    if (ioTHelper->displayUpdater != NULL)
    {
        ioTHelper->displayUpdater->showIcon(&params2);
    }
}

int64_t TempRecorder::flushDatatoDB(long long sourceId)
{

    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "addTemps";
    doc["queryStringParameters"]["configName"] = ioTHelper->configName;
    doc["httpMethod"] = "POST";
    doc["body"] = getStorageAsJson(sourceId);

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    // safeSerial.println(jsonPayload);

    // safeSerial.printf("flushDatatoDB! %d\n", sourceId);
    // safeSerial.print("jsonPayload:");
    // safeSerial.println(jsonPayload);

    if (!ioTHelper->sendToDb)
    {
        safeSerial.println("Skipping sendToDb");
        clearSource();
        return (int64_t)-99;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        ioTHelper->wiFiBegin();
    }

    HTTPClient http;
    http.begin(ioTHelper->url);

    int httpCode = http.POST(jsonPayload);

    String response = "";
    if (httpCode == HTTP_CODE_OK)
    {
        response = http.getString();
        http.end();
    }
    else
    {
        safeSerial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return (int64_t)-2;
    }

    // safeSerial.println("response!!!!");
    // safeSerial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        safeSerial.printf("Failed to parse JSON: %s\n", error.c_str());
        return (int64_t)-1;
    }
    else
    {

        String val = doc["headers"]["Doc-Id"];
        String config = doc["headers"]["Config"];

        // safeSerial.println("getting header doc id!!!!!!");
        // safeSerial.println(val);

        char *end;
        auto id = strtoll(val.c_str(), &end, 10); // Base 10
        clearSource(sourceId);

        ioTHelper->parseConfig(config);

        // safeSerial.print("addTemps: ");
        // safeSerial.println(id);
        return id;
    }
}

size_t TempRecorder::getRecordCount(String name)
{

    auto sourceId = ioTHelper->getSourceId(name);

    // safeSerial.print("sourceId:");
    // safeSerial.println(sourceId);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            return source.data.size();
        }
    }

    return 0;
}

size_t TempRecorder::recordTemp(String name, int64_t time, float temperatureC)
{

    auto sourceId = ioTHelper->getSourceId(name);

    // safeSerial.print("sourceId:");
    // safeSerial.println(sourceId);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.push_back(DataPoint(time, temperatureC));
            return source.data.size();
        }
    }

    // safeSerial.println("Adding new newSource for this");
    SourceData newSource;
    // safeSerial.println("F");
    newSource.sourceId = sourceId;
    // safeSerial.println("G");
    newSource.data.push_back(DataPoint(time, temperatureC));
    // safeSerial.println("H");
    storage.push_back(newSource);
    // safeSerial.println("I");
    auto size = newSource.data.size();
    // safeSerial.println("J");
    // safeSerial.print(size);
    // safeSerial.println("K");

    return size;
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

            auto uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            // printf("Task 'doTemp' high-water mark: %u bytes\n", uxHighWaterMark);

            safeSerial.printf("FLUSH %ld at %s (high water %d)\n", flushCount, ioTHelper->getFormattedTime().c_str(), uxHighWaterMark);
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

// void doTempX(void *parameter)
// {

//     auto uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
//     printf("Task 'doTempX' high-water mark: %u bytes\n", uxHighWaterMark);

//     // TempHelper *me = static_cast<TempHelper *>(((TaskParamsHolder *)parameter)->sharedObj);
//     // MyIoTHelper *ioTHelper = static_cast<MyIoTHelper *>(((TaskParamsHolder *)parameter)->sharedObj);

//     // MyIoTHelper *ioTHelper = static_cast<MyIoTHelper *>(parameter);

//     TempRecorder *th = static_cast<TempRecorder *>(parameter);
//     MyIoTHelper *ioTHelper = th->ioTHelper;

//     while (true)
//     {
//         safeSerial.print("helper->getTime(): ");
//         safeSerial.println(ioTHelper->_timeLastCheck);
//         vTaskDelay(pdMS_TO_TICKS(1000)); // Convert milliseconds to ticks
//     }
// }

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
        1024 * 5, // Stack size (in bytes) // 8192
        this,     // Parameter passed to the task
        1,        // Priority (0-24, higher number means higher priority)
        NULL      // Handle to the task (not used here)
    );
}