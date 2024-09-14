#include "MyIoTHelper.h"
#include "DisplayUpdater.h"
#include "TempRecorder.h"

#ifndef PROJECT_SRC_DIR
#define PROJECT_SRC_DIR "PROJECT_SRC_DIR"
#endif

static void resetWifiTask(void *pvParameter)
{
    // Cast the parameter back to a MyIoTHelper* object
    MyIoTHelper *x = static_cast<MyIoTHelper *>(pvParameter);

    for (;;)
    {
        if (x->x_resetWifi)
        {
            safeSerial.println("killing wifi b");
            WiFi.disconnect(false);
            safeSerial.println("wifi killed b");
            x->x_resetWifi = false;
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
        resetWifiTask, // Function to run on the new thread
        "_resetWifi",  // Name of the task (for debugging)
        2048 * 4,      // Stack size (in bytes)
        this,          // Parameter passed to the task
        1,             // Priority (0-24, higher number means higher priority)
        NULL           // Handle to the task (not used here)
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
    // safeSerial.printf("elapsed %f, delay_duration %f, configUpdateSec %d \n", elapsed, delay_duration, configUpdateSec);
    return elapsed >= delay_duration;
}

int64_t MyIoTHelper::getSourceId(String name)
{

    std::string key = name.c_str();

    auto it = sourceIdCache.find(key);
    if (it != sourceIdCache.end())
    {
        // safeSerial.println("getSourceId cached!");
        return it->second; // Return cached result
    }
    else
    {
        // safeSerial.println("getSourceId NOT cached!");
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
    // safeSerial.println("internalUpdateConfig");
    // safeSerial.println(jsonPayload);

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin(wifi_ssid, wifi_password, NULL);
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
        safeSerial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return (int64_t)-2;
    }

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

        // safeSerial.println("getting header doc id!!!!!!");
        // safeSerial.println(val);

        char *end;
        int64_t number = strtoll(val.c_str(), &end, 10); // Base 10
        // safeSerial.println(number);

        sourceIdCache[key] = number;
        return sourceIdCache[key];
    }
}

void MyIoTHelper::updateConfig()
{

    safeSerial.println("updateConfig");

    if (!configHasBeenDownloaded)
    {
        preferences.begin("MyIoTHelper", false);
        auto body = preferences.getString("config", "");
        preferences.end();
        if (body != "")
        {
            // safeSerial.println("using saved config");
            parseConfig(body);
        }
        else
        {
            // safeSerial.println("No saved config");
        }
    }

    if (!configHasBeenDownloaded || MyIoTHelper::hasTimePassed())
    {

        safeSerial.println("Updating config");

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
            2048 * 4,               // Stack size (in bytes)
            params,                 // Parameter passed to the task
            1,                      // Priority (0-24, higher number means higher priority)
            NULL                    // Handle to the task (not used here)
        );
    }
    else
    {
        safeSerial.println("NOT updating config");
    }
}

void MyIoTHelper::chaos(const String &mode)
{

    if (mode.equals("wifi"))
    {

        resetWifi();
        // safeSerial.println("killing wifi");
        // WiFi.disconnect(false);
        // safeSerial.println("killing wifi done");
    }
}

void MyIoTHelper::internalUpdateConfig()
{

    safeSerial.println("internalUpdateConfig");

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
    // safeSerial.println("getConfig Payload:");
    // safeSerial.println(jsonPayload);

    String payload = "";

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin(wifi_ssid, wifi_password, NULL);
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
            safeSerial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return;
        }

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            safeSerial.printf("Failed to parse JSON: %s\n", error.c_str());
        }
        else
        {

            String body = doc["body"];

            // safeSerial.print("body:");
            // safeSerial.println(body);

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
        safeSerial.println("Wi-Fi not connected");
    }
}

void MyIoTHelper::parseConfig(const String &config)
{

    JsonDocument bodyDoc;
    DeserializationError error = deserializeJson(bodyDoc, config);

    if (error)
    {
        safeSerial.printf("Failed to parse Body JSON: %s\n", error.c_str());
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
            safeSerial.println("######## parseConfig ########");
            safeSerial.printf("\t pressDownHoldTime: %d\n", pressDownHoldTime);
            safeSerial.printf("\t tempReadIntevalSec: %d\n", tempReadIntevalSec);
            safeSerial.printf("\t sendToDb: %s\n", sendToDb ? "true" : "false");
            safeSerial.printf("\t testJsonBeforeSend: %s\n", pressDownHoldTime ? "true" : "false");
            safeSerial.printf("\t tempFlushIntevalSec: %d\n", tempFlushIntevalSec);
            safeSerial.printf("\t configUpdateSec: %d\n", configUpdateSec);
            safeSerial.printf("\t servoAngle: %d\n", servoAngle);
            safeSerial.printf("\t servoHomeAngle: %d\n", servoHomeAngle);
            safeSerial.println("######## parseConfig ########");
        }
    }
}

/*
void loop() {
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  safeSerial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  safeSerial.printf("This chip has %d cores\n", ESP.getChipCores());
  safeSerial.print("Chip ID: ");
  safeSerial.println(chipId);

  delay(3000);
}


*/

void MyIoTHelper::Setup()
{
    Serial.begin(115200);
    while (!Serial)
        continue;

    showStartReason();
    safeSerial.print("File: ");
    safeSerial.print(PROJECT_SRC_DIR);
    safeSerial.print(" (");
    safeSerial.print(__FILE__);
    safeSerial.println(")");
    safeSerial.print("Compiled on: ");
    safeSerial.print(__DATE__);
    safeSerial.print(" at ");
    safeSerial.println(__TIME__);
    safeSerial.print("MAC:");
    safeSerial.println(WiFi.macAddress());
}

// auto getTimeCount = 0;

int64_t MyIoTHelper::getTime()
{

    if (timeClient == NULL)
    {
        safeSerial.println("getTime: timeClient not set yet.");
        return 1726189566ll * 1000ll;
    }

    if (timeClientOk)
    {

        lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
        lastNTPCheckTimeMs = millis();
    }
    else
    {
        // if we have one, but it' bad, guess the time.

        timeGuesses++;
        lastNTPTime = lastNTPTime + (millis() - lastNTPCheckTimeMs);
        lastNTPCheckTimeMs = millis();
        auto testNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
        safeSerial.printf("time guess %d,  %lld vs %lld\n", timeGuesses, lastNTPTime, testNTPTime);
    }

    if (millis() - getTimeLastCheck() > 5 * 60 * 1000)
    {

        xTaskCreate(
            [](void *parameter)
            {
                MyIoTHelper *me = static_cast<MyIoTHelper *>(parameter);

                try
                {
                    me->timeClientOk = false;
                    if (WiFi.status() != WL_CONNECTED)
                    {
                        me->wiFiBegin(me->wifi_ssid, me->wifi_password, NULL);
                    }
                    else
                    {
                        delete me->timeClient;
                        WiFiUDP ntpUDP;
                        me->timeClient = new NTPClient(ntpUDP, "pool.ntp.org", -4 * 60 * 60, 60 * 60 * 1000);
                        me->timeClient->begin();
                        auto updated = me->timeClient->update();

                        if (updated)
                        {
                            me->timeClientOk = true;
                        }
                        // safeSerial.printf("getTime updated %s\n", updated ? "true" : "false");
                    }
                    me->setTimeLastCheck(millis());
                }
                catch (const std::exception &e)
                {
                    safeSerial.println("e.what()!!!");
                    safeSerial.println(e.what());
                }

                vTaskDelete(NULL);
            },              // Function to run on the new thread
            "getTimeChild", // Name of the task (for debugging)
            2048 * 2,       // Stack size (in bytes) // 8192
            this,           // Parameter passed to the task
            1,              // Priority (0-24, higher number means higher priority)
            NULL            // Handle to the task (not used here)
        );
    }

    return lastNTPTime;
}

String MyIoTHelper::getFormattedTime()
{

    // if (timeClient == NULL)
    // {
    //     safeSerial.println("getFormattedTime: timeClient not set yet.");
    //     return "12:12:12";
    //     // return 1726187296l * 1000l; // + (millis() / 1000);
    // }

    struct tm *timeinfo;
    char timeStringBuff[12];

    // safeSerial.println("start getTime");
    auto lastNTPTime = getTime() / 1000;
    // safeSerial.print("lastNTPTime:");
    // safeSerial.println(lastNTPTime);
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

wl_status_t MyIoTHelper::wiFiBegin(const String &ssid, const String &passphrase, DisplayUpdater *_displayUpdater)
{
    auto result = WiFi.begin(ssid, passphrase);

    if (_displayUpdater != NULL)
    {
        displayUpdater = _displayUpdater;
    }

    // flash
    DisplayParameters params2 = {-1, 500, false, epd_bitmap_icons8_wifi_13, 80, 0, NULL};
    if (displayUpdater != NULL)
    {
        displayUpdater->flashIcon(&params2);
    }

    wifi_ssid = ssid;
    wifi_password = passphrase;
    // auto result = WiFi.begin("","");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        safeSerial.print(".");
    }

    if (displayUpdater != NULL)
    {
        displayUpdater->showIcon(&params2);
    }

    safeSerial.print("\nConnected to Wi-Fi: ");
    safeSerial.println(WiFi.localIP().toString());

    WiFiUDP ntpUDP;

    if (timeClient != NULL)
    {
        delete timeClient;
    }

    // NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000 * 15); // Time offset in seconds and update interval
    timeClient = new NTPClient(ntpUDP, "pool.ntp.org", -4 * 60 * 60, 60 * 60 * 1000);
    timeClient->begin();

    try
    {
        auto updated = timeClient->update();

        if (updated)
        {
            timeClientOk = true;
        }
    }
    catch (const std::exception &e)
    {
        safeSerial.println(e.what());
    }

    safeSerial.print("Current time: ");
    safeSerial.println(timeClient->getFormattedTime());

    lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
    lastNTPReadMillis = millis();

    updateConfig();

    return result;
}

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

    safeSerial.print("Reset reason: ");
    long delayMs = 0;

    switch (reason)
    {
    case ESP_RST_POWERON:
        safeSerial.println("Power on reset");
        break;
    case ESP_RST_EXT:
        safeSerial.println("External reset");
        break;
    case ESP_RST_SW:
        safeSerial.println("Software reset");
        break;
    case ESP_RST_PANIC:
        safeSerial.println("Exception/panic reset");
        delayMs = 1000;
        break;
    case ESP_RST_INT_WDT:
        safeSerial.println("Interrupt watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_TASK_WDT:
        safeSerial.println("Task watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_WDT:
        safeSerial.println("Other watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_DEEPSLEEP:
        safeSerial.println("Deep sleep reset");
        break;
    case ESP_RST_BROWNOUT:
        safeSerial.println("Brownout reset");
        break;
    case ESP_RST_SDIO:
        safeSerial.println("SDIO reset");
        break;
    default:
        safeSerial.println("Unknown reset reason");
        break;
    }

    // delay(delayMs);
}
