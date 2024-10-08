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

#include "MyIoTHelper.h"
#include "DisplayUpdater.h"
#include "WebServerHelper.h"
#include "ButtonServerHelper.h"
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

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

MyIoTHelper iotHelper("SmartAC");
TempRecorder *tempRecorder;
WebServerHelper *webServerHelper;
DisplayUpdater *displayUpdater;
ButtonServerHelper *buttonServerHelper;

// bool btn1IsDown = false;
// bool btn2IsDown = true;

ThreadSafeSerial safeSerial;
ThreadSafeSerial *sSerial = &safeSerial;

void setup()
{

  iotHelper.Setup();

  buttonServerHelper = new ButtonServerHelper();

  tempRecorder = new TempRecorder(&iotHelper);

  displayUpdater = new DisplayUpdater(&iotHelper, tempRecorder);

  webServerHelper = new WebServerHelper();

  iotHelper.SetDisplay(displayUpdater);

  if (touchRead(TOUCH_PIN) < 50)
  {
    // while (true)
    // {

    //   safeSerial.println("Touch on boot detected");
    //   auto xxx = touchRead(TOUCH_PIN);
    //   safeSerial.printf("Touch on boot detected %d\n", xxx);
    //   delay(100);
    // }

    // iotHelper.setSafeBoot();
  }

  displayUpdater->begin();

  iotHelper.wiFiBegin();

  tempRecorder->begin();

  buttonServerHelper->begin(&iotHelper, displayUpdater);

  ArduinoOTA.begin();

  webServerHelper->begin();

  delay(1000);
}

int lastPotValue = -1;

TaskHandle_t ArduinoOTAHandle = NULL;

touch_value_t lastVal = -100;
touch_value_t currentVal = -100;
void loop()
{

  // currentVal = touchRead(TOUCH_PIN);
  // if (currentVal != lastVal)
  // {
  //   lastVal = touchRead(TOUCH_PIN);
  //   safeSerial.printf("touchRead %d\n", lastVal);
  // }

  // safeSerial.printf("##########\n", lastVal);
  // lastVal = touchRead(33);
  // safeSerial.printf("33 touchRead %d\n", lastVal);
  // safeSerial.printf("##########\n", lastVal);

  // safeSerial.printf("##########\n", lastVal);
  // lastVal = touchRead(12);
  // safeSerial.printf("12 touchRead %d\n", lastVal);
  // safeSerial.printf("##########\n", lastVal);

  // safeSerial.printf("##########\n", lastVal);
  // lastVal = touchRead(13);
  // safeSerial.printf("13 touchRead %d\n", lastVal);
  // safeSerial.printf("##########\n", lastVal);

  // safeSerial.printf("##########\n", lastVal);
  // lastVal = touchRead(14);
  // safeSerial.printf("14 touchRead %d\n", lastVal);
  // safeSerial.printf("##########\n", lastVal);
  // delay(500);
  // return;

  // safeSerial.printf("##########\n", lastVal);
  // lastVal = touchRead(39);
  // safeSerial.printf("39 touchRead %d\n", lastVal);
  // safeSerial.printf("##########\n", lastVal);

  lastVal = currentVal;

  buttonServerHelper->updateLoop();

  ArduinoOTA.handle();

  vTaskDelay(pdMS_TO_TICKS(16));
}
