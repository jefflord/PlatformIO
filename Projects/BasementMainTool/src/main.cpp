
#include <Arduino.h>
// #include <ArduinoOTA.h>
// #include <WiFi.h>
// #include <NTPClient.h>

#include <base_helpers.h>

#define LED_PIN 2
#define TOUCH_PIN 12
#define BOOT_BUTTON_PIN 0
#define BJT_PIN 23 // Replace with your desired GPIO pin

OTAStatus *oTAStatus = NULL;

GlobalState globalState;

void setup()
{

  globalState.xBJT_PIN = BJT_PIN;

  Serial.begin(115200);
  while (!Serial)
    continue;

  // Initialize the BJT pin as an output
  pinMode(globalState.xBJT_PIN, OUTPUT);

  // Optional: Start with the BJT off
  digitalWrite(globalState.xBJT_PIN, LOW);

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
bool ledOn = false;
auto lastSentTime = millis();
void loop()
{

  if (oTAStatus->ArduinoOTARunning)
  {
    ArduinoOTA.handle();
    delay(1);
    return;
  }

  if (millis() - lastSentTime > 5000)
  {
    lastSentTime = millis();
    sendEspNowAction("test");
  }

  // loopCounter++;
  // if ((loopCounter % 50) == 0)
  // {
  //   Serial.print("ledOn:");
  //   Serial.println(ledOn);

  //   if (ledOn)
  //   {
  //     digitalWrite(globalState.xBJT_PIN, HIGH);
  //   }
  //   else
  //   {
  //     digitalWrite(globalState.xBJT_PIN, LOW);
  //   }

  //   ledOn = !ledOn;
  // }

  ArduinoOTA.handle();
  delay(100);
}
