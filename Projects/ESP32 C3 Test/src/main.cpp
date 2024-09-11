#include <Arduino.h>

#include "USBCDC.h"

// USBCDC USBSerial;
#define LED_PIN 2

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  digitalWrite(8, HIGH);
  pinMode(8, OUTPUT);
}

void loop()
{

  Serial.printf("millis %lld\n", millis());
  digitalWrite(8, HIGH);
  delay(1000);
  digitalWrite(8, LOW);
  delay(1000);
}
