#include "helper.h"

#ifndef PROJECT_SRC_DIR
#define PROJECT_SRC_DIR "PROJECT_SRC_DIR"
#endif

bool x_resetWifi = false;
void _resetWifi(void *p)
{
    for (;;)
    {
        if (x_resetWifi)
        {
            Serial.println("killing wifi");
            WiFi.disconnect(false);
            x_resetWifi = false;
        }
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void MyIoTHelper::resetWifi()
{
    x_resetWifi = true;
}

MyIoTHelper::MyIoTHelper(const String &name)
{

    mutex = xSemaphoreCreateMutex();
    setTimeLastCheck(millis());
    configName = name;
    xTaskCreate(
        _resetWifi,             // Function to run on the new thread
        "internalUpdateConfig", // Name of the task (for debugging)
        4096,                   // Stack size (in bytes)
        NULL,                   // Parameter passed to the task
        1,                      // Priority (0-24, higher number means higher priority)
        NULL                    // Handle to the task (not used here)
    );
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

    for (auto &source : storage)
    {
        flushDatatoDB(source.sourceId);

        // Serial.println("RETURN!!!!");
        // break;
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
    // Serial.println(jsonPayload);

    // Serial.printf("flushDatatoDB! %d\n", sourceId);
    // Serial.print("jsonPayload:");
    // Serial.println(jsonPayload);

    if (!ioTHelper->sendToDb)
    {
        Serial.println("Skipping sendToDb");
        clearSource();
        return (int64_t)-99;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        ioTHelper->wiFiBegin(ioTHelper->wifi_ssid, ioTHelper->wifi_password);
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

        ioTHelper->parseConfig(config);

        // Serial.print("addTemps: ");
        // Serial.println(id);
        return id;
    }
}

size_t TempRecorder::getRecordCount(String name)
{

    auto sourceId = ioTHelper->getSourceId(name);

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

size_t TempRecorder::recordTemp(String name, int64_t time, float temperatureC)
{

    auto sourceId = ioTHelper->getSourceId(name);

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
            [](void *parameter)
            {
                TaskParams *params = static_cast<TaskParams *>(parameter);
                params->obj->internalUpdateConfig();
                vTaskDelete(NULL);
            },                      // Function to run on the new thread
            "internalUpdateConfig", // Name of the task (for debugging)
            4096,                   // Stack size (in bytes)
            params,                 // Parameter passed to the task
            1,                      // Priority (0-24, higher number means higher priority)
            NULL                    // Handle to the task (not used here)
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
        Serial.println("killing wifi done");
    }
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

    showStartReason();
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
    if (timeClient == NULL)
    {
        Serial.println("getTime: timeClient not set yet.");
        return 1726189566ll * 1000ll;
    }

    if (millis() - getTimeLastCheck() > 60000)
    {
        delete timeClient;
        WiFiUDP ntpUDP;
        timeClient = new NTPClient(ntpUDP, "pool.ntp.org", -4 * 60 * 60, 60000); // Time offset in seconds and update interval
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

String MyIoTHelper::getFormattedTime()
{

    // if (timeClient == NULL)
    // {
    //     Serial.println("getFormattedTime: timeClient not set yet.");
    //     return "12:12:12";
    //     // return 1726187296l * 1000l; // + (millis() / 1000);
    // }

    struct tm *timeinfo;
    char timeStringBuff[12];
    auto lastNTPTime = getTime() / 1000;
    // Serial.println(lastNTPTime);
    timeinfo = localtime((time_t *)&lastNTPTime);
    strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", timeinfo);

    return String(timeStringBuff);
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

    timeClient->update();
    lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
    lastNTPReadMillis = millis();

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

TempRecorder::TempRecorder(MyIoTHelper *_helper)
{
    ioTHelper = _helper;
}

DisplayUpdater::DisplayUpdater(MyIoTHelper *_helper, TempRecorder *_tempRecorder)
{
    mutex = xSemaphoreCreateMutex();
    ioTHelper = _helper;
    tempRecorder = _tempRecorder;
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

            // Serial.print("Sensor ");
            // Serial.printf("%d, %s", i, sensorId.c_str());
            // Serial.print(": ");
            // Serial.println(sensors.getTempC(deviceAddress));

            me->temperatureC[i] = sensors.getTempC(deviceAddress);
            auto time = ioTHelper->getTime();
            auto itemCount = me->recordTemp(sensorId, time, me->temperatureC[i]);

            Serial.printf("'%s': %d items, time: %lld, temp: %f\n", sensorId.c_str(), itemCount, time, me->temperatureC[i]);

            // Serial.printf("'%s': %d items, %s\n", sensorId.c_str(), itemCount, helper.getStorageAsJson(helper.getSourceId(sensorId)).c_str());
        }

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

        if (currentMillis - me->previousTempFlushMillis >= (ioTHelper->tempFlushIntevalSec * 1000))
        {

            me->previousTempFlushMillis = currentMillis;
            Serial.printf("FLUSH TIME %ld\n", ++flushCount);

            for (int i = 0; i < sensors.getDeviceCount(); i++)
            {
                DeviceAddress deviceAddress;
                sensors.getAddress(deviceAddress, i);
                auto sensorId = ioTHelper->formatDeviceAddress(deviceAddress);
                auto itemCount = me->getRecordCount(sensorId);
                Serial.printf("'%s': %d items\n", sensorId.c_str(), itemCount);
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
        Serial.print("helper->getTime(): ");
        Serial.println(ioTHelper->_timeLastCheck);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Convert milliseconds to ticks
    }
}

// TaskParamsHolder params;

void DisplayUpdater::renderClickIcon(bool _showRenderClickIcon)
{

    showRenderClickIcon = _showRenderClickIcon;
    if (showRenderClickIcon)
    {
        xTaskCreate(DisplayUpdater::_renderClickIcon, "renderClickIcon", 2048, this, 1, NULL);
    }
}

void DisplayUpdater::_renderClickIcon(void *parameter)
{
    DisplayUpdater *me = static_cast<DisplayUpdater *>(parameter);
    auto gfx = me->gfx;

    while (me->showRenderClickIcon)
    {
        xSemaphoreTake(me->mutex, portMAX_DELAY);
        gfx->fillRect(0, 0, 13, 13, BLACK);
        gfx->drawXBitmap(0, 0, epd_bitmap_icons8_natural_user_interface_2_13, 13, 13, WHITE);
        xSemaphoreGive(me->mutex);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreTake(me->mutex, portMAX_DELAY);
        gfx->fillRect(0, 0, 13, 13, BLACK);
        xSemaphoreGive(me->mutex);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void DisplayUpdater::updateDisplay(void *parameter)
{
    DisplayUpdater *me = static_cast<DisplayUpdater *>(parameter);

    /**/
    auto animationShowing = false;
    auto forceUpdate = false;
    const int SET_CUR_TOP_Y = 16 - 16;
    const int FONT_SIZE = 2;
    /**/
    auto gfx = me->gfx;

    gfx->begin();

    gfx->setTextSize(FONT_SIZE);
    gfx->fillScreen(BLACK);

    long loopDelayMs = 1000;
    // long lastRun = 0;

    double lastT1 = 0;
    double lastT2 = 0;
    double lastT3 = 0;

    auto lastUpdateTimeMillis = millis();
    for (;;)
    {

        auto startTime = millis();
        if (animationShowing)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            forceUpdate = true;
            continue;
        }

        // sprintf(timeString, "%02d:%02d:%02d %s", hours, minutes, seconds, ampm.c_str());
        // sprintf(timeString, "%4.1f\u00B0C", temperature_1_C);

        // temperature_0_C = round(temperature_0_C * 10) / 10.0;
        // temperature_1_C = round(temperature_1_C * 10) / 10.0;
        // temperature_2_C = round(temperature_2_C * 10) / 10.0;

        auto temperature_0_C = me->tempRecorder->temperatureC[0];
        auto temperature_1_C = me->tempRecorder->temperatureC[1];
        auto temperature_2_C = me->tempRecorder->temperatureC[2];

        temperature_0_C = round(temperature_0_C);
        temperature_1_C = round(temperature_1_C);
        temperature_2_C = round(temperature_2_C);

        auto temperatureF1 = (temperature_0_C * (9.0 / 5.0)) + 32;
        auto temperatureF2 = (temperature_1_C * (9.0 / 5.0)) + 32;
        auto temperatureF3 = (temperature_2_C * (9.0 / 5.0)) + 32;

        auto timeSinceLast = millis() - lastUpdateTimeMillis;
        if (timeSinceLast > 10000 || forceUpdate || lastT1 != temperatureF1 || lastT2 != temperatureF2 || lastT3 != temperatureF3)
        {
            // Serial.println("Display update...");

            lastUpdateTimeMillis = millis();
            forceUpdate = false;
            lastT1 = temperatureF1;
            lastT2 = temperatureF2;
            lastT3 = temperatureF3;

            xSemaphoreTake(me->mutex, portMAX_DELAY);

            gfx->setTextColor(WHITE);

            gfx->fillRect(0, SET_CUR_TOP_Y + 16, 96, 64, BLACK);
            // char randomChar = (char)random(97, 127);
            gfx->setCursor(0, SET_CUR_TOP_Y + 16);

            // if (uploadingData)
            // {
            //   renderUploadIcon();
            // }

            gfx->println(me->ioTHelper->getFormattedTime());

            gfx->setCursor(gfx->getCursorX(), gfx->getCursorY() + 6);

            char bufferForNumber[20];

            gfx->setTextColor(BLUE);
            sprintf(bufferForNumber, "%2.0f", temperatureF1);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF1));
            gfx->setTextSize(FONT_SIZE);

            auto y = gfx->getCursorY();
            gfx->setCursor(gfx->getCursorX() + 12, y);
            gfx->setTextColor(RED);
            sprintf(bufferForNumber, "%2.0f", temperatureF2);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF2));
            gfx->setTextSize(FONT_SIZE);

            gfx->setCursor(gfx->getCursorX() + 12, y);
            gfx->setTextColor(GREEN);
            sprintf(bufferForNumber, "%2.0f", temperatureF3);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF3));
            gfx->setTextSize(FONT_SIZE);

            xSemaphoreGive(me->mutex);
        }

        vTaskDelay(loopDelayMs - (millis() - startTime) / portTICK_PERIOD_MS);
    }
}

void DisplayUpdater::begin()
{
    Arduino_DataBus *bus = new Arduino_HWSPI(OLED_DC, OLED_CS, OLED_SCL, OLED_SDA);
    gfx = new Arduino_SSD1331(bus, OLED_RES);

    xTaskCreate(
        updateDisplay,   // Function to run on the new thread
        "updateDisplay", // Name of the task (for debugging)
        8192 * 2,        // Stack size (in bytes) // 8192
        this,            // Parameter passed to the task
        1,               // Priority (0-24, higher number means higher priority)
        NULL             // Handle to the task (not used here)
    );
}

void TempRecorder::begin()
{

    // // params.sharedObj = ioTHelper;
    // params.sharedObj = new MyIoTHelper("SmartAC");

    // MyIoTHelper *p_ioTHelper = static_cast<MyIoTHelper *>(params.sharedObj);

    // Serial.print("1 ############################: ");
    // Serial.println(p_ioTHelper->_timeLastCheck);
    // delay(1000);

    // Serial.print("2 ############################: ");
    // Serial.println(ioTHelper->_timeLastCheck);
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