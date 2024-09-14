#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>



void setup()
{

  Serial.begin(115200);
  while (!Serial)
    continue;

  WiFi.begin("DarkNet", "7pu77ies77");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("Connected %s\n", WiFi.localIP().toString());
}

void loop()
{
  Serial.println("!!!");
  vTaskDelay(pdMS_TO_TICKS(1000));
  // put your main code here, to run repeatedly:
}
