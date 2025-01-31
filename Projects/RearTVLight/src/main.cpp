#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <TaskScheduler.h>
#include <espnow.h>

#define FLASH_BTN 0
#define LIGHTS_PIN 12 // D6

#define LED_BUILTIN_OFF HIGH
#define LED_BUILTIN_ON LOW

#define LIGHTS_PIN_ON HIGH
#define LIGHTS_PIN_OFF LOW

void wifiBegin()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  WiFi.begin();
  int timeout = 20000; // Timeout after 10 seconds
  int elapsed = 0;
  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    delay(500);
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    delay(500);
    Serial.print("O");
    elapsed += 1000;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nStarting SmartConfig...");
    // Start SmartConfig
    WiFi.beginSmartConfig();

    timeout = 2 * 60 * 1000;
    elapsed = 0;

    // Wait for SmartConfig to finish
    while (!WiFi.smartConfigDone() && elapsed < timeout)
    {
      digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
      delay(250);
      digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
      delay(250);
      digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
      delay(250);
      digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
      delay(250);
      Serial.print("X");
      elapsed += 1000;
    }

    if (!WiFi.smartConfigDone())
    {
      Serial.println("smartConfigDone failed, rebooting.");
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi Connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

ESP8266WebServer server(80); // Create a web server on port 80
bool lightStateOn = false;
bool toggleOk = true;

// Structure to receive data
typedef struct struct_message
{
  char action[4] = "";
} struct_message;

// Create a struct_message to hold incoming data
struct_message lastNowMessage;
bool lastNowMessageReady = false;
bool espNowWorking = false;

void handleOptions()
{
  // Set CORS headers
  Serial.println("handleOptions");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204); // Respond with 204 No Content (valid for OPTIONS)
}

void handlePostRequest()
{
  Serial.println("handlePostRequest");

  String json = server.arg("plain"); // Get the JSON string

  // Parse the received JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error)
  {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Extract data from the parsed JSON
  auto action = String(doc["action"]); // Assuming there's an "action" field in the JSON

  if (action == "light_off")
  {
    lightStateOn = false;
  }
  else if (action == "light_on")
  {
    lightStateOn = true;
  }
  else if (action == "light_toggle")
  {
    lightStateOn = !lightStateOn;
  }

  // Perform your action based on the received data
  String result = String("Action performed: ") + action;

  // Create a JSON response
  JsonDocument responseDoc;
  responseDoc["light_state_on"] = lightStateOn;

  String response;
  serializeJson(responseDoc, response);

  // Send the response
  server.sendHeader("Access-Control-Allow", "*");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", response);
}

void handleRoot()
{
  Serial.println("handleRoot");

  JsonDocument responseDoc;
  responseDoc["light_state_on"] = lightStateOn;
  responseDoc["ip"] = WiFi.localIP().toString();
  responseDoc["mac"] = WiFi.macAddress();
  responseDoc["last_now_message_ready"] = lastNowMessageReady;
  responseDoc["esp_now_working"] = espNowWorking;
  responseDoc["up_time"] = millis();

  String response;
  serializeJson(responseDoc, response);

  server.send(200, "application/json", response.c_str()); // Send a response
}

Scheduler scheduler;
void setupmDNSTask()
{

  if (MDNS.begin("reartvlight"))
  {
    Serial.println("mDNS responder started: reartvlight.local");
    MDNS.addService("http", "tcp", 80);
  }
  else
  {
    Serial.println("Error setting up mDNS responder!");
  }
}

Task myTaskRunner(3, 1, &setupmDNSTask);

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
// void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  // Serial.println("command recieved");

  if (len != sizeof(lastNowMessage))
  {

    // for (int i = 0; i < 3; i++)
    // {

    //   delay(2000);

    //   digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    //   digitalWrite(LIGHTS_PIN, LIGHTS_PIN_ON);

    //   delay(2000);

    //   digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    //   digitalWrite(LIGHTS_PIN, LIGHTS_PIN_OFF);
    // }

    // Serial.println("invalid message size");
    return;
  }

  memcpy(&lastNowMessage, incomingData, sizeof(lastNowMessage));
  lastNowMessageReady = true;
}

void setup()
{

  pinMode(FLASH_BTN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LIGHTS_PIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  digitalWrite(LIGHTS_PIN, HIGH);

  Serial.begin(115200);
  while (!Serial)
    continue;

  wifiBegin();

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  if (esp_now_init() != 0)
  {
    espNowWorking = false;
  }
  else
  {
    espNowWorking = true;
    esp_now_register_recv_cb(OnDataRecv);
  }

  scheduler.addTask(myTaskRunner);
  myTaskRunner.enable();

  ArduinoOTA.begin();

  server.on("/", handleRoot);

  server.on("/doAction", HTTP_POST, handlePostRequest);

  server.on("/doAction", HTTP_OPTIONS, handleOptions);

  // Start the server
  server.begin();
  Serial.println("server.begin");
}

void loop()
{

  wifiBegin();

  if (lastNowMessageReady)
  {

    if (strcmp(lastNowMessage.action, "tog") == 0)
    {
      lightStateOn = !lightStateOn;
    }
    else if (strcmp(lastNowMessage.action, "on") == 0)
    {
      lightStateOn = true;
    }
    else if (strcmp(lastNowMessage.action, "off") == 0)
    {
      lightStateOn = false;
    }
    else
    {
      Serial.println("unknown command");
    }
    lastNowMessageReady = false;
  }

  scheduler.execute();
  server.handleClient();

  int buttonState = digitalRead(FLASH_BTN);

  if (lightStateOn)
  {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    digitalWrite(LIGHTS_PIN, LIGHTS_PIN_ON);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    digitalWrite(LIGHTS_PIN, LIGHTS_PIN_OFF);
  }

  if (buttonState == LOW && toggleOk)
  {
    toggleOk = false;
    // Serial.println("Flash button pressed!");
    lightStateOn = !lightStateOn;
    if (lightStateOn)
    {
      digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    }
    else
    {
      digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    }
    // delay(1000);
  }
  else if (buttonState == HIGH)
  {
    // Serial.println("Flash button NOT pressed!");
    toggleOk = true;
  }

  ArduinoOTA.handle();
  delay(16);
}
