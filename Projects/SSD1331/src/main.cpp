/*
3V3
EN
GND
GND
GPIO01
GPIO02
GPIO03
GPIO04
GPIO05	OLED_CS
GPIO12
GPIO13
GPIO14
GPIO15
GPIO16  SWITCH_PIN
GPIO17
GPIO18	OLED_SCL
GPIO19
GPIO21	OLED_DC
GPIO22	OLED_RES
GPIO23	OLED_SDA
GPIO25
GPIO26
GPIO27
GPIO32	SERVO
GPIO33
GPIO34
GPIO35
GPIO36
GPIO39
VIN



3V3		OLED_VCC
GND		OLED_GND
GPIO05	OLED_CS
GPIO18	OLED_SCL
GPIO21	OLED_DC
GPIO22	OLED_RES
GPIO23	OLED_SDA

GPIO16  SWITCH_PIN

GPIO32	SERVO
*/
#include <Arduino.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PWM_PIN 32
#define PWM_CHANNEL 0
// #define PWM_FREQUENCY 500
// #define PWM_FREQUENCY 312500 // Frequency in Hz (500 Hz)
#define PWM_FREQUENCY 2441

// #define PWM_RESOLUTION 8
#define PWM_RESOLUTION 15

#define GFX_BL DF_GFX_BL // Backlight control pin
#define OLED_CS 5
#define OLED_DC 21
#define OLED_RES 22
#define OLED_SDA 23
#define OLED_SCL 18

#define SERVO_PIN 32

#define ONE_WIRE_BUS 33
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define SET_CUR_TOP_Y 8 * 2
#define FONT_SIZE 2

#define GFX_BL DF_GFX_BL // Backlight control pin

Arduino_DataBus *bus = new Arduino_HWSPI(OLED_DC, OLED_CS, OLED_SCL, OLED_SDA);
Arduino_GFX *gfx = new Arduino_SSD1331(bus, OLED_RES);

#define SWITCH_PIN 16

#define TEST_PWM_RESOLUTION false

Servo myServo; // Create a Servo object

void setup()
{
  Serial.begin(115200);

  while (!Serial)
    continue;

  Serial.print("File: ");
  Serial.print(PROJECT_SRC_DIR);
  Serial.print(" (");
  Serial.print(__FILE__);
  Serial.println(")");
  Serial.print("Compiled on: ");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.println(__TIME__);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  sensors.begin(); // Start the DS18B20 sensor

  if (TEST_PWM_RESOLUTION)
  {
    auto dutyPercent = 0.666;
    // int duty = 255 * dutyPercent;
    int duty = 32767 * dutyPercent;

    Serial.println();
    Serial.print("duty:");
    Serial.print(duty);
    Serial.print("/");
    Serial.println(dutyPercent);

    Serial.print("PWM_FREQUENCY:");
    Serial.println(PWM_FREQUENCY);

    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, duty); // Set duty cycle to 50% (128/255)
    return;
  }

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  gfx->begin();
  gfx->fillScreen(BLACK);
  // gfx->setUTF8Print(true);

  // gfx->setFont(u8g2_font_helvR08_te); // not fixed
  // gfx->setFont(u8g2_font_bitcasual_tf); // not fixed
  // gfx->setFont(u8g2_font_luBIS08_tf);   // not fixed-italics
  gfx->setFont(u8g2_font_t0_11_tr); // not fixed

  auto i = 0;
  while (!true)
  {
    // gfx->setFont(u8g2_font_10x20_tf);
    gfx->setFont(u8g2_font_helvR08_te);

    gfx->setTextSize(FONT_SIZE);
    gfx->fillScreen(BLACK);
    gfx->setCursor(0, SET_CUR_TOP_Y);
    gfx->print(i);
    gfx->println(" - A1 Hello 12345");
    Serial.print(i);
    Serial.println(" -A1");
    i++;
    delay(1000);

    // gfx->setTextSize(2);
    // gfx->fillScreen(BLACK);
    // gfx->setCursor(0, 0);
    // gfx->println("A2 Hello 12345");
    // Serial.println("A2");
    // delay(2000);

    // gfx->setTextSize(3);
    // gfx->fillScreen(BLACK);
    // gfx->setCursor(0, 0);
    // gfx->println("A3 Hello 12345");
    // Serial.println("A3");
    // delay(2000);

    // gfx->setTextSize(1);
    // gfx->setFont();
    // gfx->fillScreen(BLACK);
    // gfx->setCursor(0, 0);
    // gfx->println("B Hello");
    // Serial.println("B");
    // delay(2000);
  }

  gfx->begin();
  gfx->setTextSize(FONT_SIZE);
  gfx->fillScreen(BLACK);

  gfx->setCursor(2, SET_CUR_TOP_Y);
  gfx->print("1");
  // gfx->drawRect(0, 0, 10, 20, WHITE);

  delay(1000);
  gfx->setCursor(2, SET_CUR_TOP_Y);
  // gfx->drawRect(0, 0, 20, 20, WHITE);
  gfx->print("12");

  delay(1000);
  gfx->setCursor(2, SET_CUR_TOP_Y);
  // gfx->drawRect(0, 0, 30, 20, WHITE);
  gfx->print("123");

  delay(1000);
  gfx->setCursor(2, SET_CUR_TOP_Y);

  gfx->print("1234");

  delay(1000);
  gfx->fillScreen(BLACK);

  gfx->drawRect(0, 0, 96, 64, WHITE);

  Serial.println("done");

  xTaskCreate(
      getTemp,   // Function to run on the new thread
      "getTemp", // Name of the task (for debugging)
      8192,      // Stack size (in bytes)
      NULL,      // Parameter passed to the task
      1,         // Priority (0-24, higher number means higher priority)
      NULL       // Handle to the task (not used here)
  );
}

unsigned long lastTime = millis(); // Last recorded time
int frameCount = 0;                // Frame counter
int totalFrameCount = 0;           // Frame counter
float fps = 1;                     // FPS value
auto switchState = HIGH;
unsigned long startMicros;
unsigned long elapsedTime;

int angle = 0;
bool dirUp = true;
float temperatureC = 0;

void getTemp(void *parameter)
{
  while (true)
  {
    sensors.requestTemperatures();
    temperatureC = sensors.getTempCByIndex(0);
    Serial.print("temp: ");
    Serial.println(temperatureC);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay for 10ms
  }
}

void loop()
{

  // return;

  switchState = digitalRead(SWITCH_PIN);

  startMicros = micros();

  if (switchState == LOW)
  {

    Serial.println(angle);
    updateAngle();
    myServo.write(angle);

    frameCount++;
    totalFrameCount++;
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= 1000)
    {
      fps = frameCount / ((currentTime - lastTime) / 1000.0);

      frameCount = 0;
      lastTime = currentTime;
    }

    // Serial.println("Switch is closed");
    char randomChar = (char)random(97, 127);
    // gfx->fillScreen(BLACK);
    gfx->fillRect(1, 1, 94, 62, BLACK);
    // gfx->drawRect(0, 0, 94, 94, WHITE);
    gfx->setCursor(2, SET_CUR_TOP_Y);
    // gfx->print(i);
    // gfx->print("-");
    gfx->printf("%.1f", fps);
    gfx->print("-");
    gfx->println(angle);
    gfx->print(temperatureC);
    
    elapsedTime = micros() - startMicros;
    // 8333
    delayMicroseconds(16666 * 1 - elapsedTime);
    // delay(1000);
  }
  else
  {
    delay(33);
  }
}

void updateAngle()
{
  if (dirUp && angle >= 180)
  {
    dirUp = false;
  }
  else if (!dirUp && angle <= 0)
  {
    dirUp = true;
  }
  if (dirUp)
  {
    angle++;
  }
  else
  {
    angle--;
  }
}
