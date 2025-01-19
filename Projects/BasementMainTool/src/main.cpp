
#include <Arduino.h>
// #include <ArduinoOTA.h>
// #include <WiFi.h>
// #include <NTPClient.h>

#include <base_helpers.h>

#define LED_PIN 2
#define TOUCH_PIN 12
#define BOOT_BUTTON_PIN 0



OTAStatus *oTAStatus = NULL;




GlobalState globalState;


void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  wiFiBegin();

  xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, NULL, 1, NULL);

  oTAStatus = MySetupOTA();

  Serial.println("Setup with OTA done.");

  setupServer();


  // Serial2.begin(256000, SERIAL_8N1, 16, 17);  // TX=17, RX=16
  // ld2450_01.begin(Serial2);                   // Pass Serial2 to the library
}

unsigned long long loopCounter = 0;
void loop()
{

  if (oTAStatus->ArduinoOTARunning)
  {
    ArduinoOTA.handle();
    delay(1);
    return;
  }


  ArduinoOTA.handle();
  delay(100);
}
