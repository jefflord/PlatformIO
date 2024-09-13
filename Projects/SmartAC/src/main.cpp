/*
Notes:
  - Temp senor needs 5v and 4.6kΩR (pull-up I think)
  - Activation button has a extra 10kΩR (pull-up)

To Do:
  - Log start time, boot reason
  - Reboot from web
*/
#include <Arduino.h>
#include <ESP32Servo.h>
#include "helper.h"

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

// Servo myServo; // Create a Servo object

MyIoTHelper helper("SmartAC");
TempRecorder *tempRecorder;
DisplayUpdater *displayUpdater;

// bool btn1IsDown = false;
// bool btn2IsDown = true;

bool servoMoving = false;

void pushServoButtonX(void *pvParameters)
{
  DisplayUpdater *displayUpdater = static_cast<DisplayUpdater *>(pvParameters);
  displayUpdater->renderClickIcon(true);

  servoMoving = true;
  digitalWrite(LED_ONBOARD, HIGH);
  // myServo.write(0);
  delay(750);
  // myServo.write(90);
  digitalWrite(LED_ONBOARD, LOW);
  servoMoving = false;
  displayUpdater->renderClickIcon(false);
  vTaskDelete(NULL);
}

void pushServoButton()
{
  if (!servoMoving)
  {
    xTaskCreate(pushServoButtonX, "pushServoButton", 2048, displayUpdater, 1, NULL);
  }
}

void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  displayUpdater->begin();

  helper.wiFiBegin("DarkNet", "7pu77ies77");

  tempRecorder->begin();

  touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);
}

int lastPotValue = -1;

void loop()
{
}
