#include <Arduino.h>
#include <WiFi.h>

void setup()
{

  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.begin(115200);

  // Try to connect to the saved WiFi credentials
  WiFi.begin();

  // Wait for connection
  int timeout = 10000; // Timeout after 10 seconds
  int elapsed = 0;
  Serial.println("\nStarting WiFi...");
  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    delay(500);
    Serial.print(".");
    elapsed += 500;
  }

  // If not connected, start SmartConfig
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nStarting SmartConfig...");
    WiFi.beginSmartConfig();

    // Wait for SmartConfig to finish
    while (!WiFi.smartConfigDone())
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println("\nSmartConfig done.");
    // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
  }
  else
  {
    // Serial.printf("Connected to saved WiFi: %s\n", WiFi.SSID().c_str());
  }

  Serial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

int lastPotValue = -1;

void loop()
{

  Serial.println("");
  Serial.printf("WiFi %d\n", WiFi.status());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  delay(1000);
}
