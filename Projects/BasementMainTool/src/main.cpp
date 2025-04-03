
#include <Arduino.h>
#include <ThreadSafeSerial.h>
#include <base_helpers.h>
#include <NodeCode.h>

/********************/
#include <SPI.h>

#ifdef ILI9341_2_DRIVER
// CYD touchscreen SPI pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Button sizes
#define BUTTON_W 50
#define BUTTON_H 50

#define LDR_PIN 34
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// XPT2046_Touchscreen ts(XPT2046_CS);
TFT_eSPI tft = TFT_eSPI();

ButtonWidget btn1 = ButtonWidget(&tft);
ButtonWidget btn2 = ButtonWidget(&tft);
ButtonWidget btn3 = ButtonWidget(&tft);
ButtonWidget btn4 = ButtonWidget(&tft);
MeterWidget lum = MeterWidget(&tft);

// Array of button instances to use in for() loops
ButtonWidget *btn[] = {&btn1, &btn2, &btn3, &btn4};
#endif

unsigned long ldrCheckDue = 0;
int ldrDelay = 1000;

#ifdef ILI9341_2_DRIVER

uint8_t buttonCount = sizeof(btn) / sizeof(btn[0]);

//    btn2.drawSmoothButton(!btn2.getState(), 3, TFT_BLACK, btn2.getState() ? "OFF" : "ON");
//    Serial.print("Button toggled: ");

//    btn2.setPressTime(millis());
auto btnPressedDelay = 30;

void btn1_pressAction(void)
{
  Serial.println(btn1.getState() ? "B1 turned OFF" : "B1 turned ON");
  btn1.drawSmoothButton(!btn1.getState(), 3, TFT_BLACK, btn1.getState() ? "B1 OFF" : "B1 ON");
  // if(!btn1.getState()) drawLDRValue(0);
  delay(btnPressedDelay);
}

void btn2_pressAction(void)
{
  Serial.println(btn2.getState() ? "B2 turned OFF" : "B2 turned ON");
  btn2.drawSmoothButton(!btn2.getState(), 3, TFT_BLACK, btn2.getState() ? "B2 OFF" : "B2 ON");
  delay(btnPressedDelay);
}

void btn3_pressAction(void)
{
  Serial.println(btn3.getState() ? "B3 turned OFF" : "B3 turned ON");
  btn3.drawSmoothButton(!btn3.getState(), 3, TFT_BLACK, btn3.getState() ? "B3 OFF" : "B3 ON");
  delay(btnPressedDelay);
}

void btn4_pressAction(void)
{
  Serial.println(btn4.getState() ? "B4 turned OFF" : "B4 turned ON");
  btn4.drawSmoothButton(!btn4.getState(), 3, TFT_BLACK, btn4.getState() ? "B4 OFF" : "B4 ON");
  delay(btnPressedDelay);
}

void initButtons()
{
  // uint16_t x = (tft.width() - BUTTON_W) / 2;
  uint16_t x = 10;
  uint16_t y = 5;
  btn1.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_RED, "B1 OFF", 1);
  btn1.setPressAction(btn1_pressAction);
  btn1.drawSmoothButton(false, 3, TFT_BLACK);

  y = 65;
  btn2.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_RED, "B2 OFF", 1);
  btn2.setPressAction(btn2_pressAction);
  btn2.drawSmoothButton(false, 3, TFT_BLACK);

  y = 125;
  btn3.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_RED, "B3 OFF", 1);
  btn3.setPressAction(btn3_pressAction);
  btn3.drawSmoothButton(false, 3, TFT_BLACK);

  y = 185;
  btn4.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_RED, "B4 OFF", 1);
  btn4.setPressAction(btn4_pressAction);
  btn4.drawSmoothButton(false, 3, TFT_BLACK);
}

void drawLDRValue(int sensorValue)
{
  int x = 180;
  int y = 5;
  int fontSize = 4;
  if (btn1.getState())
  {
    char fmtSV[5];
    snprintf(fmtSV, sizeof(fmtSV), "%04d", sensorValue);
    tft.drawString("LDR: " + String(fmtSV), x, y, fontSize);
  }
  else
  {
    tft.drawString("LDR: OFF  ", x, y, fontSize);
  }
}
#endif
/********************/
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

void node_setup();
void node_loop();
void command_setup();
void command_loop();

void setup()
{

  Serial.begin(115200); // Hardware Serial for debugging
  while (!Serial)
    continue;

  chipId = ESP.getEfuseMac(); // Get the unique ID of the chip

  safeSerial.print("Chip ID: ");
  printUint64(chipId);

  if (chipId != 163551045461512)
  {
    globalState.isNode = true;
  }

  if (globalState.isNode)
  {
    WiFi.macAddress(globalState.macAddress);
    wiFiBegin();    
    oTAStatus = MySetupOTA();
    node_setup();
    return;
  }
  else
  {
    WiFi.macAddress(globalState.macAddress);
    // wiFiBegin();
    // oTAStatus = MySetupOTA();
    command_setup();
  }
}

#ifdef ILI9341_2_DRIVER

TFT_eSPI_Button key[6];
void drawButtons()
{
  uint16_t bWidth = TFT_HEIGHT / 3;
  uint16_t bHeight = TFT_WIDTH / 2;
  // Generate buttons with different size X deltas
  for (int i = 0; i < 6; i++)
  {
    key[i].initButton(&tft,
                      bWidth * (i % 3) + bWidth / 2,
                      bHeight * (i / 3) + bHeight / 2,
                      bWidth,
                      bHeight,
                      TFT_BLACK, // Outline
                      TFT_BLUE,  // Fill
                      TFT_BLACK, // Text
                      "",
                      1);

    key[i].drawButton(false, String(i + 1));
  }
}
#endif
void command_setup()
{
#ifdef ILI9341_2_DRIVER

  safeSerial.println("command_setup");
  
  auto myDnsName = "btcommand";
  // xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, (void *)myDnsName, 1, NULL);

  // safeSerial.printf("TFT_MOSI %d \r\n", TFT_MOSI);

  tft.init();
  tft.setRotation(1); // This is the display in landscape

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  Serial.println("Initialising...");
  pinMode(LDR_PIN, INPUT);
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);

  // ts.begin();
  ts.begin(mySpi);

  tft.init();
  tft.invertDisplay(1);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  initButtons();
  lum.setZones(75, 100, 50, 75, 25, 50, 0, 25);
  lum.analogMeter(70, 5, 2000, "Lum", "0", "500", "1000", "1500", "2000");
  // drawLDRValue(0);

  return;
  drawButtons();
  return;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int x = 5;
  int y = 10;
  int fontNum = 2;
  tft.drawString("Hello", x, y, fontNum); // Left Aligned
  x = 320 / 2;
  y += 16;
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawCentreString("World", x, y, fontNum);
#endif
}

void node_setup()
{

  safeSerial.println("node_setup");

  meNode = new Node("self", globalState.macAddress, 0, 0);

  globalState.nodes.AddNode(meNode);

  // uint8_t addr1[] = {0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //  uint8_t addr2[] = {0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // globalState.nodes.AddNode(new Node("other 1", addr1, 0, millis()));
  //  globalState.nodes.AddNode(new Node("other 2", addr2, 0, millis()));

  globalState.xBJT_PIN = BJT_PIN;

  if (chipId == 84599911616844)
  {
    // the weird one
    x_LED_PIN = 1;
    x_LED_ON = LOW;
    x_LED_OFF = HIGH;
    safeSerial.useBorkMode = true;
  }
  else
  {
    x_LED_PIN = 2;
  }

  setPinMode(x_LED_PIN, OUTPUT);

  // safeSerial.print("chipId: ");
  // safeSerial.println(chipId);

  safeSerial.println("GET'S GO!");

  // Initialize the BJT pin as an output
  setPinMode(globalState.xBJT_PIN, OUTPUT);

  // Optional: Start with the BJT off
  digitalWrite(globalState.xBJT_PIN, LOW);

  // get max for dns name
  auto macStr = "esp" + String(WiFi.macAddress().c_str());

  // now remove the colons from macStr
  macStr.replace(":", "");

  xTaskCreate(mDNSSetup, "mDNSSetup", 1024 * 2, (void *)macStr.c_str(), 1, NULL);

  // xTaskCreate(simulateGetToken, "simulateGetToken", 1024 * 2, NULL, 1, NULL);
  // xTaskCreate(watchNodes, "watchNodes", 1024 * 4, NULL, 1, NULL);

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

bool announcementToggle = true;
auto checkInterval = 100;
auto lightOnTime = 3000;

// auto lastWorkTime = millis() - millis();

void loop()
{

  if (oTAStatus != NULL && oTAStatus->ArduinoOTARunning)
  {
    ArduinoOTA.handle();
    delay(1);
    return;
  }

  if (globalState.isNode)
  {
    node_loop();
    delay(100);
  }
  else
  {
    command_loop();
  }

  if (oTAStatus != NULL && oTAStatus->ArduinoOTARunning)
  {
    ArduinoOTA.handle();
  }
}

static uint32_t now = millis();

void touch_loop()
{
#ifdef ILI9341_2_DRIVER
  if (millis() - now >= 33)
  {

    // if (ts.touched())
    // {
    //   Serial.println("you touched the screen");
    // }

    // if (ts.tirqTouched())
    // {
    //   Serial.println("you tirqTouched the screen");
    // }

    now = millis();

    if (ts.tirqTouched() && ts.touched())
    {
      Serial.printf("CHECKING TOUCH %s\r\n", (ts.tirqTouched() && ts.touched()) ? " touching!" : "");

      TS_Point p = ts.getPoint();
      for (uint8_t b = 0; b < buttonCount; b++)
      {
        // Rough calibration:
        // x: 3800 / 320 = 11.875
        // y: 3800 / 240 = 15.83
        if (btn[b]->contains((p.x / 11.875), (p.y / 15.83)))
        {
          btn[b]->press(true);
          btn[b]->pressAction();
        }
      }
    }
  }

  if (now > ldrCheckDue && btn1.getState())
  {

    analogSetAttenuation(ADC_0db);
    // auto value = analogReadMilliVolts(CDS);
    int sensorValue = analogRead(LDR_PIN);
    // Serial.printf("LDR CHECK %d -> ", LDR_PIN);
    // Serial.printf("LDR: mv:%d, read: %u\r\n", sensorValue, value);
    // value is number from 0 to 330

    sensorValue = 2000 + (-1 * sensorValue);
    // auto randomValue = random(0, 2000);
    lum.updateNeedle(sensorValue, 45);

    ldrCheckDue = now + ldrDelay;
  }

  if (false)
  {
    if (now > ldrCheckDue && btn1.getState())
    {

      for (int lastLDRPinCheck = 1; lastLDRPinCheck < 40; lastLDRPinCheck++)
      {

        if (lastLDRPinCheck != 1 && lastLDRPinCheck != 3

            && lastLDRPinCheck != 4 && lastLDRPinCheck != 5 && lastLDRPinCheck != 6 && lastLDRPinCheck != 7 && lastLDRPinCheck != 8 && lastLDRPinCheck != 9 && lastLDRPinCheck != 10 && lastLDRPinCheck != 11

            && lastLDRPinCheck != 16 && lastLDRPinCheck != 17 && lastLDRPinCheck != 18 && lastLDRPinCheck != 19 && lastLDRPinCheck != 20 && lastLDRPinCheck != 21 && lastLDRPinCheck != 22 && lastLDRPinCheck != 23 && lastLDRPinCheck != 24

            && lastLDRPinCheck != 28 && lastLDRPinCheck != 29 && lastLDRPinCheck != 30 && lastLDRPinCheck != 31 && lastLDRPinCheck != 40)
        {
          // Serial.printf("LDR CHECK %d \r\n", lastLDRPinCheck);
          Serial.printf("LDR CHECK %d -> ", lastLDRPinCheck);
          pinMode(lastLDRPinCheck, INPUT);
          ldrCheckDue = now + ldrDelay;
          int sensorValue = analogRead(lastLDRPinCheck);

          Serial.println(sensorValue);
          lum.updateNeedle(sensorValue, 0);
        }
        else
        {
          // Serial.printf("SKIP LDR CHECK %d \r\n", lastLDRPinCheck);
        }

        // drawLDRValue(sensorValue);
      }
      Serial.printf("################\r\n");
    }
    delay(16);
  }
#endif
}

void command_loop()
{
  touch_loop();
  // delay(50);
}

void node_loop()
{

  // safeSerial.println("GET'S GO!node_loop RETURN");
  // delay(1000);
  // return;

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

    // safeSerial.printf("XA_1: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());
    // delay(10);
    // safeSerial.printf("XA_2: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());

    // auto deadAfterSeconds = lightOnTime * globalState.nodes.GetAllNodes().size() + 1000;
    // safeSerial.printf("deadAfterSeconds: %ld\r\n", deadAfterSeconds);

    globalState.nodes.DeleteDeadNodes(meNode, 10000 / checkInterval); // 10 second?

    // printNodes();

    auto tokenNode = globalState.nodes.GetTokenHolder();

    auto nodeCount = globalState.nodes.GetAllNodes().size();

    // safeSerial.printf("XB_1: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());
    // delay(10);
    // safeSerial.printf("XB_2: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());

    if (tokenNode == meNode)
    {
      auto timeSinceLastWork = millis() - meNode->lastWorkTime;

      // yay, I have the token
      meNode->lostTokenReminder = 0;
      meNode->feetDeep = 0;
      meNode->lastWorkTime = millis();

      // tell everyone I have the token
      sendEspNodeUpdate(meNode, meNode);

      // safeSerial.println("doing stuff: ");
      // do the work (with reminders that I have the token)
      {
        digitalWrite(x_LED_PIN, x_LED_ON);
        safeSerial.printf("LED ON (timeSinceLastWork:%ld)\r\n", timeSinceLastWork);

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

        meNode->lastWorkTime = millis();
        // lastWorkTime = millis();

        digitalWrite(x_LED_PIN, x_LED_OFF);
        safeSerial.println("LED OFF");
        // safeSerial.println("done doing stuff, handing token off");
      }

      // get next

      // safeSerial.printf("A: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());
      tokenNode = globalState.nodes.SetNextAsReady(meNode);
      // safeSerial.printf("B: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());

      // if (tokenNode != meNode)
      // {
      //   safeSerial.printf("it's %s turn now not me %s\r\n", tokenNode->macAddressAsString(), meNode->macAddressAsString());
      // }
      // else
      // {
      //   safeSerial.printf("ME AGAIN? %s turn now not me %s\r\n", tokenNode->macAddressAsString(), meNode->macAddressAsString());
      // }

      // announce that I am done and tell who has the token now
      sendEspNodeUpdate(tokenNode, tokenNode);

      // safeSerial.printf("C1: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());
      // delay(10);
      // safeSerial.printf("C2: %s\r\n", globalState.nodes.GetTokenHolder()->macAddressAsString());
    }
    else
    {
      // someone else has the token, dig them deeper in the ground
      tokenNode->feetDeep++;
      tokenNode->lastWorkTime = millis();
    }

    // announcementToggle = !announcementToggle;
    if (announcementToggle)
    {

      digitalWrite(x_LED_PIN, x_LED_OFF);
      // just send announcement that I am still here, and remind everyone who has the token, they might need to know!
      sendEspNodeUpdate(meNode, tokenNode);
    }

    lastSentTime = millis();
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
}
