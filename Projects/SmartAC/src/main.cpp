/*
Notes:
  - Temp senor needs 5v and 4.6kΩR (pull-up I think) 
  - Activation button has a extra 10kΩR (pull-up)
*/

#include <Arduino.h>
#include <ESP32Servo.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
#include "helper.h"

const int button1Pin = 33;
const int button2Pin = 39;

const int servoPin = 32;
const int potPin = 34;

int button1State = 0;
int button2State = 0;

int potValue = 0;

// int pressDownHoldTime = 250;
// int startAngle = 90;

Servo myServo; // Create a Servo object

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/**/
const int numReadings = 10; // Number of readings to average
int readings[numReadings];  // Array to store the readings
int readIndex = 0;          // Index of the current reading
int total = 0;

MyIoTHelper helper("SmartAC");

int updateAverage(int nextValue)
{
  // Subtract the last reading from the total
  total -= readings[readIndex];
  // Add the new value to the total
  readings[readIndex] = nextValue;
  total += nextValue;
  // Advance to the next index
  readIndex = (readIndex + 1) % numReadings;

  // Return the average
  return total / numReadings;
}

/**/

bool btn1IsDown = false;
bool btn2IsDown = true;

void setup()
{
  helper.Setup();
  helper.wiFiBegin("DarkNet", "7pu77ies77");
  sensors.begin(); // Start the DS18B20 sensor

  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  myServo.attach(servoPin);
  myServo.write(helper.servoHomeAngle);
}

int lastPotValue = -1;

unsigned long previousTempMillis = 0;      // Store the last time doTemp() was executed
unsigned long previousTempFlushMillis = 0; // Store the last time flusht to Db

bool doTempStarted = false;

void _doTemp(void *parameter)
{
  while (true)
  {
    //Serial.println("_doTemp!!!");
    auto currentMillis = millis();
    // Serial.print("doTemp() ");
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    auto time = helper.getTime();
    auto itemCount = helper.recordTemp("mytemp1", time, temperatureC);

    // Serial.printf("items:  %d, temperature: %f\n", itemCount, temperatureC);

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
      Serial.println("FLUSH TIME");
      helper.flushDatatoDB();
    }

    vTaskDelay(pdMS_TO_TICKS(helper.tempReadIntevalSec * 1000)); // Convert milliseconds to ticks
  }
}

void doTemp()
{

  if (!doTempStarted)
  {
    doTempStarted = true;
    xTaskCreate(
        _doTemp,  // Function to run on the new thread
        "doTemp", // Name of the task (for debugging)
        8192,     // Stack size (in bytes)
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

  if (false)
  {
    potValue = updateAverage(analogRead(potPin));
    int mappedPotValue = map(updateAverage(potValue), 0, 4095, 0, 180); // Map to 0-180 degrees
    if (lastPotValue != mappedPotValue)
    {
      Serial.println(mappedPotValue);
      lastPotValue = mappedPotValue;
    }
  }

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
