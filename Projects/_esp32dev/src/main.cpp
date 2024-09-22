
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <NTPClient.h>

#define LED_PIN 2
#define TOUCH_PIN 12
#define BOOT_BUTTON_PIN 0

bool defaultState = true;

void wiFiBegin();

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  wiFiBegin();

  xTaskCreate([](void *p)
              {
                while (defaultState)
                {
                  digitalWrite(LED_PIN, HIGH);
                  vTaskDelay(pdMS_TO_TICKS(250));
                  digitalWrite(LED_PIN, LOW);
                  vTaskDelay(pdMS_TO_TICKS(250));
                } 
                vTaskDelete(NULL); },
              "flash", 2048, NULL, 1, NULL);

  ArduinoOTA.begin();
  Serial.println("Setup with OTA done.");
}

unsigned long long loopCounter = 0;
void loop()
{

  // auto mod = loopCounter++ % 10;

  bool showMessage = ((loopCounter++ % 60) == 0);

  // Serial.printf("loopCounter %lld\n", loopCounter);

  if (defaultState)
  {
    if (showMessage)
    {
      Serial.printf("Waiting to be touched on BOOT button or PIN %d...\n", TOUCH_PIN);
    }
  }
  else
  {
    if (showMessage)
    {
      Serial.printf("I've been touched on BOOT button or PIN %d...\n", TOUCH_PIN);
    }
  }

  if (touchRead(TOUCH_PIN) < 50 || digitalRead(BOOT_BUTTON_PIN) == LOW)
  {
    defaultState = false;
  }

  ArduinoOTA.handle();
  delay(16);
}

void wiFiBegin()
{
  // Try to connect to the saved WiFi credentials

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin();
  }

  // Wait for connection
  int timeout = 20000; // Timeout after 10 seconds
  int elapsed = 0;
  Serial.println("\nStarting WiFi...");

  // flash

  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    delay(1000);
    Serial.print("O");
    elapsed += 1000;
  }

  // If not connected, start SmartConfig
  if (WiFi.status() != WL_CONNECTED)
  {

    Serial.println("\nStarting SmartConfig...");
    WiFi.beginSmartConfig();

    timeout = 2 * 60 * 1000;
    elapsed = 0;

    // Wait for SmartConfig to finish
    while (!WiFi.smartConfigDone() && elapsed < timeout)
    {
      delay(1000);
      Serial.print("X");
      elapsed += 1000;
    }

    if (!WiFi.smartConfigDone())
    {
      Serial.println("smartConfigDone failed, rebooting.");
      ESP.restart();
    }

    Serial.println("\nSmartConfig done.");
    // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
  }

  Serial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  WiFiUDP ntpUDP;
  NTPClient *timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
  timeClient->begin();

  auto updated = timeClient->update();

  Serial.print("Current time: ");
  Serial.println(timeClient->getFormattedTime());
}