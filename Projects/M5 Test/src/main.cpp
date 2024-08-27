#include <Arduino.h>
#include "M5StickCPlus2.h"

#define BTN_A 37
#define BTN_B 39
#define BTN_C 35
#define BTN_C 35

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(1);
  StickCP2.Display.setTextColor(GREEN);
  StickCP2.Display.setTextDatum(middle_center);
  StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
  StickCP2.Display.setTextSize(1);

  // pinMode(BTN_A, INPUT);
  // pinMode(BTN_B, INPUT);
  // pinMode(BTN_C, INPUT);
}

void loop()
{

  StickCP2.Display.clear();
  int vol = StickCP2.Power.getBatteryVoltage();
  StickCP2.Display.setCursor(10, 30);
  StickCP2.Display.printf("BAT: %dmv", vol);
  delay(1000);

  int buttonAState = digitalRead(BTN_A);
  int buttonBState = digitalRead(BTN_B);
  int buttonCState = digitalRead(BTN_C);

  if (buttonAState == LOW)
  {
    Serial.println("Button A Pressed!");
  }

  if (buttonBState == LOW)
  {
    Serial.println("Button B Pressed!");
  }

  if (buttonCState == LOW)
  {
    Serial.println("Button C Pressed!");
  }

  // delay(33);
}
