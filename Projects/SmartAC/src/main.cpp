/*
Notes:
  - Temp senor needs 5v and 4.6kΩR (pull-up I think)
  - Activation button has a extra 10kΩR (pull-up)

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

bool SmartConfig()
{
  Serial.println("beginSmartConfig...");
  delay(2000);
  Serial.println("beginSmartConfig...Go...");

  // Set ESP32 to station mode
  WiFi.mode(WIFI_AP_STA);

  // Serial.println("WIFI_AP_STA done");

  // WiFi.mode(WIFI_STA);

  // Serial.println("WIFI_STA done");

  // Start SmartConfig
  Serial.println("beginSmartConfig in 2000");
  delay(2000);
  try
  {
    WiFi.beginSmartConfig();
  }
  catch (const std::exception &e)
  {
    safeSerial.println("e.what()!!!");
    safeSerial.println(e.what());
  }

  return false;

  delay(2000);
  Serial.println("beginSmartConfig going");
  delay(2000);

  while (!WiFi.smartConfigDone())
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("SmartConfig received.");

  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  return true;
}
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

void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  helper.SetDisplay(displayUpdater);
  displayUpdater->begin();

  if (!SmartConfig())
  {
    helper.wiFiBegin("DarkNet", "7pu77ies77");
  }

  tempRecorder->begin();

  myServo.attach(SERVO_PIN);

  touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);

  ArduinoOTA.begin();
}

int lastPotValue = -1;

void loop()
{

  if (millis() > lastTouch + 500)
  {
    // over 500ms since last time we saw touch, mark as OK to go AND pretend we just pressed it by setting lastPress = now.
    okToGo = true;
    lastPress = millis();
  }

  ArduinoOTA.handle();
}
