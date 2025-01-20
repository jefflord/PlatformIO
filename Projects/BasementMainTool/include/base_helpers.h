#ifndef BASE_HELPERS_H
#define BASE_HELPERS_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <LD2450.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_now.h>

#endif

// Structure to receive data
typedef struct struct_message
{
    char action[32] = "";
} struct_message;


struct GlobalState
{
    bool sensorInit;
    bool sensorOn;
    bool lastNowMessageReady;
    int currentSensor;
    int xBJT_PIN;
    struct_message lastNowMessage;
    String lastEspNowAction;
    GlobalState() : sensorInit(false), sensorOn(false), currentSensor(0) {}
};



class OTAStatus
{
public:
    bool ArduinoOTARunning;
    int otaLastPercent;

    OTAStatus() : ArduinoOTARunning(false), otaLastPercent(-1) {}
};

void wiFiBegin();
OTAStatus *MySetupOTA();
void mDNSSetup(void *p);
void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleGetHtml(AsyncWebServerRequest *request);
void setupServer();
bool sendEspNowAction(const char *action);
void registerEspNowPeer();