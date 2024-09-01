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

MyIoTHelper helper;

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

unsigned long previousTempMillis = 0; // Store the last time doTemp() was executed

void doTemp()
{

  unsigned long currentMillis = millis();

  if (currentMillis - previousTempMillis >= (helper.tempReadIntevalSec * 1000))
  {
    // Save the last time doTemp() was run
    previousTempMillis = currentMillis;

    // Call the function

    Serial.print("doTemp() ");
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    float temperatureF = sensors.getTempFByIndex(0);

    auto time = helper.getTime();

    auto itemCount = helper.recordTemp("mytemp1", time, temperatureC);

    Serial.printf("items:  %d, temperature: %f\n", itemCount, temperatureC);

    Serial.print(temperatureC);
    Serial.print(" -- ");
    Serial.print(temperatureF);
    Serial.print(" -- ");
    Serial.print(time);
    Serial.print(" -- ");
    Serial.print(itemCount);
    Serial.println();

    if (itemCount >= 2)
    {
      Serial.println();
      Serial.println();
      Serial.println(helper.getStorageAsJson().c_str());
      Serial.println();
      helper.clearSource("mytemp1");
    }
  }
}

void loop()
{

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

  potValue = updateAverage(analogRead(potPin));
  int mappedPotValue = map(updateAverage(potValue), 0, 4095, 0, 180); // Map to 0-180 degrees
  if (lastPotValue != mappedPotValue)
  {
    Serial.println(mappedPotValue);
    lastPotValue = mappedPotValue;
  }

  // delay(100);
  // return;

  if (button1State == LOW)
  {
    if (!btn1IsDown)
    {

      helper.updateConfig("SmartAC");

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

  delay(33); // Small delay to avoid bouncing issues
}
