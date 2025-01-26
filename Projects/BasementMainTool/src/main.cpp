
#include <Arduino.h>
// #include <ArduinoOTA.h>
// #include <WiFi.h>
// #include <NTPClient.h>
#include <ThreadSafeSerial.h>
#include <base_helpers.h>
#include <NodeCode.h>

#define TOUCH_PIN 12
#define BOOT_BUTTON_PIN 0
#define BJT_PIN 23 // Replace with your desired GPIO pin

OTAStatus *oTAStatus = NULL;
int pinModes[40]; // Assuming GPIOs 0–39 on an ESP32
GlobalState globalState;
auto x_LED_PIN = 1;
auto x_LED_ON = HIGH;
auto x_LED_OFF = LOW;
uint64_t chipId = 0;

ThreadSafeSerial safeSerial(pinModes);

void setPinMode(int pin, int mode)
{
  pinMode(pin, mode);
  pinModes[pin] = mode; // Store the mode
}

int getPinMode(int pin)
{
  return pinModes[pin];
}

void printUint64(uint64_t value)
{
  char buffer[21];                // Sufficient for 20 digits + null terminator
  sprintf(buffer, "%llu", value); // Use %llu for unsigned long long
  safeSerial.print("[");
  safeSerial.print(buffer);
  safeSerial.print("]");
}

Node *meNode = NULL;

void printNodes();

// void watchNodes(void *p)
// {
//   while (true)
//   {
//     vTaskDelay(pdMS_TO_TICKS(10000));
//     printNodes();
//   }
// }

// void simulateGetToken(void *p)
// {
//   while (true)
//   {
//     for (Node *node : globalState.nodes.GetAllNodes())
//     {
//       node->status = 3;
//     }

//     meNode->status = 1;
//     Serial.println("simulateGetToken meNode->status = 1");
//     vTaskDelay(pdMS_TO_TICKS(9000));
//   }
// }

void setup()
{

  WiFi.macAddress(globalState.macAddress);
  meNode = new Node("self", globalState.macAddress, 0, millis());

  globalState.nodes.AddNode(meNode);

  // uint8_t addr1[] = {0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //  uint8_t addr2[] = {0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // globalState.nodes.AddNode(new Node("other 1", addr1, 0, millis()));
  //  globalState.nodes.AddNode(new Node("other 2", addr2, 0, millis()));

  globalState.xBJT_PIN = BJT_PIN;

  Serial.begin(115200); // Hardware Serial for debugging
  while (!Serial)
    continue;

  wiFiBegin();

  chipId = ESP.getEfuseMac(); // Get the unique ID of the chip

  safeSerial.print("Chip ID: ");
  printUint64(chipId);

  if (chipId == 224926312670728)
  {
    x_LED_PIN = 2;
  }
  else if (chipId == 84599911616844)
  {
    x_LED_PIN = 1;
    x_LED_ON = LOW;
    x_LED_OFF = HIGH;
    safeSerial.useBorkMode = true;
  }

  setPinMode(x_LED_PIN, OUTPUT);

  safeSerial.println("GET'S GO!");

  // Initialize the BJT pin as an output
  setPinMode(globalState.xBJT_PIN, OUTPUT);

  // Optional: Start with the BJT off
  digitalWrite(globalState.xBJT_PIN, LOW);

  xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, NULL, 1, NULL);

  // xTaskCreate(simulateGetToken, "simulateGetToken", 1024 * 2, NULL, 1, NULL);
  // xTaskCreate(watchNodes, "watchNodes", 1024 * 4, NULL, 1, NULL);

  oTAStatus = MySetupOTA();

  safeSerial.println("Setup with OTA done.");

  setupServer();

  // Serial2.begin(256000, SERIAL_8N1, 16, 17);  // TX=17, RX=16
  // ld2450_01.begin(Serial2);                   // Pass Serial2 to the library

  // announce myself
  sendEspNodeUpdate(meNode);

  // wait for a bit to get some other announcements
  delay(2000);
}

unsigned long long loopCounter = 0;
auto toggle = true;
bool ledOn = false;
auto lastSentTime = millis();
auto printTime = millis();

void printNodes()
{
  safeSerial.println("#############");
  for (Node *node : globalState.nodes.GetAllNodes())
  {

    safeSerial.printf("Node MAC %s:", convertMacAddressToString(node->macAddress));
    safeSerial.print(", Status:");
    safeSerial.print(node->status);
    safeSerial.print(", feetDeep:");
    safeSerial.printf("%d %s\r\n", node->feetDeep, (node == meNode) ? " (self)" : "");
  }
}

bool announcementToggle = false;
auto checkInterval = 500;
auto lightOnTime = 5000;

void loop()
{

  if (oTAStatus->ArduinoOTARunning)
  {
    ArduinoOTA.handle();
    delay(1);
    return;
  }

  if (millis() - printTime > 1000)
  {
    printTime = millis();
    // printNodes();
  }

  if (millis() - lastSentTime > checkInterval)
  {

    /**
     * status = 0: default
     * status = 1: your turn
     */

    globalState.nodes.DeleteDeadNodes(meNode, 5000 / checkInterval); // 10 second?

    // printNodes();

    auto tokenNode = globalState.nodes.GetNextReady(1);

    auto nodeCount = globalState.nodes.GetAllNodes().size();

    // if (nodeCount == 1)
    // {
    //   safeSerial.println("It's just me!");
    //   // let everyone know it's my turn
    //   meNode->status = 1;
    // }

    if (tokenNode == meNode)
    {
      meNode->lostTokenReminder = 0;
      meNode->feetDeep = 0;
      meNode->lastWorkTime = millis();

      // safeSerial.printf("UPDATE (nodeCount:%d) %s %d, %s %d\r\n", nodeCount, meNode->macAddressAsString(), meNode->status, meNode->macAddressAsString(), meNode->status);
      sendEspNodeUpdate(meNode, meNode);

      // safeSerial.println("doing stuff: ");
      digitalWrite(x_LED_PIN, x_LED_ON);
      safeSerial.println("LED ON");

      auto loopCount = 5;
      for (auto i = 0; i < loopCount; i++)
      {
        // need to say I have the token

        delay(lightOnTime / loopCount);
        meNode->lastWorkTime = millis();

        // remind everyone I am still here
        // safeSerial.printf("UPDATE (nodeCount:%d) %s %d, %s %d", nodeCount, meNode->macAddressAsString(), meNode->status, meNode->macAddressAsString(), meNode->status);
        sendEspNodeUpdate(meNode, meNode);
      }

      digitalWrite(x_LED_PIN, x_LED_OFF);
      safeSerial.println("LED OFF");
      // safeSerial.println("done doing stuff, handing token off");

      // sendEspNodeUpdate(meNode);
      tokenNode = globalState.nodes.SetNextAsReady(meNode);

      if (tokenNode != meNode)
      {
        // safeSerial.printf("it's %s turn now not me %s\r\n", tokenNode->macAddressAsString(), meNode->macAddressAsString());
      }

      // tell the new next node it's their turn
      // safeSerial.printf("NEXT (nodeCount:%d) %s %d, %s %d\r\n", nodeCount, tokenNode->macAddressAsString(), tokenNode->status, tokenNode->macAddressAsString(), tokenNode->status);
      sendEspNodeUpdate(tokenNode, tokenNode);
    }
    else
    {
      tokenNode->feetDeep++;
    }

    announcementToggle = !announcementToggle;
    if (announcementToggle)
    {

      digitalWrite(x_LED_PIN, x_LED_OFF);
      // just send announcement that I am still here.
      // safeSerial.printf("Announcing (nodeCount:%d) %s %d, %s %d\r\n", nodeCount, meNode->macAddressAsString(), meNode->status, tokenNode->macAddressAsString(), tokenNode->status);
      sendEspNodeUpdate(meNode, tokenNode);
    }

    lastSentTime = millis();

    // toggle = !toggle;
    // if (toggle)
    // {
    //   digitalWrite(x_LED_PIN, HIGH);
    //   safeSerial.println("LED ON");
    // }
    // else
    // {
    //   digitalWrite(x_LED_PIN, LOW);
    //   safeSerial.println("LED OFF");
    // }
  }

  // loopCounter++;
  // if ((loopCounter % 50) == 0)
  // {
  //   safeSerial.print("ledOn:");
  //   safeSerial.println(ledOn);

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
