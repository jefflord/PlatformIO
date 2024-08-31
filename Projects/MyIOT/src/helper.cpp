#include "helper.h"

#ifndef PROJECT_SRC_DIR
#define PROJECT_SRC_DIR "PROJECT_SRC_DIR"
#endif

MyIoTHelper::MyIoTHelper()
{
    // Constructor implementation
}

MyIoTHelper::~MyIoTHelper()
{
    // Destructor implementation
}

bool MyIoTHelper::hasTimePassed()
{

    if (!configHasBeenDownloaded)
    {
        return true;
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = now - lastConfigUpdateTime;
    std::chrono::duration<double, std::milli> delay_duration(configUpdateSec * 1000); // Convert delay to duration
    // Serial.printf("elapsed %f, delay_duration %f, configUpdateSec %d \n", elapsed, delay_duration, configUpdateSec);
    return elapsed >= delay_duration;
}

void MyIoTHelper::updateConfig(const String &name)
{

    Serial.println("updateConfig");

    if (MyIoTHelper::hasTimePassed())
    {
        Serial.println("NOT updating config");

        TaskParams *params = new TaskParams;
        params->obj = this;
        params->message = name;

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
    params->obj->internalUpdateConfig(params->message);
    vTaskDelete(NULL);
}

void MyIoTHelper::internalUpdateConfig(const String &name)
{

    Serial.println("internalUpdateConfig");
    lastConfigUpdateTime = std::chrono::steady_clock::now();
    configHasBeenDownloaded = true;

    auto url = "https://5p9y34b4f9.execute-api.us-east-2.amazonaws.com/test";
    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "getConfig";
    doc["queryStringParameters"]["configName"] = name;
    doc["httpMethod"] = "POST";

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    Serial.println("getConfig Payload:");
    Serial.println(jsonPayload);

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
            JsonDocument bodyDoc;
            String body = doc["body"];
            Serial.print("body:");
            Serial.println(body);

            error = deserializeJson(bodyDoc, body);

            if (error)
            {
                Serial.printf("Failed to parse Body JSON: %s\n", error.c_str());
            }
            else
            {
                pressDownHoldTime = bodyDoc["pressDownHoldTime"];
                configUpdateSec = bodyDoc["configUpdateSec"];
                Serial.printf("pressDownHoldTime2: %d\n", pressDownHoldTime);
            }
        }
    }
    else
    {
        Serial.println("Wi-Fi not connected");
    }
}

void mySetup()
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

    return result;
}

void getConfig(const String &name)
{

    auto url = "https://5p9y34b4f9.execute-api.us-east-2.amazonaws.com/test";
    JsonDocument doc;

    // Populate the JSON document
    doc["headers"]["Authorization"] = "letmein";
    doc["queryStringParameters"]["action"] = "getConfig";
    doc["queryStringParameters"]["configName"] = name;
    doc["httpMethod"] = "POST";

    // Serialize JSON to a String
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Print the JSON payload to the Serial Monitor
    Serial.println("getConfig Payload:");
    Serial.println(jsonPayload);

    String payload = "";
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(url);

        int httpCode = http.POST(jsonPayload);

        if (httpCode == HTTP_CODE_OK)
        {
            String response = http.getString();
            Serial.println("Response:");
            Serial.println(response);

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);
            int pressDownHoldTime = doc["pressDownHoldTime"];
        }
        else
        {
            Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Serial.println("Wi-Fi not connected");
    }
}