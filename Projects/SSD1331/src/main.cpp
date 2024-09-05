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
GPIO27  TOUCH_PIN
GPIO32	SERVO
GPIO33
GPIO34  TEMP_SENSOR
GPIO35
GPIO36
GPIO39
VIN

*/
#include <Arduino.h>
#include <bmp.h>
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
#define TEMP_SENSOR_1WIRE 19
#define TOUCH_PIN 33
#define SET_CUR_TOP_Y 16 - 16
#define FONT_SIZE 2
#define GFX_BL DF_GFX_BL // Backlight control pin
#define SWITCH_PIN 16
#define TEST_PWM_RESOLUTION false

OneWire oneWire(TEMP_SENSOR_1WIRE);
DallasTemperature sensors(&oneWire);
Arduino_DataBus *bus = new Arduino_HWSPI(OLED_DC, OLED_CS, OLED_SCL, OLED_SDA);
Arduino_GFX *gfx = new Arduino_SSD1331(bus, OLED_RES);
Servo myServo; // Create a Servo object
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

bool isTouchDown = false;

void updateAngle();
void pushServoButton();
void updateDisplay(void *p);

// TaskHandle_t myTaskHandle = NULL;
QueueHandle_t onTouchQueue = NULL;

BaseType_t xHigherPriorityTaskWoken = pdFALSE;
// portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
void onTouchOffWatcher(void *pvParameters)
{
  int receivedValue;
  for (;;)
  {
    // Serial.printf("xQueueReceive 1\n!");
    xQueueReceive(onTouchQueue, &receivedValue, portMAX_DELAY); // Wait for a message
    // Serial.printf("xQueueReceive 2 - %d\n!", receivedValue);
    if (isTouchDown)
    {
      vTaskDelay(500 / portTICK_PERIOD_MS); // Delay for 10ms
      isTouchDown = false;
    }
  }
}

void delayedTask(void *parameter)
{
  // Your delayed code here
  vTaskDelay(100 / portTICK_PERIOD_MS);
  isTouchDown = false;
  vTaskDelete(NULL); // Delete the task after it's done
}

void onTouch()
{

  // Serial.printf("Touch detected on %d\n!", TOUCH_PIN);

  // taskENTER_CRITICAL(&myMutex);
  // Serial.printf("Touch detected on %s\n!", isTouchDown ? "true" : "false");
  int sendValue = 2;
  if (xQueueSendFromISR(onTouchQueue, &sendValue, &xHigherPriorityTaskWoken) == pdTRUE)
  {
    isTouchDown = true;
  }
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  // taskEXIT_CRITICAL(&myMutex);
}

void displayTest(int delayTimeMs)
{

  return;
  gfx->begin();
  gfx->setTextSize(FONT_SIZE);
  gfx->fillScreen(BLACK);

  gfx->setCursor(2, SET_CUR_TOP_Y);

  gfx->print("1");

  delay(delayTimeMs);
  gfx->setCursor(2, SET_CUR_TOP_Y);
  gfx->print("12");

  delay(delayTimeMs);
  gfx->setCursor(2, SET_CUR_TOP_Y);
  gfx->print("123");

  delay(delayTimeMs);
  gfx->setCursor(2, SET_CUR_TOP_Y);

  gfx->print("1234");

  delay(delayTimeMs);
  gfx->fillScreen(BLACK);

  gfx->drawRect(0, 0, 96, 64, WHITE);
}

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
  // myServo.write(0);

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
  // gfx->setFont(u8g2_font_t0_11_tr); // not fixed

  if (false)
  {
    onTouchQueue = xQueueCreate(1, sizeof(uint8_t));
    xTaskCreate(onTouchOffWatcher, "onTouch", 2048, NULL, 1, NULL);
    touchAttachInterrupt(TOUCH_PIN, onTouch, 40);
  }
  else
  {
    touchAttachInterrupt(TOUCH_PIN, pushServoButton, 50);
  }

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

  displayTest(100);

  xTaskCreate(
      getTemp,   // Function to run on the new thread
      "getTemp", // Name of the task (for debugging)
      8192,      // Stack size (in bytes)
      NULL,      // Parameter passed to the task
      1,         // Priority (0-24, higher number means higher priority)
      NULL       // Handle to the task (not used here)
  );

  xTaskCreate(
      updateDisplay,   // Function to run on the new thread
      "updateDisplay", // Name of the task (for debugging)
      8192,            // Stack size (in bytes)
      NULL,            // Parameter passed to the task
      1,               // Priority (0-24, higher number means higher priority)
      NULL             // Handle to the task (not used here)
  );

  Serial.println("Setup Done");
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
int touchValue = 0;
bool servoMoving = false;

void pushServoButtonX(void *pvParameters)
{
  servoMoving = true;

  myServo.write(0);
  delay(750);
  myServo.write(90);
  delay(250);
  servoMoving = false;
  vTaskDelete(NULL);
}

void pushServoButton()
{
  if (!servoMoving)
  {
    xTaskCreate(pushServoButtonX, "pushServoButton", 2048, NULL, 1, NULL);
  }
}

String formatTime(unsigned long epochTime)
{
  // Extract hours, minutes, seconds
  int hours = (epochTime % 86400L) / 3600; // 24-hour format
  int minutes = (epochTime % 3600) / 60;
  int seconds = epochTime % 60;

  // Determine if it's AM or PM
  String ampm = "AM";
  if (hours >= 12)
  {
    ampm = "PM";
    if (hours > 12)
      hours -= 12; // Convert to 12-hour format
  }
  else if (hours == 0)
  {
    hours = 12; // Midnight case
  }

  // Format the result as hh:mm:ss AM/PM
  char timeString[12]; // Buffer for formatted time
  // sprintf(timeString, "%02d:%02d:%02d %s", hours, minutes, seconds, ampm.c_str());
  sprintf(timeString, "%02d:%02d:%02d", hours, minutes, seconds, ampm.c_str());

  return String(timeString);
}

String getTime()
{
  auto currentTime = (1725491463531 + millis()) / 1000;
  return formatTime(currentTime);

  // timeClient->update();
  // lastNTPTime = ((int64_t)timeClient->getEpochTime() * 1000);
  // return ((int64_t)timeClient->getEpochTime() * 1000);
}

String getDecimalPart(double number)
{
  char buffer[20]; // Buffer to store the formatted result

  // Format the number to get only the decimal part (keeping two decimal places)
  sprintf(buffer, "%.2f", number); // Adjust precision if needed (e.g., "%.2f")

  // Find the position of the decimal point
  char *decimalPoint = buffer;
  while (*decimalPoint != '.' && *decimalPoint != '\0')
  {
    decimalPoint++;
  }

  // Return the part after the decimal point
  if (*decimalPoint == '.')
  {
    return String(decimalPoint + 1); // Skip the decimal point itself
  }
  else
  {
    return String("0"); // Return "0" if there is no decimal part
  }
}

int getSignal()
{

  return 78;
}

void updateDisplay(void *p)
{
  gfx->begin();
  gfx->setTextSize(FONT_SIZE);
  gfx->fillScreen(BLACK);

  auto loopDelayMs = 100;
  auto lastRun = 0;

  for (;;)
  {

    auto startTime = millis();
    while (servoMoving)
    {
      showClickAnimation(1);
    }

    // Serial.println("updateDisplay!");
    // Serial.println(lastRun);
    // Serial.println(millis());
    // Serial.println(millis() - lastRun);

    if (millis() - lastRun <= 2000)
    {
      // Serial.println("Not updateDisplay!");
      vTaskDelay(loopDelayMs - (millis() - startTime) / portTICK_PERIOD_MS); // Delay for 10ms
      continue;
    }
    else
    {
      Serial.println("updateDisplay!");
    }

    lastRun = millis();

    gfx->setTextColor(WHITE);

    gfx->fillRect(0, 0, 96, 64, BLACK);
    // char randomChar = (char)random(97, 127);
    gfx->setCursor(0, SET_CUR_TOP_Y + 16);

    gfx->println(getTime());

    gfx->setCursor(gfx->getCursorX(), gfx->getCursorY() + 6);

    char timeString[20]; // Buffer for formatted time
    // sprintf(timeString, "%02d:%02d:%02d %s", hours, minutes, seconds, ampm.c_str());
    // sprintf(timeString, "%4.1f\u00B0C", temperatureC);

    auto temperatureF = (temperatureC * (9.0 / 5.0)) + 32;
    // sprintf(timeString, "%4.1f/", temperatureF);
    gfx->setTextColor(BLUE);
    sprintf(timeString, "%2.0f", temperatureF);
    gfx->print(timeString);
    gfx->setTextSize(FONT_SIZE - 1);
    gfx->print(getDecimalPart(temperatureF));
    gfx->setTextSize(FONT_SIZE);

    temperatureF += 5.6;

    gfx->setCursor(gfx->getCursorX() + 22, gfx->getCursorY());

    gfx->setTextColor(GREEN);
    sprintf(timeString, "%2.0f", temperatureF);
    gfx->print(timeString);
    gfx->setTextSize(FONT_SIZE - 1);
    gfx->print(getDecimalPart(temperatureF));
    gfx->setTextSize(FONT_SIZE);

    // sprintf(timeString, "%4.1f", temperatureF + 5.6);
    // gfx->print(timeString);
    //  sprintf(timeString, "%4.1fC", temperatureC);

    vTaskDelay(loopDelayMs - (millis() - startTime) / portTICK_PERIOD_MS); // Delay for 10ms
  }
}

void showClickAnimation(int loopCount)
{

  // Serial.print("FRAME_COUNT:");
  // Serial.println(FRAME_COUNT);

  for (int i = 0; i < loopCount; i++)
  {

    // Serial.print("loopCount:");
    // Serial.print(i);
    // Serial.print("/");
    // Serial.println(loopCount);

    for (int frame = 0; frame < FRAME_COUNT;)
    {
      // Serial.print("frame:");
      // Serial.print(frame);
      // Serial.print("/");
      // Serial.println(FRAME_COUNT);

      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      // gfx->bl
      gfx->drawBitmap((96 / 2) - (30 / 2), 8, frames[frame], FRAME_WIDTH, FRAME_HEIGHT, WHITE, BLACK);
      // display.display();
      frame = (frame + 1) % FRAME_COUNT;
      delay(FRAME_DELAY / 2);
      if (frame == 0)
      {
        break;
      }
    }
  }
}

void loop()
{
  // showClickAnimation(2);
  return;

  switchState = digitalRead(SWITCH_PIN);

  startMicros = micros();

  touchValue = touchRead(TOUCH_PIN);
  Serial.print("touchValue");
  Serial.println(touchValue);

  // isTouchDown || touchValue < 40
  if (switchState == LOW)
  {

    // Serial.println(angle);
    // updateAngle();
    // myServo.write(angle);

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
