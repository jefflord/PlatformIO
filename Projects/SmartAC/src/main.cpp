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

const int button1Pin = 2; // update config
const int button2Pin = 4; // kill wifi

const int servoPin = 32;
const int potPin = 34;

int button1State = 0;
int button2State = 0;

int potValue = 0;

// int pressDownHoldTime = 250;
// int startAngle = 90;

Servo myServo; // Create a Servo object

#define ONE_WIRE_BUS 19
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/**/
const int numReadings = 10; // Number of readings to average
int readings[numReadings];  // Array to store the readings
int readIndex = 0;          // Index of the current reading
int total = 0;

MyIoTHelper helper("SmartAC");

/**/

bool btn1IsDown = false;
bool btn2IsDown = true;

void showStartReason()
{
  esp_reset_reason_t reason = esp_reset_reason();

  Serial.print("Reset reason: ");
  long delayMs = 0;

  switch (reason)
  {
  case ESP_RST_POWERON:
    Serial.println("Power on reset");
    break;
  case ESP_RST_EXT:
    Serial.println("External reset");
    break;
  case ESP_RST_SW:
    Serial.println("Software reset");
    break;
  case ESP_RST_PANIC:
    Serial.println("Exception/panic reset");
    delayMs = 1000;
    break;
  case ESP_RST_INT_WDT:
    Serial.println("Interrupt watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("Task watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_WDT:
    Serial.println("Other watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_DEEPSLEEP:
    Serial.println("Deep sleep reset");
    break;
  case ESP_RST_BROWNOUT:
    Serial.println("Brownout reset");
    break;
  case ESP_RST_SDIO:
    Serial.println("SDIO reset");
    break;
  default:
    Serial.println("Unknown reset reason");
    break;
  }

  // delay(delayMs);
}
void setup()
{
  helper.Setup();

  showStartReason();

  helper.wiFiBegin("DarkNet", "7pu77ies77");
  sensors.begin(); // Start the DS18B20 sensor

  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  myServo.attach(servoPin);

  Serial.printf("helper.servoHomeAngle: %d\n", helper.servoHomeAngle);
  myServo.write(helper.servoHomeAngle);
}

int lastPotValue = -1;

unsigned long previousTempMillis = 0;      // Store the last time doTemp() was executed
unsigned long previousTempFlushMillis = 0; // Store the last time flusht to Db

bool doTempStarted = false;

// String formatDeviceAddress(DeviceAddress deviceAddress)
// {
//   String address = "";
//   for (int i = 0; i < 8; i++)
//   {
//     address += String(deviceAddress[i], HEX);
//     if (i < 7)
//     {
//       address += " ";
//     }
//   }
//   return address;
// }

String formatDeviceAddress(DeviceAddress deviceAddress)
{
  String address = "";
  for (int i = 0; i < 8; i++)
  {
    char hexByte[3];                            // Buffer to hold two hex digits + null terminator
    sprintf(hexByte, "%02X", deviceAddress[i]); // Format with zero padding
    address += hexByte;
    if (i < 7)
    {
      address += " ";
    }
  }
  return address;
}

// int64_t currentTime = 0;
// int64_t startMills = millis();
// int64_t timeLastChech = millis();

// void updateTime(void *p)
// {
//   for (;;)
//   {
//     Serial.print("A-");
//     Serial.printf("currentTime %lld, upTime %lld\n", currentTime, (millis() - startMills));
//     currentTime = helper.getTime();
//     Serial.print("B-");
//     Serial.printf("currentTime %lld, upTime %lld\n", currentTime, (millis() - startMills));
//     vTaskDelay(pdMS_TO_TICKS(1000)); // Convert milliseconds to ticks
//   }
// }

void _doTemp(void *parameter)
{
  long flushCount = 0;
  while (true)
  {
    // Serial.println("_doTemp!!!");
    auto currentMillis = millis();
    // Serial.print("doTemp() ");
    sensors.requestTemperatures();

    for (int i = 0; i < sensors.getDeviceCount(); i++)
    {
      DeviceAddress deviceAddress;
      sensors.getAddress(deviceAddress, i);
      auto sensorId = formatDeviceAddress(deviceAddress);
      // Serial.print("Sensor ");
      // Serial.printf("%d, %s", i, sensorId.c_str());
      // Serial.print(": ");
      // Serial.println(sensors.getTempC(deviceAddress));

      float temperatureC = sensors.getTempC(deviceAddress);
      auto time = helper.getTime();
      auto itemCount = helper.recordTemp(sensorId, time, temperatureC);

      // Serial.printf("'%s': %d items, time: %lld, temperature: %f\n", sensorId.c_str(), itemCount, time, temperatureC);

      // Serial.printf("'%s': %d items, %s\n", sensorId.c_str(), itemCount, helper.getStorageAsJson(helper.getSourceId(sensorId)).c_str());
    }

    // if (false)
    // {
    //   Serial.print(temperatureC);
    //   Serial.print(" -- ");
    //   Serial.print(temperatureF);
    //   Serial.print(" -- ");
    //   Serial.print(time);
    //   Serial.print(" -- ");
    //   Serial.print(itemCount);
    //   Serial.println();
    // }

    if (currentMillis - previousTempFlushMillis >= (helper.tempFlushIntevalSec * 1000))
    {

      previousTempFlushMillis = currentMillis;
      Serial.printf("FLUSH TIME %ld\n", ++flushCount);

      for (int i = 0; i < sensors.getDeviceCount(); i++)
      {
        DeviceAddress deviceAddress;
        sensors.getAddress(deviceAddress, i);
        auto sensorId = formatDeviceAddress(deviceAddress);
        auto itemCount = helper.getRecordCount(sensorId);
        Serial.printf("'%s': %d items\n", sensorId.c_str(), itemCount);
      }

      helper.flushAllDatatoDB();
    }

    vTaskDelay(pdMS_TO_TICKS(helper.tempReadIntevalSec * 1000)); // Convert milliseconds to ticks
  }
}

void doTemp()
{

  if (!doTempStarted)
  {

    Serial.print("xPortGetFreeHeapSize: ");
    Serial.println(xPortGetFreeHeapSize());

    doTempStarted = true;

    // for (;;)
    // {

    //   Serial.print("A-");
    //   Serial.printf("currentTime %lld, upTime %lld\n", currentTime, (millis() - startMills));

    //   if (millis() - timeLastChech > 10000)
    //   {

    //     delete helper.timeClient;

    //     WiFiUDP ntpUDP;
    //     helper.timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60000); // Time offset in seconds and update interval
    //     helper.timeClient->begin();
    //     auto updated = helper.timeClient->update();
    //     Serial.printf("update %s\n", updated ? "true" : "false");
    //     timeLastChech = millis();
    //   }

    //   currentTime = helper.timeClient->getEpochTime() * 1000;
    //   Serial.print("B-");
    //   Serial.printf("currentTime %lld, upTime %lld\n", currentTime, (millis() - startMills));
    //   vTaskDelay(pdMS_TO_TICKS(1000)); // Convert milliseconds to ticks
    // }

    // xTaskCreate(
    //     updateTime,   // Function to run on the new thread
    //     "updateTime", // Name of the task (for debugging)
    //     1024 * 2,    // Stack size (in bytes) // 8192
    //     NULL,         // Parameter passed to the task
    //     1,            // Priority (0-24, higher number means higher priority)
    //     NULL          // Handle to the task (not used here)
    // );

    xTaskCreate(
        _doTemp,  // Function to run on the new thread
        "doTemp", // Name of the task (for debugging)
        8192 * 2, // Stack size (in bytes) // 8192
        NULL,     // Parameter passed to the task
        1,        // Priority (0-24, higher number means higher priority)
        NULL      // Handle to the task (not used here)
    );
  }
}

void loop()
{

  auto startTime = micros();

  doTemp();

  // Read the state of the button
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);

  // Serial.printf("btn1 %d -- btn2 %d\n", button1State, button2State);
  if (button2State == HIGH)
  {
    // Serial.println("btn2 low");
    btn2IsDown = false;
  }
  else
  {

    if (!btn2IsDown)
    {
      Serial.println("btn2 down!!!!!");
      helper.chaos("wifi");
      btn2IsDown = true;
    }
  }

  //   if (button1State == HIGH)
  //   {
  //     // Serial.println("btn2 low");
  //     btn1IsDown = false;
  //   }
  //   else
  //   {

  //     if (!btn1IsDown)
  //     {
  //       Serial.println("btn1 down!!!!");
  //     }
  //     btn1IsDown = true;
  //   }

  if (button1State == LOW)
  {
    if (!btn1IsDown)
    {

      helper.updateConfig();

      Serial.print("angle:");
      Serial.println(helper.servoAngle);
      Serial.print("pressDownHoldTime:");
      Serial.println(helper.pressDownHoldTime);

      myServo.write(helper.servoAngle);
      delay(helper.pressDownHoldTime);
      myServo.write(helper.servoHomeAngle);

      btn1IsDown = true;
    }
  }
  else
  {
    btn1IsDown = false;
    // Serial.print("pot:");
    // Serial.print(potValue);
    // Serial.println();
    // Serial.println("Button not pressed.");
  }

  auto endTime = micros();
  auto timeTaken = endTime - startTime;
  if (timeTaken > 16666)
  {
    return;
  }
  delayMicroseconds(16666 - timeTaken); // Small delay to avoid bouncing issues
}
