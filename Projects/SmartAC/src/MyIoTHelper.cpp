#ifndef PROJECT_SRC_DIR
#define PROJECT_SRC_DIR "PROJECT_SRC_DIR"
#endif
#include "MyIoTHelper.h"
#include "DisplayUpdater.h"
#include "TempRecorder.h"
#include <esp_ota_ops.h>
// #include <ESP_WiFiManager.h>

static void resetWifiTask(void *pvParameter)
{

    // Cast the parameter back to a MyIoTHelper* object
    MyIoTHelper *iotHelper = static_cast<MyIoTHelper *>(pvParameter);

    for (;;)
    {
        if (iotHelper->x_resetWifi)
        {

            auto uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (uxHighWaterMark < 800 || uxHighWaterMark > 1800)
            {
                printf("Task 'resetWifiTask' high-water mark: %u bytes\n", uxHighWaterMark);
            }

            safeSerial.println("killing wifi b");
            WiFi.disconnect(false);
            safeSerial.println("wifi killed b");
            iotHelper->x_resetWifi = false;
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

    // wifiManager = new ESP_WiFiManager();
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

String byteArrayToHexString(const uint8_t *byteArray, size_t length)
{
    String hexString = "";
    for (size_t i = 0; i < length; i++)
    {
        if (byteArray[i] < 16)
        {
            hexString += "0"; // Add leading zero for single digit hex values
        }
        hexString += String(byteArray[i], HEX);
    }
    return hexString;
}

void MyIoTHelper::regDevice()
{

    JsonDocument doc;

    const esp_app_desc_t *appDescription = esp_ota_get_app_description();

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "regDevice";
    doc["queryStringParameters"]["MAC"] = WiFi.macAddress();
    doc["queryStringParameters"]["IP"] = WiFi.localIP().toString();
    doc["queryStringParameters"]["FW"] = tskKERNEL_VERSION_NUMBER;

    doc["queryStringParameters"]["Version"] = String(appDescription->version);
    doc["queryStringParameters"]["BuildDate"] = String(appDescription->date);
    doc["queryStringParameters"]["BuildTime"] = String(appDescription->time);
    doc["queryStringParameters"]["StartReason"] = getStartReason();
    doc["queryStringParameters"]["BuildHash"] = byteArrayToHexString(appDescription->app_elf_sha256, sizeof(appDescription->app_elf_sha256));

    doc["httpMethod"] = "POST";

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    if (WiFi.status() != WL_CONNECTED)
    {
        wiFiBegin();
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
        return;
    }

    // safeSerial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        safeSerial.printf("Failed to parse JSON: %s\n", error.c_str());
        return;
    }
    else
    {
        return;
    }
}

int64_t MyIoTHelper::getSourceId(String name)
{

    regDevice();

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

    const esp_app_desc_t *appDescription = esp_ota_get_app_description();

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
        wiFiBegin();
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
                // safeSerial.println("before internalUpdateConfig");
                params->obj->internalUpdateConfig();
                // safeSerial.println("after internalUpdateConfig");

                auto uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                if (uxHighWaterMark < 800 || uxHighWaterMark > 1800)
                {
                    printf("Task 'internalUpdateConfig' high-water mark: %u bytes\n", uxHighWaterMark);
                }

                vTaskDelete(NULL);
            },                      // Function to run on the new thread
            "internalUpdateConfig", // Name of the task (for debugging)
            1024 * 4,               // Stack size (in bytes)
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

        // wifiManager->resetSettings();
        resetWifi();
        // safeSerial.println("killing wifi");
        // WiFi.disconnect(false);
        // safeSerial.println("killing wifi done");
    }
}

void MyIoTHelper::internalUpdateConfig()
{

    // safeSerial.println("internalUpdateConfig");

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
        wiFiBegin();
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
                        me->wiFiBegin();
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

                auto uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                printf("Task 'getTimeChild' high-water mark: %u bytes\n", uxHighWaterMark);

                vTaskDelete(NULL);
            },
            "getTimeChild",
            1024,
            this,
            1,
            NULL);
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

// void configModeCallback(WiFiManager *myWiFiManager)
// {
//     Serial.println("Entered config mode");
//     Serial.println(WiFi.softAPIP());
//     Serial.println(myWiFiManager->getConfigPortalSSID());
// }

// wl_status_t MyIoTHelper::wiFiAutoConnect()
// {

//     auto apName = ("ESP Setup" + WiFi.macAddress()).c_str();
//     // wifiManager->setAPCallback(configModeCallback);
//     bool res = wifiManager->autoConnect(apName); // anonymous ap
//     if (!res)
//     {
//         Serial.println("Failed to connect, restarting in 30 seconds.");
//         delay(30000);
//         ESP.restart();
//     }

//     return WiFi.status();
// }

void MyIoTHelper::SetDisplay(DisplayUpdater *_displayUpdater)
{
    displayUpdater = _displayUpdater;
}

void MyIoTHelper::setSafeBoot()
{
    safeBoot = true;
}

void MyIoTHelper::wiFiBegin()
{
    // Try to connect to the saved WiFi credentials

    if (safeBoot)
    {
        WiFi.begin("", "");
    }

    if (!safeBoot && WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin();
    }

    safeBoot = false;

    // Wait for connection
    int timeout = 20000; // Timeout after 10 seconds
    int elapsed = 0;
    Serial.println("\nStarting WiFi...");

    // flash
    DisplayParameters params2 = {-1, 500, false, epd_bitmap_icons8_wifi_13, 80, 0, NULL};
    if (displayUpdater != NULL)
    {
        displayUpdater->flashIcon(&params2);
    }

    while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
    {
        delay(500);
        Serial.print("O");
        elapsed += 500;
    }

    if (displayUpdater != NULL)
    {
        displayUpdater->hideIcon(&params2);
    }

    // If not connected, start SmartConfig
    if (WiFi.status() != WL_CONNECTED)
    {
        params2 = {-1, 100, false, epd_bitmap_icons8_wifi_13, 80, 0, NULL};
        if (displayUpdater != NULL)
        {
            displayUpdater->flashIcon(&params2);
        }

        Serial.println("\nStarting SmartConfig...");
        WiFi.beginSmartConfig();

        // Wait for SmartConfig to finish
        while (!WiFi.smartConfigDone())
        {
            delay(500);
            Serial.print("X");
        }

        Serial.println("\nSmartConfig done.");
        // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
    }
    else
    {
        // Serial.printf("Connected to saved WiFi: %s\n", WiFi.SSID().c_str());
    }

    safeSerial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    if (displayUpdater != NULL)
    {
        displayUpdater->showIcon(&params2);
    }

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
}

wl_status_t MyIoTHelper::___wiFiBegin(const String &ssid, const String &passphrase)
{

    auto result = WiFi.begin(ssid, passphrase);

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
    safeSerial.print("Reset reason: ");
    safeSerial.println(getStartReason());
}

String getStartReason()
{
    esp_reset_reason_t reason = esp_reset_reason();

    long delayMs = 0;

    switch (reason)
    {
    case ESP_RST_POWERON:
        return ("Power on reset");
        break;
    case ESP_RST_EXT:
        return ("External reset");
        break;
    case ESP_RST_SW:
        return ("Software reset");
        break;
    case ESP_RST_PANIC:
        return ("Exception/panic reset");
        delayMs = 1000;
        break;
    case ESP_RST_INT_WDT:
        return ("Interrupt watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_TASK_WDT:
        return ("Task watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_WDT:
        return ("Other watchdog reset");
        delayMs = 1000;
        break;
    case ESP_RST_DEEPSLEEP:
        return ("Deep sleep reset");
        break;
    case ESP_RST_BROWNOUT:
        return ("Brownout reset");
        break;
    case ESP_RST_SDIO:
        return ("SDIO reset");
        break;
    default:
        return ("Unknown reset reason");
        break;
    }

    // delay(delayMs);
}
