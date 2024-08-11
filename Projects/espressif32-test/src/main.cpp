#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "my_code.h"
#include <Ticker.h>

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define WIRE Wire

#include <gpio_viewer.h>
#include "Arduino.h"
GPIOViewer gpio_viewer;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);

const char *ssid = "DarkNet";
const char *password = "7pu77ies77";

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -4 * 3600; // Replace with your time zone offset
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

const int ledPin1 = GPIO_NUM_2; // Replace with actual GPIO pin for LED 1
const int myTestPin = GPIO_NUM_14;

#define TOUCH_PIN GPIO_NUM_13

int loopCounter = 0;
Ticker ticker;

void periodicTask() {
    Serial.println("Ticker triggered!");
}
void setup()
{

  Serial.begin(230400);

  while (!Serial)
    continue;

  pinMode(ledPin1, OUTPUT);
  pinMode(myTestPin, OUTPUT);

  pinMode(GPIO_NUM_23, OUTPUT);
  digitalWrite(GPIO_NUM_23, HIGH);

  // gpio_viewer.setPort(5555);

  digitalWrite(ledPin1, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x64

  // display.display();
  // delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  Serial.println("IO test");

  // pinMode(BUTTON_A, INPUT_PULLUP);
  // pinMode(BUTTON_B, INPUT_PULLUP);
  // pinMode(BUTTON_C, INPUT_PULLUP);

  connectToWiFi(display, ledPin1);

  delay(1000);

  timeClient.begin();

  Serial.println(WiFi.localIP());
  // gpio_viewer.connectToWifi(ssid, password); // we don't need this since we're already connected.
  gpio_viewer.begin();

  digitalWrite(ledPin1, LOW);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display(); // actually display all of the above

  delay(1000);

  xTaskCreate(backgroundTask, "Background Task", 2048, &display, 1, NULL);
  ticker.attach(1.0, periodicTask);  // 1.0 = 1 second
}

int row = 0;
int col = 0;

void setBrightness(uint8_t brightness)
{
  if (brightness > 255)
    brightness = 255;                           // Brightness range is 0x00 to 0xFF
  display.ssd1306_command(SSD1306_SETCONTRAST); // 0x81 is the command for contrast
  display.ssd1306_command(brightness);
  // display.ssd1306_command(0x8F);  // Reset to default brightness
}

int bestSignal = -30;
int worstSignal = -50;
void oledLoop()
{

  display.setCursor(0, 0);

  if ((loopCounter++ % 2) == 0)
  {
    Serial.println("myTestPin HIGH");
    digitalWrite(myTestPin, HIGH);
    digitalWrite(ledPin1, HIGH);
  }
  else
  {
    Serial.println("myTestPin LOW");
    digitalWrite(myTestPin, LOW);
    digitalWrite(ledPin1, LOW);
  }

  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  display.clearDisplay();

  int rowMod = (row++ % 20);
  int colMod = (col++ % 64);

  display.setCursor(0, 0);

  int signal = WiFi.RSSI();

  if (signal > bestSignal)
  {
    bestSignal = signal;
  }

  if (signal < worstSignal)
  {
    worstSignal = signal;
  }

  int mappedSignal = map(signal, -90, bestSignal, 0, 100);
  ProgressBarFPS::drawProgressBar(display, 0, 0, mappedSignal, false);

  display.setTextSize(2);
  display.setCursor(20, 20);
  display.print(timeClient.getFormattedTime());

  display.setTextSize(2);
  display.setCursor(0, 40);

  int brightness = (loopCounter % 255);

  // display.print(worstSignal);
  // display.print("/");
  // display.print(bestSignal);
  // display.print(" > ");
  display.print(signal);

  display.print("dBm ");
  display.print(mappedSignal);
  display.print("%");

  display.display();
  // setBrightness(brightness);

  yield();
  delay(1000);
}

ProgressBarFPS progressBarFPS;

void loop()
{

  unsigned long startTime = micros();

  /************ start code *************/

  // progressBarFPS.doProgressBarFPS(display);
  oledLoop();
  /************ end code *************/

  unsigned long endTime = micros();
  unsigned long executionTime = endTime - startTime;

  long remainingTime = 16333 - executionTime; // 16.3ms in microseconds

  if (remainingTime > 0)
  {
    delayMicroseconds(remainingTime);
  }

  return;
}
