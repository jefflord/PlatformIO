#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <esp_now.h>

#define LED_PIN 2
#define TOUCH_PIN 12
#define BOOT_BUTTON_PIN 0

// Structure to send data
typedef struct struct_message
{
  char a[32] = "Hello ESP-NOW!";
  int b = 42;
  float c = 3.14;
} struct_message;

// Create a struct_message
struct_message myData;

// Peer MAC address
// DeskTool: 88:13:BF:07:AC:A4
// uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0x88, 0x13, 0xBF, 0x07, 0xAC, 0xA4};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

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

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer  
  WiFi.mode(WIFI_STA);
  delay(20);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      Serial.println("Failed to add peer:");

      for (int i = 0; i < 6; i++)
      {
        Serial.print(peerInfo.peer_addr[i], HEX); // Print each byte in hex
        if (i < 5)
          Serial.print(":"); // Add colon between bytes
      }
    }
  }

  // if (esp_now_add_peer(&peerInfo) != ESP_OK)
  // {
  //   Serial.println("Failed to add peer");
  //   delay(1000);
  //   if (esp_now_add_peer(&peerInfo) != ESP_OK)
  //   {
  //     Serial.println("Failed to add peer");
  //   }
  // }

  ArduinoOTA.begin();
}

unsigned long long loopCounter = 0;
auto lastTouchTime = millis();
void loop()
{

  // auto mod = loopCounter++ % 10;

  // Serial.printf("loopCounter %lld\n", loopCounter);

  if ((millis() - lastTouchTime > 2000) && (touchRead(TOUCH_PIN) < 50 || digitalRead(BOOT_BUTTON_PIN) == LOW))
  {
    lastTouchTime = millis();
    Serial.println("touched!");

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK)
    {
      Serial.println("Sent with success");
    }
    else
    {
      Serial.println("Error sending the data");
    }
  }

  ArduinoOTA.handle();
  delay(16);
}

void wiFiBegin()
{
  // Try to connect to the saved WiFi credentials

  WiFi.mode(WIFI_STA);
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

  Serial.printf("\nWiFi Connected %s, %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());

  WiFiUDP ntpUDP;
  NTPClient *timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
  timeClient->begin();

  auto updated = timeClient->update();

  Serial.print("Current time: ");
  Serial.println(timeClient->getFormattedTime());
}