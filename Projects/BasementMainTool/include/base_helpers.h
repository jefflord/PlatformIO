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
#include <NodeCode.h>
#include <ThreadSafeSerial.h>

#endif

#define struct_message_MAX_EXPECTED_DATA_SIZE 200 // approx
#define DATA_TYPE_NAME "bmt"
#define DATA_VERSION 0

// Structure to receive data
typedef struct struct_message
{
    uint8_t version = DATA_VERSION;
    char type[4] = DATA_TYPE_NAME;
    char action[64] = "";
    uint8_t update_MacAddress[6];
    uint8_t tokenHolder_MacAddress[6];
    int update_status;

} struct_message;

struct GlobalState
{
    NodeList nodes;
    bool sensorInit;
    bool sensorOn;
    bool lastNowMessageReady;
    int currentSensor;
    uint8_t macAddress[6];
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
bool sendEspNodeUpdate(Node *node);
bool sendEspNodeUpdate(Node *node, Node *nodeWithToken);
void registerEspNowPeer();
char *convertMacAddressToString(const uint8_t mac[6]);