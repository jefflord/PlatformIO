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
int pressDownHoldTime = 250;
int startAngle = 90;

Servo myServo; // Create a Servo object

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
  mySetup();
  helper.wiFiBegin("DarkNet", "7pu77ies77");

  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  myServo.attach(servoPin);
  myServo.write(startAngle);
}

int lastPotValue = -1;
void loop()
{
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
      Serial.println(mappedPotValue);
      Serial.print("pressDownHoldTime:");
      Serial.println(helper.pressDownHoldTime);

      myServo.write(mappedPotValue);
      delay(helper.pressDownHoldTime);
      myServo.write(startAngle);

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
