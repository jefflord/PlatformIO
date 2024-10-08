
#include <Arduino.h>
#include <bmp.h>
#include <SPI.h>
#include <ESP32Servo.h>
//#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

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
//#define GFX_BL DF_GFX_BL // Backlight control pin
#define SWITCH_PIN 16
#define TEST_PWM_RESOLUTION false
#define LED_ONBOARD 2

OneWire oneWire(TEMP_SENSOR_1WIRE);
DallasTemperature sensors(&oneWire);
Arduino_DataBus *bus = new Arduino_HWSPI(OLED_DC, OLED_CS, OLED_SCL, OLED_SDA);
Arduino_GFX *gfx = new Arduino_SSD1331(bus, OLED_RES);
Servo myServo; // Create a Servo object
float temperature_0_C = 0;
float temperature_1_C = 0;
float temperature_2_C = 0;

// WiFiUDP ntpUDP;

String formatDeviceAddress(DeviceAddress deviceAddress)
{
  String address = "";
  for (int i = 0; i < 8; i++)
  {
    address += String(deviceAddress[i], HEX);
    if (i < 7)
    {
      address += " ";
    }
  }
  return address;
}

void getTemp(void *parameter)
{

  while (true)
  {
    sensors.requestTemperatures();

    sensors.requestTemperatures();
    for (int i = 0; i < sensors.getDeviceCount(); i++)
    {
      DeviceAddress deviceAddress;
      sensors.getAddress(deviceAddress, i);
      Serial.print("Sensor ");
      Serial.printf("%d, %s", i, formatDeviceAddress(deviceAddress).c_str());
      Serial.print(": ");
      Serial.println(sensors.getTempC(deviceAddress));
    }

    temperature_0_C = sensors.getTempCByIndex(0);
    temperature_1_C = sensors.getTempCByIndex(1);
    temperature_2_C = sensors.getTempCByIndex(2);

    //  Serial.print("temp: ");
    //  Serial.println(temperature_1_C);
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
portMUX_TYPE screenLock = portMUX_INITIALIZER_UNLOCKED;
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

void renderClickIcon(void *p)
{

  pinMode(LED_ONBOARD, OUTPUT);
  while (true)
  {
    while (servoMoving)
    {
      taskENTER_CRITICAL(&screenLock);
      gfx->fillRect(0, 0, 13, 13, BLACK);
      gfx->drawXBitmap(0, 0, epd_bitmap_icons8_natural_user_interface_2_13, 13, 13, WHITE);
      taskEXIT_CRITICAL(&screenLock);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      taskENTER_CRITICAL(&screenLock);
      gfx->fillRect(0, 0, 13, 13, BLACK);
      taskEXIT_CRITICAL(&screenLock);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    vTaskDelay(33 / portTICK_PERIOD_MS);
  }
}

void renderUploadIcon(void *p)
{

  while (true)
  {
    while (uploadingData)
    {

      taskENTER_CRITICAL(&screenLock);
      gfx->fillRect(96 - 13, 0, 13, 13, BLACK);
      gfx->drawXBitmap(96 - 13, 0, epd_bitmap_icons8_thick_arrow_pointing_up_13__2_, 13, 13, WHITE);
      taskEXIT_CRITICAL(&screenLock);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      taskENTER_CRITICAL(&screenLock);
      gfx->fillRect(96 - 13, 0, 13, 13, BLACK);
      taskEXIT_CRITICAL(&screenLock);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    vTaskDelay(33 / portTICK_PERIOD_MS);
  }
  // vTaskDelete(NULL);
}

void displayTest(int delayTimeMs)
{

  // gfx->begin();
  // gfx->fillScreen(BLACK);
  // gfx->setTextColor(WHITE);
  // // gfx->drawXBitmap(96 - 24, 0, epd_bitmap_icons8_upload_to_the_cloud_24_swap, 24, 24, WHITE);
  // gfx->drawXBitmap(96 - 13, 0, epd_bitmap_icons8_thick_arrow_pointing_up_13, 13, 13, WHITE);

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

void fakeUpload(void *p)
{
  vTaskDelay(2500 / portTICK_PERIOD_MS);

  int counter = 0;
  for (;;)
  {

    if ((counter++ & 8) == 0)
    {
      uploadingData = true;
      vTaskDelay(2500 / portTICK_PERIOD_MS);
      uploadingData = false;
    }
    else
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -4 * 60 * 60, 120000);

#define SSDID "DarkNet"
void setup()
{
  Serial.begin(115200);

  while (!Serial)
    continue;

  gfx->begin();
  gfx->fillScreen(BLACK);

  auto result = WiFi.begin(SSDID, "7pu77ies77");
  gfx->print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    gfx->print(".");
  }

  gfx->fillScreen(BLACK);
  Serial.print("\nConnected to Wi-Fi: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.update();

  xTaskCreate(updateTime, "updateTime", 2048, NULL, 1, NULL);

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

  // displayTest(100);
  // return;

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

  xTaskCreate(
      fakeUpload,   // Function to run on the new thread
      "fakeUpload", // Name of the task (for debugging)
      8192,         // Stack size (in bytes)
      NULL,         // Parameter passed to the task
      1,            // Priority (0-24, higher number means higher priority)
      NULL          // Handle to the task (not used here)
  );

  xTaskCreate(renderUploadIcon, "renderUploadIcon", 2048, NULL, 1, NULL);
  xTaskCreate(renderClickIcon, "renderClickIcon", 2048, NULL, 1, NULL);

  Serial.println("Setup Done");
}

unsigned long lastTime = millis(); // Last recorded time
int frameCount = 0;                // Frame counter
int totalFrameCount = 0;           // Frame counter
float fps = 1;                     // FPS value
auto switchState = HIGH;
unsigned long startMicros;
unsigned long elapsedTime;

void pushServoButtonX(void *pvParameters)
{

  servoMoving = true;
  digitalWrite(LED_ONBOARD, HIGH);
  myServo.write(0);
  delay(750);
  myServo.write(90);
  digitalWrite(LED_ONBOARD, LOW);
  servoMoving = false;
  vTaskDelete(NULL);
}

void pushServoButton()
{
  if (!servoMoving)
  {
    // xTaskCreate(showClickAnimation, "showClickAnimation", 2048, NULL, 1, NULL);
    xTaskCreate(pushServoButtonX, "pushServoButton", 2048, NULL, 1, NULL);
  }
}

String formatTime(unsigned long epochTimeMs)
{

  time_t epochSeconds = epochTimeMs / 1000;

  // Create a time_t object from epoch seconds
  time_t epochTime = epochSeconds;

  // Create a tm structure to hold the formatted time
  struct tm formattedTime;

  // Use localtime to populate the tm structure
  localtime_r(&epochTime, &formattedTime);

  // Use strftime to format the time as hh:mm:ss AM
  char formattedTimeString[20];
  strftime(formattedTimeString, sizeof(formattedTimeString), "%I:%M:%S %p", &formattedTime);

  return String(formattedTimeString);
}

String currentTime = "";

void updateTime(void *p)
{
  // return String("99:99 AX");

  for (;;)
  {
    timeClient.update();
    auto lastNTPTime = timeClient.getEpochTime();
    struct tm *timeinfo;
    char timeStringBuff[12];
    timeinfo = localtime((time_t *)&lastNTPTime);
    strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", timeinfo);
    currentTime = String(timeStringBuff);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

String getTime()
{

  return currentTime;
}

String getDecimalPart(double number)
{

  number = round(number * 10) / 10.0;

  char buffer[20]; // Buffer to store the formatted result

  // Format the number to get only the decimal part (keeping two decimal places)
  sprintf(buffer, "%.1f", number); // Adjust precision if needed (e.g., "%.2f")

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

  long loopDelayMs = 1000;
  // long lastRun = 0;

  double lastT1 = 0;
  double lastT2 = 0;
  double lastT3 = 0;

  for (;;)
  {
    auto startTime = millis();
    if (animationShowing)
    {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      forceUpdate = true;
      continue;
    }

    // sprintf(timeString, "%02d:%02d:%02d %s", hours, minutes, seconds, ampm.c_str());
    // sprintf(timeString, "%4.1f\u00B0C", temperature_1_C);

    // temperature_0_C = round(temperature_0_C * 10) / 10.0;
    // temperature_1_C = round(temperature_1_C * 10) / 10.0;
    // temperature_2_C = round(temperature_2_C * 10) / 10.0;

    temperature_0_C = round(temperature_0_C);
    temperature_1_C = round(temperature_1_C);
    temperature_2_C = round(temperature_2_C);

    auto temperatureF1 = (temperature_0_C * (9.0 / 5.0)) + 32;
    auto temperatureF2 = (temperature_1_C * (9.0 / 5.0)) + 32;
    auto temperatureF3 = (temperature_2_C * (9.0 / 5.0)) + 32;

    if (forceUpdate || lastT1 != temperatureF1 || lastT2 != temperatureF2 || lastT3 != temperatureF3)
    {
      forceUpdate = false;
      lastT1 = temperatureF1;
      lastT2 = temperatureF2;
      lastT3 = temperatureF3;

      taskENTER_CRITICAL(&screenLock);

      gfx->setTextColor(WHITE);

      gfx->fillRect(0, SET_CUR_TOP_Y + 16, 96, 64, BLACK);
      // char randomChar = (char)random(97, 127);
      gfx->setCursor(0, SET_CUR_TOP_Y + 16);

      // if (uploadingData)
      // {
      //   renderUploadIcon();
      // }

      gfx->println(getTime());

      gfx->setCursor(gfx->getCursorX(), gfx->getCursorY() + 6);

      char bufferForNumber[20];

      gfx->setTextColor(BLUE);
      sprintf(bufferForNumber, "%2.0f", temperatureF1);
      gfx->print(bufferForNumber);
      // gfx->setTextSize(FONT_SIZE - 1);
      // gfx->print(getDecimalPart(temperatureF1));
      gfx->setTextSize(FONT_SIZE);

      auto y = gfx->getCursorY();
      gfx->setCursor(gfx->getCursorX() + 12, y);
      gfx->setTextColor(RED);
      sprintf(bufferForNumber, "%2.0f", temperatureF2);
      gfx->print(bufferForNumber);
      // gfx->setTextSize(FONT_SIZE - 1);
      // gfx->print(getDecimalPart(temperatureF2));
      gfx->setTextSize(FONT_SIZE);

      gfx->setCursor(gfx->getCursorX() + 12, y);
      gfx->setTextColor(GREEN);
      sprintf(bufferForNumber, "%2.0f", temperatureF3);
      gfx->print(bufferForNumber);
      // gfx->setTextSize(FONT_SIZE - 1);
      // gfx->print(getDecimalPart(temperatureF3));
      gfx->setTextSize(FONT_SIZE);

      taskEXIT_CRITICAL(&screenLock);
    }

    vTaskDelay(loopDelayMs - (millis() - startTime) / portTICK_PERIOD_MS); // Delay for 10ms
  }
}

void showClickAnimation(void *p)
{
  return;
  Serial.println("showClickAnimation");
  int loopCount = 1;
  animationShowing = true;

  taskENTER_CRITICAL(&screenLock);
  for (int i = 0; i < loopCount; i++)
  {

    for (int frame = 0; frame < FRAME_COUNT;)
    {

      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      gfx->drawBitmap((96 / 2) - (30 / 2), 8, frames[frame], FRAME_WIDTH, FRAME_HEIGHT, WHITE, BLACK);
      frame = (frame + 1) % FRAME_COUNT;
      taskEXIT_CRITICAL(&screenLock);
      vTaskDelay((FRAME_DELAY) / portTICK_PERIOD_MS);
      taskENTER_CRITICAL(&screenLock);
      if (frame == 0)
      {
        break;
      }
    }
  }
  gfx->fillScreen(BLACK);
  taskEXIT_CRITICAL(&screenLock);
  animationShowing = false;
  vTaskDelete(NULL);
}

void loop()
{

  Serial.println("loop");
  vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay for 10ms
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
    gfx->print(temperature_1_C);

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
