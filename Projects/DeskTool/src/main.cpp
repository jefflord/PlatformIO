
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <ESPmDNS.h>

HardwareSerial RadarSerial(1);

#define LED_PIN 2
#define LED_PIN_OFF HIGH
#define LED_PIN_ON LOW

#define TOUCH_PIN 12
#define LD2450C_PIN 23 // D23
#define BOOT_BUTTON_PIN 0

const bool doWiFi = true;

void wiFiBegin();
void sendToggleAction(String action);

void setup()
{
  Serial.begin(115200);

  // The default baud rate of the serial port is
  // 256000, with 1 stop bit and no parity bit.
  RadarSerial.begin(256000, SERIAL_8N1, 16, 17);

  while (!Serial)
    continue;

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(LD2450C_PIN, INPUT);

  if (doWiFi)
  {
    wiFiBegin();

    xTaskCreate([](void *p)
                {
                  vTaskDelay(pdMS_TO_TICKS(2000));
                      if (!MDNS.begin("desktool"))
      {
        Serial.println("Error setting up MDNS responder!");
      }
      else
      {
        Serial.println("Device can be accessed at desktool.local");

        if (MDNS.addService("http", "tcp", 80))
        {
          Serial.println("addService 1 worked");
        }

        // if (MDNS.addService("_http", "_tcp", 80))
        // {
        //   Serial.println("addService 2 worked");
        // }
      vTaskDelete(NULL);
    } }, "mDNS", 1024, NULL, 1, NULL);

    ArduinoOTA.begin();
  }

  if (doWiFi)
  {
    Serial.println("Setup with OTA done.");
  }
}

void parseSensorData(uint8_t *buffer, int length)
{
  // 1. Check for Frame Header
  int offset = 0;
  while (buffer[offset + 0] != 0xAA)
  {
    offset++;
    if (offset >= length)
    {
      Serial.println("No valid frame header.");
      return;
    }
  }

  if (buffer[offset + 0] == 0xAA && buffer[offset + 1] == 0xFF && buffer[offset + 2] == 0x03 && buffer[offset + 3] == 0x00)
  {
    // 2. Iterate through targets (up to 3)
    for (int i = 0; i < 3; i++)
    {
      int targetStart = 4 + (i * 6); // Calculate starting index for each target
      targetStart += offset;

      // 3. Extract Target Data
      int16_t xCoord = (int16_t)(buffer[targetStart] << 8 | buffer[targetStart + 1]);
      int16_t yCoord = (int16_t)(buffer[targetStart + 2] << 8 | buffer[targetStart + 3]);
      int16_t speed = (int16_t)(buffer[targetStart + 4] << 8 | buffer[targetStart + 5]);

      // 4. Print or use the extracted data
      Serial.printf("Target %d: X = %d mm, Y = %d mm, Speed = %d cm/s\n", i + 1, xCoord, yCoord, speed);
    }

    // 5. (Optional) Check for End of Frame
    if (buffer[offset + 20] == 0x55 && buffer[offset + 21] == 0xCC)
    {
      //  Serial.println("End of frame received.");
    }
  }
  else
  {
    Serial.println("Invalid frame header.");
  }
}

int lastReading = -1;
int lastChangeMs = -1;
unsigned long long loopCounter = 0;
void loop()
{

  if (!true)
  {
    // Serial.println("Radar Data Check: ");
    if (RadarSerial.available())
    {
      RadarSerial.setTimeout(100);
      // Serial.println("read...");
      //   Serial.println(RadarSerial.read(), HEX);

      uint8_t buffer[300];                                            // Create a buffer to hold the radar data
      int length = RadarSerial.readBytes(buffer, sizeof(buffer) - 1); // Read the bytes
      // buffer[length] = '\0';                                          // Null-terminate the string
      Serial.printf("Radar Data %d bytes: ", length);

      // uint8_t buffer[300];                                            // Maximum frame size (4 + 3 targets * 6 + 2)
      // int length = RadarSerial.readBytes(buffer, sizeof(buffer) - 1); // Read the bytes
      // parseSensorData(buffer, length);

      int item = 0;
      for (int x = 0; x < length; x++)
      {

        int nextData = x + 1;
        if (x == 85 && nextData < length && buffer[nextData] == 314)
        {
          item++;
          Serial.printf("\n(%d):", item);
        }

        Serial.printf("%X ", buffer[x]);
      }

      // String radarData = RadarSerial.readStringUntil('\0');
      // // String radarData = RadarSerial.readString();
      // if (radarData.length() > 0)
      // {
      //   Serial.println("Received: " + radarData);
      // }
      // else
      // {
      //   // Serial.println("No Data!");
      // }
    }
    else
    {
      // Serial.println("No Radar Data");
    }
  }

  auto ld2450Val = digitalRead(LD2450C_PIN);

  if (lastReading != ld2450Val)
  {
    auto diff = ((millis() - lastChangeMs)) / 1000.0;
    Serial.printf("\nld2450Val %d, (%ld)\n", ld2450Val, diff);
    lastChangeMs = millis();

    if (ld2450Val == 1)
    {
      digitalWrite(LED_PIN, LED_PIN_ON);
      sendToggleAction("light_on");
    }
    else
    {
      digitalWrite(LED_PIN, LED_PIN_OFF);
      sendToggleAction("light_off");
    }
  }

  lastReading = ld2450Val;

  if (lastReading == 1)
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

  if (doWiFi)
  {
    ArduinoOTA.handle();
  }

  if (touchRead(TOUCH_PIN) < 50 || digitalRead(BOOT_BUTTON_PIN) == LOW)
  {

    // if (!MDNS.begin("desktool"))
    // {
    //   Serial.println("Error setting up MDNS responder!");
    // }
    // else
    // {
    //   Serial.println("Device can be accessed at desktool.local");

    //   if (MDNS.addService("http", "tcp", 80))
    //   {
    //     Serial.println("addService 1 worked");
    //   }

    //   // if (MDNS.addService("_http", "_tcp", 80))
    //   // {
    //   //   Serial.println("addService 2 worked");
    //   // }
    // }

    Serial.printf("light_toggle!!!!\n");
    sendToggleAction("light_toggle");
  }

  delay(1000);
  return;
}

void sendToggleAction(String action)
{

  HTTPClient http;

  // Specify the target URL
  http.begin("http://reartvlight.local/doAction");
  // http.begin("http://desktool.local/doAction");

  // Specify content type for JSON
  http.addHeader("Content-Type", "application/json");

  // Create the JSON body
  String jsonBody = "{\"action\":\"" + action + "\"}";

  // Send POST request
  int httpResponseCode = http.POST(jsonBody);

  // Check response code
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  }
  else
  {
    Serial.println("Error on sending POST: " + String(httpResponseCode));
  }

  // End connection
  http.end();
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