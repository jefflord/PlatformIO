#include <Arduino.h>

#include "USBCDC.h"

// USBCDC USBSerial;
#define LED_PIN 2

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

}

void loop()
{

  Serial.println(millis());
  delay(1000);
}
