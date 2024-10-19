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
#include <ESPmDNS.h>

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

int otaLastPercent = -1;
MyIoTHelper iotHelper("SmartAC");
TempRecorder *tempRecorder;
WebServerHelper *webServerHelper;
DisplayUpdater *displayUpdater;
ButtonServerHelper *buttonServerHelper;

// bool btn1IsDown = false;
// bool btn2IsDown = true;

ThreadSafeSerial safeSerial;
ThreadSafeSerial *sSerial = &safeSerial;

bool dnsReady = false;

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

  if (!MDNS.begin("smartac"))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  else
  {
    Serial.println("Device can be accessed at smartac.local");
    // vTaskDelay(pdMS_TO_TICKS(100));
    //  if (MDNS.addService("http", "tcp", 80))
    //  {
    //    Serial.println("addService 1 worked");
    //  }

    dnsReady = true;
    vTaskDelete(NULL);
  }
}

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

                         iotHelper.ArduinoOTARunning = true; });
  ArduinoOTA.onEnd([]()
                   { 
                      iotHelper.ArduinoOTARunning = false;
                      Serial.println("\nOTA Update Finished"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { if ((progress / (total / 100) != otaLastPercent)) {
                            otaLastPercent = (progress / (total / 100));
                            Serial.printf("Progress: %u%% (%u/%u)\n", otaLastPercent, progress, total);
                              } });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                        iotHelper.ArduinoOTARunning = false;
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

void setup()
{

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Serial.begin(115200);
  // while (!Serial)
  //   continue;

  iotHelper.Setup();

  iotHelper.wiFiBegin();

  xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, NULL, 1, NULL);

  SetupOTA();

  buttonServerHelper = new ButtonServerHelper();

  tempRecorder = new TempRecorder(&iotHelper);

  displayUpdater = new DisplayUpdater(&iotHelper, tempRecorder);

  webServerHelper = new WebServerHelper();

  iotHelper.SetDisplay(displayUpdater);

  // if (touchRead(TOUCH_PIN) < 50)
  // {
  //   // while (true)
  //   // {

  //   //   safeSerial.println("Touch on boot detected");
  //   //   auto xxx = touchRead(TOUCH_PIN);
  //   //   safeSerial.printf("Touch on boot detected %d\n", xxx);
  //   //   delay(100);
  //   // }

  //   // iotHelper.setSafeBoot();
  // }

  displayUpdater->begin();

  tempRecorder->begin();

  buttonServerHelper->begin(&iotHelper, displayUpdater);

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

  if (iotHelper.ArduinoOTARunning)
  {
    vTaskDelay(pdMS_TO_TICKS(16));
    return;
  }

  lastVal = currentVal;

  buttonServerHelper->updateLoop();

  ArduinoOTA.handle();

  if (digitalRead(BOOT_BUTTON_PIN) == LOW)
  {
    Serial.println("BOOT_BUTTON_PIN");
    // ButtonServerHelper::pushServoButton();
  }

  vTaskDelay(pdMS_TO_TICKS(16));
}
