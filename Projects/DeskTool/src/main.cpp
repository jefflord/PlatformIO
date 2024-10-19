
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <ESPmDNS.h>
#include <esp_now.h>

HardwareSerial RadarSerial(1);

#define LED_PIN 2
#define LED_PIN_OFF LOW
#define LED_PIN_ON HIGH
#define readSensorSerial false

#define TOUCH_PIN 12
#define LD2450C_PIN 23 // D23
#define BOOT_BUTTON_PIN 0

const bool doWiFi = true;
bool dnsReady = false;
NTPClient *timeClient;

void wiFiBegin();
void readSensor(void *p);
void sendToggleAction(String action);
bool sendEspNowAction(const char *action);

void mDNSSetup(void *p)
{
  auto waitTimeout = millis() + 100;
  // Serial.println("MDNS setup waiting start");
  while (waitTimeout > millis() || WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(10));
    // Serial.println("MDNS setup waiting!");
  }
  // Serial.println("MDNS setup waiting done");

  if (!MDNS.begin("desktool"))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  else
  {
    Serial.println("Device can be accessed at desktool.local");
    // vTaskDelay(pdMS_TO_TICKS(100));
    //  if (MDNS.addService("http", "tcp", 80))
    //  {
    //    Serial.println("addService 1 worked");
    //  }

    dnsReady = true;
    vTaskDelete(NULL);
  }
}

/************/

uint8_t rearTVLightEspAddr[] = {0x60, 0x01, 0x94, 0x75, 0x22, 0x25}; // rearTVLightEspAddr "mac": "60:01:94:75:22:25"

// Structure to receive data
typedef struct struct_message
{
  char action[4] = "";
} struct_message;

// Create a struct_message to hold incoming data
struct_message lastNowMessage;
bool lastNowMessageReady = false;

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.println("command recieved");
  memcpy(&lastNowMessage, incomingData, sizeof(lastNowMessage));
  lastNowMessageReady = true;
}

void SetupOTA();

bool ArduinoOTARunning = false;
/************/
void setup()
{
  Serial.begin(115200);

  // The default baud rate of the serial port is
  // 256000, with 1 stop bit and no parity bit.
  RadarSerial.begin(256000, SERIAL_8N1, 16, 17);

  while (!Serial)
    continue;

  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LD2450C_PIN, INPUT);

  digitalWrite(LED_PIN, LED_PIN_OFF);

  if (readSensorSerial)
  {
    xTaskCreate(readSensor, "readSensor", 1024 * 4, NULL, 1, NULL);
  }

  if (doWiFi)
  {
    wiFiBegin();

    xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, NULL, 1, NULL);

    SetupOTA();

    WiFi.mode(WIFI_STA);
    Serial.println("Setup with OTA done.");

    if (esp_now_init() != ESP_OK)
    {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    else
    {
      Serial.println("ESP-NOW ready");
      registerEspNowPeer();

      // Register callback
      esp_now_register_recv_cb(OnDataRecv);
    }
  }
}

void registerEspNowPeer()
{
  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, rearTVLightEspAddr, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer  
  WiFi.mode(WIFI_STA);
  delay(20);
  if (!esp_now_is_peer_exist(rearTVLightEspAddr))
  {
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      Serial.println("Failed to add peer:");

      for (int i = 0; i < 6; i++)
      {
        Serial.print(peerInfo.peer_addr[i], HEX); // Print each byte in hex
        if (i < 5)
          Serial.print(":"); // Add colon between bytes
      }
    }
  }
}

int otaLastPercent = -1;
void SetupOTA()
{
  ArduinoOTA.onStart([]()
                     {
                         String type;
                         if (ArduinoOTA.getCommand() == U_FLASH)
                         {
                           type = "program";
                         }
                         else
                         { // U_SPIFFS
                           type = "filesystem";
                         }

                         Serial.println("OTA Update Starting: " + type);

                         ArduinoOTARunning = true; });
  ArduinoOTA.onEnd([]()
                   { 
                      ArduinoOTARunning = false;
                      Serial.println("\nOTA Update Finished"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { if ((progress / (total / 100) != otaLastPercent)) {
                            otaLastPercent = (progress / (total / 100));
                            Serial.printf("Progress: %u%% (%u/%u)\n", otaLastPercent, progress, total);
                              } });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                        ArduinoOTARunning = false;
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        } });

  ArduinoOTA.begin();
}

int lastReading = -1;
int lastChangeMs = -1;
unsigned long long loopCounter = 0;

void readSensor(void *p)
{
  Serial.printf("readSensor starting\n");
  RadarSerial.setTimeout(10);

  uint8_t old_stationaryTargetEnergyValue, stationaryTargetEnergyValue = -99;
  uint8_t old_exerciseTargetEnergy, exerciseTargetEnergy = -99;
  uint8_t old_targetStatus, targetStatus = -99;

  int movementTargetDistance, distanceToStationaryTarget, detectionDistance = -99;
  int old_movementTargetDistance, old_distanceToStationaryTarget, old_detectionDistance = -99;

  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(1));
    // Serial.printf("RadarSerial.available() %d\n", RadarSerial.available());

    if (RadarSerial.available() > 0)
    {
      // Serial.println("read...");
      //   Serial.println(RadarSerial.read(), HEX);

      uint8_t buffer[1024];

      if (RadarSerial.read() == 0xF4)
      {
        if (RadarSerial.read() == 0xF3)
        {
          if (RadarSerial.read() == 0xF2)
          {
            if (RadarSerial.read() == 0xF1)
            {
              uint8_t lengthBuffer[2];
              int length = RadarSerial.readBytes(lengthBuffer, 2);
              if (length < 0)
              {
                Serial.printf("length < 0\n");
                continue;
              }
              // // Combine the two bytes into an int (big-endian)
              // int result1 = (lengthBuffer[0] << 8) | lengthBuffer[1];

              // // Combine the two bytes into an int (little-endian)
              // int result2 = (lengthBuffer[1] << 8) | lengthBuffer[0];

              uint16_t dataFrameLength = *(uint16_t *)lengthBuffer;

              uint8_t dataFrame[dataFrameLength];
              length = RadarSerial.readBytes(dataFrame, dataFrameLength);
              if (length < 0)
              {
                Serial.printf("length < 0\n");
                continue;
              }

              // Serial.printf("\nGOT ONE dataFrameLength:%d, read: %d \n", dataFrameLength, length);

              if (dataFrame[0] == 0x02) // Target basic information data
              {
                // for (int x = 2; x < length - 2; x++) // skip type and head and tail
                // {
                //   Serial.printf("%X ", dataFrame[x]);
                // }
                // Serial.printf("PARTS\n");

                {
                  // uint8_t targetStatus = 3;
                  //  Target Status / 1 byte (See Table 12)
                  targetStatus = (uint8_t)dataFrame[2];
                  // Serial.printf("targetStatus: %d\n", targetStatus);

                  // Movement target distance (cm) / 2 bytes
                  movementTargetDistance = (dataFrame[4] << 8) | dataFrame[3];
                  // Serial.printf("movementTargetDistance (cm): %d\n", movementTargetDistance);

                  // Exercise target energy value / 1 byte
                  exerciseTargetEnergy = (uint8_t)dataFrame[5];
                  // Serial.printf("exerciseTargetEnergy (cm): %d\n", exerciseTargetEnergy);

                  // Distance to stationary target (cm) / 2 bytes
                  distanceToStationaryTarget = (dataFrame[7] << 8) | dataFrame[6];
                  // Serial.printf("distanceToStationaryTarget (cm): %d\n", distanceToStationaryTarget);

                  // // Stationary target energy value / 1 byte
                  // uint8_t stationaryTargetEnergyValue = *(uint8_t *)dataFrame[6 + 2];
                  stationaryTargetEnergyValue = (uint8_t)dataFrame[8];
                  // Serial.printf("stationaryTargetEnergyValue: %d\n", stationaryTargetEnergyValue);

                  // Serial.printf("stationary Target (cm): %d / %d\n", distanceToStationaryTarget, stationaryTargetEnergyValue);

                  // Detection distance (cm) / 2 bytes
                  detectionDistance = (dataFrame[10] << 8) | dataFrame[9];

                  // Serial.printf("Detection distance (cm): %d\n", detectionDistance);

                  // Serial.printf("targetStatus:%d, movementTargetDistance:%d, exerciseTargetEnergy:%d, distanceToStationaryTarget:%d, stationaryTargetEnergyValue:%d, detectionDistance:%d\n", targetStatus, movementTargetDistance, exerciseTargetEnergy, distanceToStationaryTarget, stationaryTargetEnergyValue, detectionDistance);

                  if (
                      // any
                      // old_targetStatus != targetStatus || old_movementTargetDistance != movementTargetDistance || old_exerciseTargetEnergy != exerciseTargetEnergy || old_distanceToStationaryTarget != distanceToStationaryTarget || old_stationaryTargetEnergyValue != stationaryTargetEnergyValue || old_detectionDistance != detectionDistance

                      // moving target
                      // old_targetStatus != targetStatus || old_movementTargetDistance != movementTargetDistance

                      // stationary target
                      // old_targetStatus != targetStatus || old_distanceToStationaryTarget != distanceToStationaryTarget

                      // moving or stationary target
                      old_targetStatus != targetStatus || old_distanceToStationaryTarget != distanceToStationaryTarget || old_movementTargetDistance != movementTargetDistance)
                  {
                    if (targetStatus == 0)
                    {
                      Serial.printf("targetStatus:  No target\n");
                    }
                    else if (targetStatus == 1)
                    {
                      Serial.printf("targetStatus:  Campaign target\n");
                    }
                    else if (targetStatus == 2)
                    {
                      Serial.printf("targetStatus:  Stationary target\n");
                    }
                    else if (targetStatus == 3)
                    {
                      Serial.printf("targetStatus:  Campaign & Stationary target\n");
                    }

                    // Serial.printf("moving target (cm/engery): %d / %d\n", movementTargetDistance, exerciseTargetEnergy);
                    Serial.printf("moving:%d, stationary:%d\n", movementTargetDistance, distanceToStationaryTarget);

                    // Serial.println("changed");
                  }
                  else
                  {
                    // Serial.println("no changed");
                  }

                  old_targetStatus = targetStatus;
                  old_movementTargetDistance = movementTargetDistance;
                  old_exerciseTargetEnergy = exerciseTargetEnergy;
                  old_distanceToStationaryTarget = distanceToStationaryTarget;
                  old_stationaryTargetEnergyValue = stationaryTargetEnergyValue;
                  old_detectionDistance = detectionDistance;
                }
              }
            }
          }
        }
      }
    }
    else
    {
      // Serial.println("No Radar Data");
    }
  }
}

void loop()
{

  if (ArduinoOTARunning)
  {
    delay(1);
    return;
  }

  auto ld2450Val = digitalRead(LD2450C_PIN);

  if (lastReading != ld2450Val)
  {
    auto diff = ((millis() - lastChangeMs)) / 1000.0;
    Serial.printf("\nld2450Val %d, (%ld)\n", ld2450Val, diff);
    lastChangeMs = millis();

    if (ld2450Val == 1)
    {
      digitalWrite(LED_PIN, LED_PIN_ON);
      if (!sendEspNowAction("on"))
      {
        sendToggleAction("light_on");
      }
    }
    else
    {
      digitalWrite(LED_PIN, LED_PIN_OFF);
      if (!sendEspNowAction("off"))
      {
        sendToggleAction("light_off");
      }
    }
  }

  lastReading = ld2450Val;

  if (doWiFi)
  {
    wiFiBegin();
    ArduinoOTA.handle();
  }

  auto touchReadValue = touchRead(TOUCH_PIN);
  if (touchReadValue > 0 && touchReadValue < 40)
  {
    Serial.printf("touch %d, light_toggle\n", touchReadValue);
    // take a second reading
    delay(16);
    touchReadValue = touchRead(TOUCH_PIN);
    if (touchReadValue > 0 && touchReadValue < 40)
    {
      Serial.printf("touch %d, light_toggle\n", touchReadValue);
      sendToggleAction("light_toggle");
    }
    else
    {
      Serial.printf("SECOND touch %d, light_toggle\n", touchReadValue);
    }
  }
  else if (digitalRead(BOOT_BUTTON_PIN) == LOW)
  {
    Serial.printf("touch boot pin, light_toggle\n");
    sendToggleAction("light_toggle");
  }

  if (lastNowMessageReady)
  {

    if (strcmp(lastNowMessage.action, "tog") == 0)
    {
      sendToggleAction("light_toggle");
    }
    else if (strcmp(lastNowMessage.action, "on") == 0)
    {
      sendToggleAction("light_on");
    }
    else if (strcmp(lastNowMessage.action, "off") == 0)
    {
      sendToggleAction("light_off");
    }
    else
    {
      Serial.println("unknown command");
    }
    lastNowMessageReady = false;
  }

  delay(16);

  return;
}

bool sendEspNowAction(const char *action)
{
  strcpy(lastNowMessage.action, action);
  esp_err_t result = esp_now_send(rearTVLightEspAddr, (uint8_t *)&lastNowMessage, sizeof(lastNowMessage));
  if (result == ESP_OK)
  {
    Serial.printf("Sent %s with success at %s\n", lastNowMessage.action, timeClient->getFormattedTime().c_str());
    return true;
  }
  else
  {
    Serial.printf("Error sending the %s\n", lastNowMessage.action);
    return false;
  }
}

void sendToggleAction(String action)
{

  if (!dnsReady)
  {
    Serial.printf("dnsReady not ready\n");
    delay(500);
    return;
  }

  HTTPClient http;

  // Specify the target URL
  http.begin("http://reartvlight.local/doAction");
  // http.begin("http://desktool.local/doAction");

  // Specify content type for JSON
  http.addHeader("Content-Type", "application/json");

  // Create the JSON body
  String jsonBody = "{\"action\":\"" + action + "\"}";

  // Serial.println(jsonBody);

  // Send POST request
  int httpResponseCode = http.POST(jsonBody);

  Serial.println(jsonBody);

  // Check response code
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  }
  else
  {
    Serial.println("Error on sending POST: " + String(httpResponseCode));
  }

  // End connection
  http.end();
}

void wiFiBegin()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  // Wait for connection
  int timeout = 20000; // Timeout after 10 seconds
  int elapsed = 0;
  Serial.println("\nStarting WiFi...");

  // flash

  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    delay(1000);
    Serial.print("O");
    elapsed += 1000;
  }

  // If not connected, start SmartConfig
  if (WiFi.status() != WL_CONNECTED)
  {

    Serial.println("\nStarting SmartConfig...");
    WiFi.beginSmartConfig();

    timeout = 2 * 60 * 1000;
    elapsed = 0;

    // Wait for SmartConfig to finish
    while (!WiFi.smartConfigDone() && elapsed < timeout)
    {
      delay(1000);
      Serial.print("X");
      elapsed += 1000;
    }

    if (!WiFi.smartConfigDone())
    {
      Serial.println("smartConfigDone failed, rebooting.");
      ESP.restart();
    }

    Serial.println("\nSmartConfig done.");
    // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
  }

  Serial.printf("\nWiFi Connected %s, %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());

  WiFiUDP ntpUDP;
  timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
  timeClient->begin();

  auto updated = timeClient->update();

  Serial.print("Current time: ");
  Serial.println(timeClient->getFormattedTime());
}