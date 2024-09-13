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
#include "DisplayUpdater.h"

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

void pushServoButtonX(void *pvParameters)
{

  servoMoving = true;
  pinMode(LED_ONBOARD, OUTPUT);
  DisplayUpdater *displayUpdater = static_cast<DisplayUpdater *>(pvParameters);
  DisplayParameters params = {true, true, epd_bitmap_icons8_natural_user_interface_2_13, 0, 0, NULL};
  displayUpdater->showIcon(&params);
  digitalWrite(LED_ONBOARD, HIGH);
  myServo.write(helper.servoAngle);
  vTaskDelay(pdMS_TO_TICKS(helper.pressDownHoldTime));
  myServo.write(helper.servoHomeAngle);
  digitalWrite(LED_ONBOARD, LOW);
  params.show = false;
  displayUpdater->showIcon(&params);
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

void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  displayUpdater->begin();

  helper.wiFiBegin("DarkNet", "7pu77ies77", displayUpdater);

  // DisplayParameters params = {true, false, epd_bitmap_icons8_wifi_13, 80, 0, NULL};
  // if (displayUpdater != NULL)
  // {
  //   displayUpdater->showIcon(&params);
  // }

  tempRecorder->begin();

  myServo.attach(SERVO_PIN);

  touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);
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
}
