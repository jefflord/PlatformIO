
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <ESPmDNS.h>
#include <esp_now.h>
#include <esp_system.h>

HardwareSerial RadarSerial(1);
WiFiUDP ntpUDP;
WebServer diagnosticsServer(80);

#define LED_PIN 2
#define LED_PIN_OFF LOW
#define LED_PIN_ON HIGH
#define readSensorSerial false

#define TOUCH_PIN 12
#define LD2410C_PIN 23 // D23, LD2410C OUT
#define BOOT_BUTTON_PIN 0

const bool doWiFi = true;
volatile bool dnsReady = false;
NTPClient *timeClient;
bool diagnosticsServerStarted = false;
bool espNowReady = false;
esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;
volatile uint32_t wifiDisconnectCount = 0;
volatile uint32_t wifiIpAcquisitionCount = 0;
volatile uint8_t lastWiFiDisconnectReason = 0;
volatile unsigned long lastWiFiDisconnectMs = 0;
volatile unsigned long lastWiFiGotIpMs = 0;

void wiFiBegin();
void readSensor(void *p);
void sendToggleAction(String action);
bool sendEspNowAction(const char *action);
void configureDiagnosticsServer();
void startDiagnosticsServerIfConnected();
void handleDiagnosticsPage();
void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info);

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
    MDNS.addService("http", "tcp", 80);
    Serial.println("Device can be accessed at desktool.local");

    dnsReady = true;
    vTaskDelete(NULL);
  }
}

const char *resetReasonName(esp_reset_reason_t reason)
{
  switch (reason)
  {
  case ESP_RST_POWERON:
    return "Power-on";
  case ESP_RST_EXT:
    return "External reset";
  case ESP_RST_SW:
    return "Software reset";
  case ESP_RST_PANIC:
    return "Panic / exception";
  case ESP_RST_INT_WDT:
    return "Interrupt watchdog";
  case ESP_RST_TASK_WDT:
    return "Task watchdog";
  case ESP_RST_WDT:
    return "Watchdog";
  case ESP_RST_DEEPSLEEP:
    return "Deep sleep wake";
  case ESP_RST_BROWNOUT:
    return "Brownout";
  case ESP_RST_SDIO:
    return "SDIO reset";
  default:
    return "Unknown";
  }
}

const char *wifiStatusName(wl_status_t status)
{
  switch (status)
  {
  case WL_IDLE_STATUS:
    return "Idle";
  case WL_NO_SSID_AVAIL:
    return "SSID unavailable";
  case WL_SCAN_COMPLETED:
    return "Scan complete";
  case WL_CONNECTED:
    return "Connected";
  case WL_CONNECT_FAILED:
    return "Connection failed";
  case WL_CONNECTION_LOST:
    return "Connection lost";
  case WL_DISCONNECTED:
    return "Disconnected";
  case WL_NO_SHIELD:
    return "Wi-Fi unavailable";
  default:
    return "Unknown";
  }
}

String formatUptime(unsigned long seconds)
{
  char buffer[48];
  const unsigned long days = seconds / 86400UL;
  const unsigned long hours = (seconds / 3600UL) % 24UL;
  const unsigned long minutes = (seconds / 60UL) % 60UL;
  const unsigned long remainingSeconds = seconds % 60UL;
  snprintf(buffer, sizeof(buffer), "%lu d %02lu:%02lu:%02lu", days, hours, minutes, remainingSeconds);
  return String(buffer);
}

String htmlEscape(const String &value)
{
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); i++)
  {
    switch (value[i])
    {
    case '&':
      escaped += F("&amp;");
      break;
    case '<':
      escaped += F("&lt;");
      break;
    case '>':
      escaped += F("&gt;");
      break;
    case '"':
      escaped += F("&quot;");
      break;
    case '\'':
      escaped += F("&#39;");
      break;
    default:
      escaped += value[i];
      break;
    }
  }
  return escaped;
}

void appendDiagnosticsRow(String &page, const char *label, const String &value)
{
  page += F("<tr><th scope=\"row\">");
  page += label;
  page += F("</th><td>");
  page += htmlEscape(value);
  page += F("</td></tr>");
}

void handleDiagnosticsPage()
{
  const wl_status_t wifiStatus = WiFi.status();
  const bool connected = wifiStatus == WL_CONNECTED;
  const uint32_t disconnects = wifiDisconnectCount;
  const uint32_t ipAcquisitions = wifiIpAcquisitionCount;
  const uint8_t disconnectReason = lastWiFiDisconnectReason;
  const unsigned long disconnectMs = lastWiFiDisconnectMs;
  const unsigned long gotIpMs = lastWiFiGotIpMs;

  String page;
  page.reserve(3072);
  page += F("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">");
  page += F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  page += F("<meta http-equiv=\"refresh\" content=\"5\"><title>DeskTool diagnostics</title>");
  page += F("<style>body{font:16px system-ui,sans-serif;max-width:760px;margin:2rem auto;padding:0 1rem;color:#222}h1{margin-bottom:.2rem}p{color:#555}table{width:100%;border-collapse:collapse;margin-top:1rem}th,td{text-align:left;padding:.55rem .7rem;border-bottom:1px solid #ddd;overflow-wrap:anywhere}th{width:38%;background:#f5f5f5}</style>");
  page += F("</head><body><h1>DeskTool diagnostics</h1><p>Auto-refreshes every 5 seconds. Build: ");
  page += __DATE__;
  page += ' ';
  page += __TIME__;
  page += F("</p><table><tbody>");

  appendDiagnosticsRow(page, "Uptime", formatUptime(millis() / 1000UL));
  appendDiagnosticsRow(page, "Reset reason", String(resetReasonName(bootResetReason)) + " (" + String(static_cast<int>(bootResetReason)) + ")");
  appendDiagnosticsRow(page, "Chip", String(ESP.getChipModel()) + " rev " + String(ESP.getChipRevision()));
  appendDiagnosticsRow(page, "Arduino-ESP32 / ESP-IDF", String(ESP.getSdkVersion()));
  appendDiagnosticsRow(page, "CPU frequency", String(ESP.getCpuFreqMHz()) + " MHz");
  appendDiagnosticsRow(page, "Free heap", String(ESP.getFreeHeap()) + " bytes");
  appendDiagnosticsRow(page, "Minimum free heap since boot", String(ESP.getMinFreeHeap()) + " bytes");
  appendDiagnosticsRow(page, "Wi-Fi status", wifiStatusName(wifiStatus));
  appendDiagnosticsRow(page, "Wi-Fi SSID", connected ? WiFi.SSID() : String("not connected"));
  appendDiagnosticsRow(page, "IP address", WiFi.localIP().toString());
  appendDiagnosticsRow(page, "Signal strength", connected ? String(WiFi.RSSI()) + " dBm" : String("n/a"));
  appendDiagnosticsRow(page, "Wi-Fi channel", connected ? String(WiFi.channel()) : String("n/a"));
  appendDiagnosticsRow(page, "MAC address", WiFi.macAddress());
  appendDiagnosticsRow(page, "Wi-Fi IP acquisitions", String(ipAcquisitions));
  appendDiagnosticsRow(page, "Wi-Fi disconnect events", String(disconnects));
  appendDiagnosticsRow(page, "Last disconnect reason code", disconnects ? String(disconnectReason) : String("none"));
  appendDiagnosticsRow(page, "Last disconnect", disconnects ? formatUptime((millis() - disconnectMs) / 1000UL) + " ago" : String("never"));
  appendDiagnosticsRow(page, "Last IP acquisition", ipAcquisitions ? formatUptime((millis() - gotIpMs) / 1000UL) + " ago" : String("never"));
  appendDiagnosticsRow(page, "DeskTool mDNS", dnsReady ? String("started") : String("not started"));
  appendDiagnosticsRow(page, "ESP-NOW", espNowReady ? String("initialized") : String("not initialized"));
  appendDiagnosticsRow(page, "Presence input GPIO 23", digitalRead(LD2410C_PIN) == HIGH ? String("HIGH") : String("LOW"));
  appendDiagnosticsRow(page, "NTP time", timeClient != nullptr ? timeClient->getFormattedTime() : String("not initialized"));

  page += F("</tbody></table><p>Open this page at <code>http://desktool.local/</code> or use the IP shown above.</p></body></html>");
  diagnosticsServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  diagnosticsServer.send(200, "text/html; charset=utf-8", page);
}

void configureDiagnosticsServer()
{
  diagnosticsServer.on("/", HTTP_GET, handleDiagnosticsPage);
  diagnosticsServer.on("/diag", HTTP_GET, handleDiagnosticsPage);
}

void startDiagnosticsServerIfConnected()
{
  if (!diagnosticsServerStarted && WiFi.status() == WL_CONNECTED)
  {
    diagnosticsServer.begin();
    diagnosticsServerStarted = true;
    Serial.printf("Diagnostics page: http://%s/\n", WiFi.localIP().toString().c_str());
  }
}

void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info)
{
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
  {
    lastWiFiDisconnectReason = info.wifi_sta_disconnected.reason;
    lastWiFiDisconnectMs = millis();
    wifiDisconnectCount++;
  }
  else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
  {
    lastWiFiGotIpMs = millis();
    wifiIpAcquisitionCount++;
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

  if (len != sizeof(lastNowMessage))
  {
    lastNowMessageReady = false;
    // Serial.printf("bad data received, len:%d, expected:%d\n", len, sizeof(lastNowMessage));
    return;
  }
  else
  {
    Serial.printf("data received, len:%d\n", len);
  }

  memcpy(&lastNowMessage, incomingData, sizeof(lastNowMessage));

  lastNowMessageReady = true;
}

void SetupOTA();

bool ArduinoOTARunning = false;
/************/
void setup()
{
  Serial.begin(115200);
  bootResetReason = esp_reset_reason();
  Serial.printf("Reset reason: %s (%d)\n", resetReasonName(bootResetReason), static_cast<int>(bootResetReason));
  configureDiagnosticsServer();
  WiFi.onEvent(onWiFiEvent);

  // The default baud rate of the serial port is
  // 256000, with 1 stop bit and no parity bit.
  RadarSerial.begin(256000, SERIAL_8N1, 16, 17);

  while (!Serial)
    continue;

  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
      pinMode(LD2410C_PIN, INPUT);

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
    startDiagnosticsServerIfConnected();

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
      espNowReady = true;
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

                          Serial.println("OTA Update Starting (password accepted): " + type);

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

  ArduinoOTA.setPassword("desktool-update");
  ArduinoOTA.begin();
}

int lastReading = -1;
unsigned long lastChangeMs = 0;
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
                      Serial.printf("targetStatus:  Moving target\n");
                    }
                    else if (targetStatus == 2)
                    {
                      Serial.printf("targetStatus:  Stationary target\n");
                    }
                    else if (targetStatus == 3)
                    {
                      Serial.printf("targetStatus:  Moving & stationary target\n");
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

static unsigned long lastTimeCheck = 0;

static bool forceAlwaysUseWiFiBackup = true;

static bool bootButtonPressDetected()
{
  // GPIO0 can be held low by the serial adapter during a DTR reset. Do not arm
  // the button until it has been stably released, then report one event per press.
  static int lastReading = HIGH;
  static int stableReading = HIGH;
  static unsigned long lastTransitionMs = 0;
  static bool armed = false;

  const int reading = digitalRead(BOOT_BUTTON_PIN);
  const unsigned long now = millis();
  if (reading != lastReading)
  {
    lastReading = reading;
    lastTransitionMs = now;
  }

  if (now - lastTransitionMs < 50)
  {
    return false;
  }

  if (reading != stableReading)
  {
    stableReading = reading;
    if (stableReading == HIGH)
    {
      armed = true;
    }
    else if (armed)
    {
      armed = false;
      return true;
    }
  }
  else if (stableReading == HIGH)
  {
    // Arm on the first stable release after startup.
    armed = true;
  }

  return false;
}

void loop()
{

  if (doWiFi)
  {
    wiFiBegin();
    startDiagnosticsServerIfConnected();
    if (diagnosticsServerStarted)
    {
      diagnosticsServer.handleClient();
    }
    ArduinoOTA.handle();
  }

  // Give OTA handling priority over periodic, sensor, and HTTP work.
  if (ArduinoOTARunning)
  {
    delay(1);
    return;
  }

  if (millis() - lastTimeCheck > 1000)
  {
    lastTimeCheck = millis();
    if (doWiFi)
    {
      timeClient->update();
      Serial.printf("%s presence detected: %s\n",
                    timeClient->getFormattedTime().c_str(),
                    lastReading == HIGH ? "yes" : "no");
    }
  }

  auto ld2410cVal = digitalRead(LD2410C_PIN);

  if (lastReading != ld2410cVal)
  {
    const unsigned long currentMillis = millis();
    const float secondsSinceLastChange = (currentMillis - lastChangeMs) / 1000.0f;
    Serial.printf("\npresence detected: %s (%.1f seconds since last change)\n",
                  ld2410cVal == HIGH ? "yes" : "no",
                  secondsSinceLastChange);
    lastChangeMs = currentMillis;

    if (ld2410cVal == 1)
    {
      digitalWrite(LED_PIN, LED_PIN_ON);
      if (!sendEspNowAction("on") || forceAlwaysUseWiFiBackup)
      {
        sendToggleAction("light_on");
      }
    }
    else
    {
      digitalWrite(LED_PIN, LED_PIN_OFF);
      if (!sendEspNowAction("off") || forceAlwaysUseWiFiBackup)
      {
        sendToggleAction("light_off");
      }
    }
  }

  lastReading = ld2410cVal;

  auto touchReadValue = touchRead(TOUCH_PIN);
  if (touchReadValue > 0 && touchReadValue < 40)
  {
    // take a second reading
    delay(16);
    touchReadValue = touchRead(TOUCH_PIN);
    if (touchReadValue > 0 && touchReadValue < 40)
    {
      Serial.printf("Manual light state change requested: light_toggle (touch input, reading %d)\n", touchReadValue);
      sendToggleAction("light_toggle");
    }
    else
    {
      Serial.printf("Touch input not confirmed (reading %d)\n", touchReadValue);
    }
  }
  else if (bootButtonPressDetected())
  {
    Serial.println("Manual light state change requested: light_toggle (BOOT button)");
    sendToggleAction("light_toggle");
  }

  if (lastNowMessageReady)
  {

    if (strcmp(lastNowMessage.action, "tog") == 0)
    {
      Serial.println("Manual light state change requested: light_toggle (ESP-NOW)");
      sendToggleAction("light_toggle");
    }
    else if (strcmp(lastNowMessage.action, "on") == 0)
    {
      Serial.println("Manual light state change requested: light_on (ESP-NOW)");
      sendToggleAction("light_on");
    }
    else if (strcmp(lastNowMessage.action, "off") == 0)
    {
      Serial.println("Manual light state change requested: light_off (ESP-NOW)");
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

  timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
  timeClient->begin();

  auto updated = timeClient->update();

  Serial.print("Current time: ");
  Serial.println(timeClient->getFormattedTime());
}
