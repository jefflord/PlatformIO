#include "helper.h"

#ifndef PROJECT_SRC_DIR
#define PROJECT_SRC_DIR "PROJECT_SRC_DIR"
#endif

MyIoTHelper::MyIoTHelper(const String &name)
{

    mutex = xSemaphoreCreateMutex();
    setTimeLastCheck(millis());
    configName = name;
}

MyIoTHelper::~MyIoTHelper()
{
    // Destructor implementation
}

bool MyIoTHelper::hasTimePassed()
{

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = now - lastConfigUpdateTime;
    std::chrono::duration<double, std::milli> delay_duration(configUpdateSec * 1000); // Convert delay to duration
    // Serial.printf("elapsed %f, delay_duration %f, configUpdateSec %d \n", elapsed, delay_duration, configUpdateSec);
    return elapsed >= delay_duration;
}

int64_t MyIoTHelper::getSourceId(String name)
{

    std::string key = name.c_str();

    auto it = sourceIdCache.find(key);
    if (it != sourceIdCache.end())
    {
        // Serial.println("getSourceId cached!");
        return it->second; // Return cached result
    }
    else
    {
        // Serial.println("getSourceId NOT cached!");
    }

    lastConfigUpdateTime = std::chrono::steady_clock::now();
    configHasBeenDownloaded = true;

    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "addSource";
    doc["queryStringParameters"]["MAC"] = WiFi.macAddress();
    doc["queryStringParameters"]["name"] = name;
    doc["httpMethod"] = "POST";

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    // Serial.println("internalUpdateConfig");
    // Serial.println(jsonPayload);

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin(wifi_ssid, wifi_password);
    }

    HTTPClient http;
    http.begin(url);

    int httpCode = http.POST(jsonPayload);

    String response = "";
    if (httpCode == HTTP_CODE_OK)
    {
        response = http.getString();
        http.end();
    }
    else
    {
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return (int64_t)-2;
    }

    // Serial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.printf("Failed to parse JSON: %s\n", error.c_str());
        return (int64_t)-1;
    }
    else
    {

        String val = doc["headers"]["Doc-Id"];

        // Serial.println("getting header doc id!!!!!!");
        // Serial.println(val);

        char *end;
        int64_t number = strtoll(val.c_str(), &end, 10); // Base 10
        // Serial.println(number);

        sourceIdCache[key] = number;
        return sourceIdCache[key];
    }
}

String MyIoTHelper::getStorageAsJson(long long sourceId)
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

void MyIoTHelper::clearSource()
{
    for (auto &source : storage)
    {
        source.data.clear();
    }
}

void MyIoTHelper::clearSource(long long sourceId)
{

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.clear();
        }
    }
}

void MyIoTHelper::clearSource(String name)
{

    auto sourceId = getSourceId(name);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.clear();
        }
    }
}

void MyIoTHelper::flushAllDatatoDB()
{

    for (auto &source : storage)
    {
        flushDatatoDB(source.sourceId);

        // Serial.println("RETURN!!!!");
        // break;
    }
}

int64_t MyIoTHelper::flushDatatoDB(long long sourceId)
{

    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "addTemps";
    doc["queryStringParameters"]["configName"] = configName;
    doc["httpMethod"] = "POST";
    doc["body"] = getStorageAsJson(sourceId);

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    // Serial.println(jsonPayload);

    if (testJsonBeforeSend)
    {
        DeserializationError error = deserializeJson(doc, jsonPayload);
        if (error)
        {
            Serial.printf("Failed to parse JSON: %s\n", error.c_str());
            return (int64_t)-1;
        }
    }

    // Serial.printf("flushDatatoDB! %d\n", sourceId);
    // Serial.print("jsonPayload:");
    // Serial.println(jsonPayload);

    if (!sendToDb)
    {
        Serial.println("Skipping sendToDb");
        clearSource();
        return (int64_t)-99;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin(wifi_ssid, wifi_password);
    }

    HTTPClient http;
    http.begin(url);

    int httpCode = http.POST(jsonPayload);

    String response = "";
    if (httpCode == HTTP_CODE_OK)
    {
        response = http.getString();
        http.end();
    }
    else
    {
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return (int64_t)-2;
    }

    // Serial.println("response!!!!");
    // Serial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.printf("Failed to parse JSON: %s\n", error.c_str());
        return (int64_t)-1;
    }
    else
    {

        String val = doc["headers"]["Doc-Id"];
        String config = doc["headers"]["Config"];

        // Serial.println("getting header doc id!!!!!!");
        // Serial.println(val);

        char *end;
        auto id = strtoll(val.c_str(), &end, 10); // Base 10
        clearSource(sourceId);

        parseConfig(config);

        // Serial.print("addTemps: ");
        // Serial.println(id);
        return id;
    }
}

size_t MyIoTHelper::getRecordCount(String name)
{

    auto sourceId = getSourceId(name);

    // Serial.print("sourceId:");
    // Serial.println(sourceId);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            return source.data.size();
        }
    }

    return 0;
}

size_t MyIoTHelper::recordTemp(String name, int64_t time, float temperatureC)
{

    auto sourceId = getSourceId(name);

    // Serial.print("sourceId:");
    // Serial.println(sourceId);

    for (auto &source : storage)
    {
        if (source.sourceId == sourceId)
        {
            source.data.push_back(DataPoint(time, temperatureC));
            return source.data.size();
        }
    }

    // Serial.println("Adding new newSource for this");
    SourceData newSource;
    // Serial.println("F");
    newSource.sourceId = sourceId;
    // Serial.println("G");
    newSource.data.push_back(DataPoint(time, temperatureC));
    // Serial.println("H");
    storage.push_back(newSource);
    // Serial.println("I");
    auto size = newSource.data.size();
    // Serial.println("J");
    // Serial.print(size);
    // Serial.println("K");

    return size;
}

void MyIoTHelper::updateConfig()
{

    Serial.println("updateConfig");

    if (!configHasBeenDownloaded)
    {
        preferences.begin("MyIoTHelper", false);
        auto body = preferences.getString("config", "");
        preferences.end();
        if (body != "")
        {
            // Serial.println("using saved config");
            parseConfig(body);
        }
        else
        {
            // Serial.println("No saved config");
        }
    }

    if (!configHasBeenDownloaded || MyIoTHelper::hasTimePassed())
    {
        Serial.println("Updating config");

        TaskParams *params = new TaskParams;
        params->obj = this;

        xTaskCreate(
            MyIoTHelper::TaskFunction, // Function to run on the new thread
            "internalUpdateConfig",    // Name of the task (for debugging)
            4096,                      // Stack size (in bytes)
            params,                    // Parameter passed to the task
            1,                         // Priority (0-24, higher number means higher priority)
            NULL                       // Handle to the task (not used here)
        );
    }
    else
    {
        Serial.println("NOT updating config");
    }
}

void MyIoTHelper::chaos(const String &mode)
{

    if (mode.equals("wifi"))
    {
        Serial.println("killing wifi");
        WiFi.disconnect(false);
    }
}

void MyIoTHelper::TaskFunction(void *parameter)
{
    TaskParams *params = static_cast<TaskParams *>(parameter);
    params->obj->internalUpdateConfig();
    vTaskDelete(NULL);
}

void MyIoTHelper::internalUpdateConfig()
{

    Serial.println("internalUpdateConfig");
    lastConfigUpdateTime = std::chrono::steady_clock::now();
    configHasBeenDownloaded = true;

    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "getConfig";
    doc["queryStringParameters"]["configName"] = configName;
    doc["httpMethod"] = "POST";

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    // Serial.println("getConfig Payload:");
    // Serial.println(jsonPayload);

    String payload = "";

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin(wifi_ssid, wifi_password);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(url);

        int httpCode = http.POST(jsonPayload);

        String response = "";
        if (httpCode == HTTP_CODE_OK)
        {
            response = http.getString();
            http.end();
        }
        else
        {
            Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return;
        }

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            Serial.printf("Failed to parse JSON: %s\n", error.c_str());
        }
        else
        {

            String body = doc["body"];

            // Serial.print("body:");
            // Serial.println(body);

            parseConfig(body);
            preferences.begin("MyIoTHelper", false);
            auto lastBody = preferences.getString("config", "");
            if (lastBody != body)
            {
                preferences.putString("config", body);
            }

            preferences.end();
        }
    }
    else
    {
        Serial.println("Wi-Fi not connected");
    }
}

void MyIoTHelper::parseConfig(const String &config)
{

    JsonDocument bodyDoc;
    DeserializationError error = deserializeJson(bodyDoc, config);

    if (error)
    {
        Serial.printf("Failed to parse Body JSON: %s\n", error.c_str());
    }
    else
    {
        pressDownHoldTime = bodyDoc["pressDownHoldTime"];
        tempReadIntevalSec = bodyDoc["tempReadIntevalSec"];
        sendToDb = bodyDoc["sendToDb"];
        testJsonBeforeSend = bodyDoc["testJsonBeforeSend"];
        tempFlushIntevalSec = bodyDoc["tempFlushIntevalSec"];
        configUpdateSec = bodyDoc["configUpdateSec"];
        servoAngle = bodyDoc["servoAngle"];
        servoHomeAngle = bodyDoc["servoHomeAngle"];

        if (false)
        {
            Serial.println("######## parseConfig ########");
            Serial.printf("\t pressDownHoldTime: %d\n", pressDownHoldTime);
            Serial.printf("\t tempReadIntevalSec: %d\n", tempReadIntevalSec);
            Serial.printf("\t sendToDb: %s\n", sendToDb ? "true" : "false");
            Serial.printf("\t testJsonBeforeSend: %s\n", pressDownHoldTime ? "true" : "false");
            Serial.printf("\t tempFlushIntevalSec: %d\n", tempFlushIntevalSec);
            Serial.printf("\t configUpdateSec: %d\n", configUpdateSec);
            Serial.printf("\t servoAngle: %d\n", servoAngle);
            Serial.printf("\t servoHomeAngle: %d\n", servoHomeAngle);
            Serial.println("######## parseConfig ########");
        }
    }
}

/*
void loop() {
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);

  delay(3000);
}


*/

void MyIoTHelper::Setup()
{
    Serial.begin(115200);
    while (!Serial)
        continue;

    Serial.print("File: ");
    Serial.print(PROJECT_SRC_DIR);
    Serial.print(" (");
    Serial.print(__FILE__);
    Serial.println(")");
    Serial.print("Compiled on: ");
    Serial.print(__DATE__);
    Serial.print(" at ");
    Serial.println(__TIME__);
    Serial.print("MAC:");
    Serial.println(WiFi.macAddress());
}

int64_t MyIoTHelper::getTime()
{

    Serial.println("A");
    auto xxx = millis();
    Serial.println(xxx);
    Serial.println("B");

    Serial.print("Running on core: ");
    Serial.println(xPortGetCoreID());

    Serial.println(getTimeLastCheck());

    Serial.println("C!!!!!!!!!!!!!!!!!!!");

    if (millis() - getTimeLastCheck() > 60000)
    {
        delete timeClient;
        WiFiUDP ntpUDP;
        timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60000); // Time offset in seconds and update interval
        timeClient->begin();
        auto updated = timeClient->update();
        // Serial.printf("update %s\n", updated ? "true" : "false");
        setTimeLastCheck(millis());
    }

    lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
    return ((int64_t)timeClient->getEpochTime() * 1000);

    // unsigned long currentMillis = millis();
    // int64_t elapsedMillis = currentMillis - lastNTPReadMillis;
    // return lastNTPTime + elapsedMillis;
}

int64_t MyIoTHelper::getTimeLastCheck()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    auto x = _timeLastCheck;
    xSemaphoreGive(mutex);
    return x;
}

void MyIoTHelper::setTimeLastCheck(int64_t value)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    _timeLastCheck = value;
    xSemaphoreGive(mutex);
}

wl_status_t MyIoTHelper::wiFiBegin(const String &ssid, const String &passphrase)
{
    auto result = WiFi.begin(ssid, passphrase);

    wifi_ssid = ssid;
    wifi_password = passphrase;
    // auto result = WiFi.begin("","");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.print("\nConnected to Wi-Fi: ");
    Serial.println(WiFi.localIP());

    WiFiUDP ntpUDP;
    // NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000 * 15); // Time offset in seconds and update interval
    timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60000);
    timeClient->begin();
    timeClient->update();
    Serial.print("Current time: ");
    Serial.println(timeClient->getFormattedTime());

    hasRtc = true;
    if (!rtc.begin())
    {
        hasRtc = false;
        Serial.println("Couldn't find RTC");
    }
    else
    {
        Serial.println("Found RTC");
    }

    if (hasRtc && rtc.lostPower())
    {
        Serial.println("RTC lost power, setting the time!");
        // Set the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    if (hasRtc)
    {
        // Set RTC with NTP time
        DateTime now = DateTime(timeClient->getEpochTime());
        rtc.adjust(now);
        Serial.println("RTC set to NTP time");
    }
    else
    {
        Serial.println("Manual time keeping");
        // Store the current NTP time and millis()
        timeClient->update();
        lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
        lastNTPReadMillis = millis();
    }

    updateConfig();

    return result;
}

// void getConfig(const String &name)
// {

//     auto url = "https://5p9y34b4f9.execute-api.us-east-2.amazonaws.com/test";
//     JsonDocument doc;

//     // Populate the JSON document
//     doc["headers"]["Authorization"] = "letmein";
//     doc["queryStringParameters"]["action"] = "getConfig";
//     doc["queryStringParameters"]["configName"] = name;
//     doc["httpMethod"] = "POST";

//     // Serialize JSON to a String
//     String jsonPayload;
//     serializeJson(doc, jsonPayload);

//     // Print the JSON payload to the Serial Monitor
//     Serial.println("getConfig Payload:");
//     Serial.println(jsonPayload);

//     String payload = "";
//     if (WiFi.status() == WL_CONNECTED)
//     {
//         HTTPClient http;
//         http.begin(url);

//         int httpCode = http.POST(jsonPayload);

//         if (httpCode == HTTP_CODE_OK)
//         {
//             String response = http.getString();
//             Serial.println("Response:");
//             Serial.println(response);

//             JsonDocument doc;
//             DeserializationError error = deserializeJson(doc, response);
//             int pressDownHoldTime = doc["pressDownHoldTime"];
//         }
//         else
//         {
//             Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
//         }
//         http.end();
//     }
//     else
//     {
//         Serial.println("Wi-Fi not connected");
//     }
// }

String MyIoTHelper::formatDeviceAddress(DeviceAddress deviceAddress)
{
    String address = "";
    for (int i = 0; i < 8; i++)
    {
        char hexByte[3];                            // Buffer to hold two hex digits + null terminator
        sprintf(hexByte, "%02X", deviceAddress[i]); // Format with zero padding
        address += hexByte;
        if (i < 7)
        {
            address += " ";
        }
    }
    return address;
}

void showStartReason()
{
    esp_reset_reason_t reason = esp_reset_reason();

    Serial.print("Reset reason: ");
    long delayMs = 0;

    switch (reason)
    {
    case ESP_RST_POWERON:
        Serial.println("Power on reset");
        break;
    case ESP_RST_EXT:
        Serial.println("External reset");
        break;
    case ESP_RST_SW:
        Serial.println("Software reset");
        break;
    case ESP_RST_PANIC:
        Serial.println("Exception/panic reset");
        delayMs = 1000;
        break;
    case ESP_RST_INT_WDT:
        Serial.println("Interrupt watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_TASK_WDT:
        Serial.println("Task watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_WDT:
        Serial.println("Other watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_DEEPSLEEP:
        Serial.println("Deep sleep reset");
        break;
    case ESP_RST_BROWNOUT:
        Serial.println("Brownout reset");
        break;
    case ESP_RST_SDIO:
        Serial.println("SDIO reset");
        break;
    default:
        Serial.println("Unknown reset reason");
        break;
    }

    // delay(delayMs);
}

TempHelper::TempHelper(MyIoTHelper *_helper)
{
    ioTHelper = _helper;
}

void TempHelper::doTemp(void *parameter)
{

    TempHelper *thisTempHelper = (TempHelper *)parameter;

    OneWire oneWire(ONE_WIRE_BUS);
    thisTempHelper->sensors = new DallasTemperature(&oneWire);
    thisTempHelper->sensors->begin();

    auto helper = thisTempHelper->ioTHelper;
    auto sensors = thisTempHelper->sensors;

    long flushCount = 0;
    while (true)
    {
        // Serial.println("_doTemp!!!");
        auto currentMillis = millis();
        // Serial.print("doTemp() ");
        Serial.println("111");
        sensors->requestTemperatures();

        Serial.println("222");

        for (int i = 0; i < sensors->getDeviceCount(); i++)
        {
            Serial.println("222bbbb");
            DeviceAddress deviceAddress;
            sensors->getAddress(deviceAddress, i);
            Serial.println("ccccccccc");
            auto sensorId = helper->formatDeviceAddress(deviceAddress);

            Serial.println("444444444");
            // Serial.print("Sensor ");
            // Serial.printf("%d, %s", i, sensorId.c_str());
            // Serial.print(": ");
            // Serial.println(sensors.getTempC(deviceAddress));

            Serial.println("444444444xxxxxxxxxxxx");
            float temperatureC = sensors->getTempC(deviceAddress);
            Serial.println("444444444xxxxxxxxxxxx222");
            auto time = helper->getTime();
            Serial.println("444444444xxxxxxxxxxxx333");
            auto itemCount = helper->recordTemp(sensorId, time, temperatureC);
            Serial.println("444444444xxxxxxxxxxxx444");

            Serial.println("5555555555");

            Serial.printf("'%s': %d items, time: %lld, temp: %f\n", sensorId.c_str(), itemCount, time, temperatureC);

            // Serial.printf("'%s': %d items, %s\n", sensorId.c_str(), itemCount, helper.getStorageAsJson(helper.getSourceId(sensorId)).c_str());
        }

        Serial.println("33");

        // if (false)
        // {
        //   Serial.print(temperatureC);
        //   Serial.print(" -- ");
        //   Serial.print(temperatureF);
        //   Serial.print(" -- ");
        //   Serial.print(time);
        //   Serial.print(" -- ");
        //   Serial.print(itemCount);
        //   Serial.println();
        // }

        if (currentMillis - thisTempHelper->previousTempFlushMillis >= (helper->tempFlushIntevalSec * 1000))
        {

            thisTempHelper->previousTempFlushMillis = currentMillis;
            Serial.printf("FLUSH TIME %ld\n", ++flushCount);

            for (int i = 0; i < sensors->getDeviceCount(); i++)
            {
                DeviceAddress deviceAddress;
                sensors->getAddress(deviceAddress, i);
                auto sensorId = helper->formatDeviceAddress(deviceAddress);
                auto itemCount = helper->getRecordCount(sensorId);
                Serial.printf("'%s': %d items\n", sensorId.c_str(), itemCount);
            }

            helper->flushAllDatatoDB();
        }

        vTaskDelay(pdMS_TO_TICKS(helper->tempReadIntevalSec * 1000)); // Convert milliseconds to ticks
    }
}

void TempHelper::begin()
{

    xTaskCreate(
        doTemp,    // Function to run on the new thread
        "doTemp",  // Name of the task (for debugging)
        8192 * 8,  // Stack size (in bytes) // 8192
        ioTHelper, // Parameter passed to the task
        1,         // Priority (0-24, higher number means higher priority)
        NULL       // Handle to the task (not used here)
    );
}