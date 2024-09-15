/*
Notes:
  - Temp senor needs 5v and 4.6kOR (pull-up I think)
  - Activation button has a extra 10kOR (pull-up)

Done phase 1 (software):
  *- Records 3 temps
  *- Shows temps
  *- Has time
  *- Upload animation
  *- Real-time wifi connection status
  - Keep AP password out of source code.
  - Method to force enter WiFi setup
  - Records IP for OTA update
    - Include boot reason

Done phase 1 (hardware):
  - Touch button
  - Soldered to PCB
  - Tape down
  - Power


To Do:
  - Log start time, boot reason
  - Reboot from web
*/

#include <Arduino.h>
#include <ESP32Servo.h>
#include "MyIoTHelper.h"
#include "DisplayUpdater.h"
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <esp_system.h>
#include <esp_task_wdt.h>

#include <FS.h>     // File System library
#include <SPIFFS.h> // SPIFFS file system

/**************/

/**************/
/***************************************************/

// const int button1Pin = 2; // update config
// const int button2Pin = 4; // kill wifi

// const int servoPin = 32;
// const int potPin = 34;

// int button1State = 0;
// int button2State = 0;

// int potValue = 0;

// int pressDownHoldTime = 250;
// int startAngle = 90;

MyIoTHelper helper("SmartAC");
TempRecorder *tempRecorder;
DisplayUpdater *displayUpdater;
Servo myServo;
// bool btn1IsDown = false;
// bool btn2IsDown = true;

bool servoMoving = false;
ThreadSafeSerial safeSerial;

void pushServoButtonX(void *pvParameters)
{

  servoMoving = true;
  pinMode(LED_ONBOARD, OUTPUT);
  DisplayUpdater *displayUpdater = static_cast<DisplayUpdater *>(pvParameters);

  DisplayParameters params = {-1, 250, false, epd_bitmap_icons8_natural_user_interface_2_13, 0, 0, NULL};
  displayUpdater->flashIcon(&params);

  digitalWrite(LED_ONBOARD, HIGH);
  myServo.write(helper.servoAngle);
  vTaskDelay(pdMS_TO_TICKS(helper.pressDownHoldTime));
  myServo.write(helper.servoHomeAngle);
  digitalWrite(LED_ONBOARD, LOW);

  displayUpdater->hideIcon(&params);

  servoMoving = false;

  vTaskDelete(NULL);
}

auto lastTouch = millis();
auto lastPress = millis() * 2;
auto okToGo = false;

void pushServoButton()
{
  // record each time we are touched
  lastTouch = millis();

  if (millis() > lastPress + 3000)
  {
    // reset this each time
    lastPress = millis();

    // we only want to do this one per, so we use okToGo
    if (okToGo)
    {

      helper.chaos("wifi");
      vTaskDelay(pdMS_TO_TICKS(2000));
      helper.updateConfig();
      okToGo = false;
    }
  }

  if (!servoMoving)
  {
    xTaskCreate(pushServoButtonX, "pushServoButton", 2048 * 8, displayUpdater, 1, NULL);
  }
}
AsyncWebServer server(8222);

void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  Serial.println("json!");
  // Parse incoming JSON
  JsonDocument jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, data);

  if (error)
  {
    Serial.println("Error parsing JSON");
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Extract data from JSON
  const char *name = jsonDoc["name"];
  int age = jsonDoc["age"];
  bool hitSwitch = jsonDoc["hitSwitch"];

  if (hitSwitch)
  {
    safeSerial.println("Doing pushServoButton");
    pushServoButton();
  }
  else
  {
    safeSerial.println("Not doing pushServoButton");
  }

  // Print received data
  // Serial.printf("Name: %s, Age: %d\n", name, age);

  // int currentLedState = digitalRead(LED_BUILTIN);
  // sprintf()

  JsonDocument doc;

  // Add values in the document
  doc["sensor"] = "gps";
  doc["status"] = "success";
  doc["millis"] = millis();

  String responseData = "";
  serializeJsonPretty(doc, responseData);
  // Send a JSON response with CSP header
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseData);

  // Add Content Security Policy (CSP) header
  response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");

  // Send the response
  request->send(response);
}

void setupServer()
{

  SPIFFS.begin(true);

  server.on("/js/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {              
              safeSerial.println("main.js request");
              request->send(SPIFFS, "/wwwroot/js/main.js", "text/html"); });

  server.on("/spiffs", HTTP_GET, [](AsyncWebServerRequest *request)
            {              
              safeSerial.println("spiffs request");
              request->send(SPIFFS, "/wwwroot/index.html", "text/html"); });

  

  // server.on("/spiffsgz", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html.gz", "text/html");
  //   response->addHeader("Content-Encoding", "gzip");
  //   request->send(response); });

  server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("html!");
              String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web Server!!!!</title></head><body><h1>Hello from ESP32!!!!</h1></body></html>";
              
              // Send a JSON response with CSP header
              AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);

              // Add Content Security Policy (CSP) header
              response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");
              response->addHeader("Test", "!!!");

              // Send the response
              request->send(response); });

  server.on("/htmlx", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("htmlx!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmlx!</title></head><body><h1>Hello from htmlx!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

  server.on("/htmly", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("htmly!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmly!</title></head><body><h1>Hello from htmly!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

  // server.on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
  //           { handleJsonPost(request, data, len, index, total); }

  server.on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleJsonPost);

  // Start the server

  server.begin();
}

void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  helper.SetDisplay(displayUpdater);

  if (touchRead(TOUCH_PIN) < 50)
  {
    Serial.println("Touch on boot detected");
    helper.setSafeBoot();
  }

  displayUpdater->begin();

  helper.wiFiBegin();

  tempRecorder->begin();

  myServo.attach(SERVO_PIN);

  touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);

  ArduinoOTA.begin();

  safeSerial.println("setupServer starting");
  setupServer();
  safeSerial.println("setupServer done");
  delay(1000);
}

int lastPotValue = -1;

TaskHandle_t ArduinoOTAHandle = NULL;
void loop()
{

  if (millis() > lastTouch + 500)
  {
    // over 500ms since last time we saw touch, mark as OK to go AND pretend we just pressed it by setting lastPress = now.
    okToGo = true;
    lastPress = millis();
  }

  ArduinoOTA.handle();

  vTaskDelay(pdMS_TO_TICKS(16));
}
