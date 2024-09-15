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
#include <WiFi.h>
#include <ESP32Servo.h>
#include "MyIoTHelper.h"
#include "DisplayUpdater.h"
#include <ArduinoOTA.h>

#include <esp_system.h>
#include <esp_task_wdt.h>

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

void showWiFiMode()
{

  int currentMode = WiFi.getMode();

  // Print the mode to the serial monitor for debugging
  Serial.print("Current Wi-Fi mode: ");
  switch (currentMode)
  {
  case WIFI_OFF:
    Serial.println("OFF");
    break;
  case WIFI_STA:
    Serial.println("STA");
    break;
  case WIFI_AP:
    Serial.println("AP");
    break;
  case WIFI_AP_STA:
    Serial.println("AP_STA");
    break;
  default:
    Serial.println("Unknown");
    break;
  }
}

void wifiShit()
{

  safeSerial.println("wifiShit start");
  showWiFiMode();

  WiFi.mode(WIFI_STA);
  // Try to connect to the saved WiFi credentials
  WiFi.begin();

  // Wait for connection
  int timeout = 10000; // Timeout after 10 seconds
  int elapsed = 0;
  safeSerial.println("\nStarting WiFi...");

  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    delay(500);
    Serial.print(".");
    elapsed += 500;
  }

  bool smartConfigUsed = false;
  // If not connected, start SmartConfig
  if (WiFi.status() != WL_CONNECTED)
  {
    smartConfigUsed = true;
    WiFi.mode(WIFI_AP_STA);
    Serial.println("\nStarting SmartConfig...");
    WiFi.beginSmartConfig();

    // Wait for SmartConfig to finish
    while (!WiFi.smartConfigDone())
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println("\nSmartConfig done.");
    // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
  }
  else
  {
    // Serial.printf("Connected to saved WiFi: %s\n", WiFi.SSID().c_str());
  }

  if (smartConfigUsed)
  {
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    Serial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }

  showWiFiMode();
}
void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  helper.SetDisplay(displayUpdater);

  displayUpdater->begin();

  helper.wiFiBegin("", "");

  tempRecorder->begin();
  myServo.attach(SERVO_PIN);

  touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);

  ArduinoOTA.begin();
}

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
