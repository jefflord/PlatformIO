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

void setup()
{

  helper.Setup();

  tempRecorder = new TempRecorder(&helper);
  displayUpdater = new DisplayUpdater(&helper, tempRecorder);
  displayUpdater->begin();

  helper.wiFiBegin("DarkNet", "7pu77ies77");

  
  tempRecorder->begin();


  // pinMode(button1Pin, INPUT_PULLUP);
  // pinMode(button2Pin, INPUT_PULLUP);
  // myServo.attach(servoPin);

  // Serial.printf("helper.servoHomeAngle: %d\n", helper.servoHomeAngle);
  // myServo.write(helper.servoHomeAngle);
}

int lastPotValue = -1;

void loop()
{

  // // Read the state of the button
  // button1State = digitalRead(button1Pin);
  // button2State = digitalRead(button2Pin);

  // // Serial.printf("btn1 %d -- btn2 %d\n", button1State, button2State);
  // if (button2State == HIGH)
  // {
  //   // Serial.println("btn2 low");
  //   btn2IsDown = false;
  // }
  // else
  // {

  //   if (!btn2IsDown)
  //   {
  //     Serial.println("btn2 down!!!!!");
  //     helper.chaos("wifi");
  //     btn2IsDown = true;
  //   }
  // }

  // //   if (button1State == HIGH)
  // //   {
  // //     // Serial.println("btn2 low");
  // //     btn1IsDown = false;
  // //   }
  // //   else
  // //   {

  // //     if (!btn1IsDown)
  // //     {
  // //       Serial.println("btn1 down!!!!");
  // //     }
  // //     btn1IsDown = true;
  // //   }

  // if (button1State == LOW)
  // {
  //   if (!btn1IsDown)
  //   {

  //     helper.updateConfig();

  //     Serial.print("angle:");
  //     Serial.println(helper.servoAngle);
  //     Serial.print("pressDownHoldTime:");
  //     Serial.println(helper.pressDownHoldTime);

  //     myServo.write(helper.servoAngle);
  //     delay(helper.pressDownHoldTime);
  //     myServo.write(helper.servoHomeAngle);

  //     btn1IsDown = true;
  //   }
  // }
  // else
  // {
  //   btn1IsDown = false;
  //   // Serial.print("pot:");
  //   // Serial.print(potValue);
  //   // Serial.println();
  //   // Serial.println("Button not pressed.");
  // }

  // auto endTime = micros();
  // auto timeTaken = endTime - startTime;
  // if (timeTaken > 16666)
  // {
  //   return;
  // }
  // delayMicroseconds(16666 - timeTaken); // Small delay to avoid bouncing issues
}
