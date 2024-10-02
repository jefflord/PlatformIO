#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "utils.h"
#include "M5StickCPlus2.h"
#include "esp32/ulp.h"
#include <chrono>
#include "esp_sleep.h"
#include "driver/i2s.h"
#include <NTPClient.h>

// #include <ESP_WiFiManager.h>
// #include <ESPAsyncUDP.h>
// #include <ESPAsync_WiFiManager_Lite.h>
#define BTN_A 37 // BIG
#define BTN_B 39 // TOP
#define BTN_C 35 // BOOT

#define BUTTON_PIN_BITMASK ((1ULL << 35) | (1ULL << 37) | (1ULL << 39))
#define BUZZER_PIN 2 // GPIO 2
#define SCREEN_TOP 30

#define BRIGHTNESS 64

#define I2S_WS 0  // Word Select (LRCL)
#define I2S_SD 34 // Serial Data (DOUT)
#define I2S_SCK 0 // Bit Clock (BCK)
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 16000
#define I2S_BUFFER_SIZE 512

volatile bool is_recording = false;
auto _mutex = xSemaphoreCreateMutex();
int btnCounter = 0;
int busySecs = 0;

int16_t prevVoltage[10];
int16_t prevVoltagePos = 0;

// hw_timer_t *timer = NULL;

// void IRAM_ATTR onTimer()
// {
//   // Perform your tasks here (if any)

//   // Go back to light sleep (adjust duration as needed)
//   esp_sleep_enable_timer_wakeup(1 * 1000000ULL); // 5 seconds
//   esp_light_sleep_start();
// }

void verifyWiFi();
void wifiBegin();

void i2sInit()
{
  const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  const i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (result != ESP_OK)
  {
    Serial.printf("Error installing I2S driver: %d\n", result);
    return;
  }
  else
  {
    Serial.printf("Success installing I2S driver\n", result);
  }

  result = i2s_set_pin(I2S_PORT, &pin_config);
  if (result != ESP_OK)
  {
    Serial.printf("Error setting I2S pins: %d\n", result);
    return;
  }
  else
  {
    Serial.printf("Success setting I2S pins\n", result);
  }
}

void busyWait(int milliseconds)
{
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + std::chrono::milliseconds(milliseconds);

  while (std::chrono::high_resolution_clock::now() < end)
  {
    // Perform some CPU-intensive computation
    volatile int x = 0;
    for (int i = 0; i < 1000000; ++i)
    {
      x += i;
    }
  }

  busySecs += milliseconds;
}

void timerCallback(TimerHandle_t xTimer)
{
  for (;;)
  {
    Serial.printf("timerCallback on %d\n", xPortGetCoreID());
    busyWait(500);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ESP_WiFiManager wm;
// ESPAsync_WiFiManager_Lite wifiManager;

void wifiBegin()
{
  WiFi.begin();
  int timeout = 20000; // Timeout after 10 seconds
  int elapsed = 0;
  while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
  {
    delay(1000);
    Serial.print("O");
    elapsed += 1000;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nStarting SmartConfig...");
    // Start SmartConfig
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
  }
}

void verifyWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    wifiBegin();
  }
}

void IRAM_ATTR wakeUp()
{
  Serial.println("wakeUp!!!!!!!!!");
}

esp_reset_reason_t resetReason;
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  M5.begin();
  M5.Rtc.begin();

  resetReason = esp_reset_reason();

  if (resetReason == ESP_RST_INT_WDT || resetReason == ESP_RST_TASK_WDT || resetReason == ESP_RST_WDT)
  {
    Serial.printf("WDT resetReason %d\n", resetReason);
    // resetReason = ESP_RST_UNKNOWN;
  }

  auto cfg = M5.config();

  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(1);
  StickCP2.Display.setTextColor(RED);
  // StickCP2.Display.setTextDatum(middle_center);
  //  StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.setBrightness(BRIGHTNESS);
  StickCP2.Display.clear();
  StickCP2.Display.setCursor(0, SCREEN_TOP);
  StickCP2.Display.printf("Bootin'");

  {

    // WiFi.mode(WIFI_AP_STA);
    Serial.println("\nStarting WiFi...");

    wifiBegin();

    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org", -4 * 60 * 60, 60000); // 0 offset from UTC, update every 60 seconds

    timeClient.begin();
    timeClient.update();
    timeClient.update();

    unsigned long epochTime = timeClient.getEpochTime();

    // Convert epoch time to RTC time
    struct tm *timeinfo = gmtime((time_t *)&epochTime);

    m5::rtc_time_t time;
    m5::rtc_date_t date;
    time.hours = timeinfo->tm_hour;
    time.minutes = timeinfo->tm_min;
    time.seconds = timeinfo->tm_sec;
    date.year = timeinfo->tm_year;
    date.month = timeinfo->tm_mon;
    date.date = timeinfo->tm_mday;

    M5.Rtc.setTime(time);
    M5.Rtc.setDate(date);

    Serial.setDebugOutput(true);
    Serial.println("");
    Serial.println("WiFi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  // wifiManager.begin();
  //  bool res = wifiManager.autoConnect("DarkNet ESP Setup"); // anonymous ap

  // if (!res)
  // {
  //   Serial.println("Failed to connect, restarting in 30 seconds.");
  //   delay(30000);
  //   ESP.restart();
  // }

  StickCP2.Display.setCursor(0, 0);
  // while (!WiFi.isConnected())
  // {
  //   Serial.print(".");
  //   StickCP2.Display.print(".");
  //   vTaskDelay(pdMS_TO_TICKS(500));
  // }

  ArduinoOTA.begin();

  StickCP2.Display.clear();
  StickCP2.Display.setCursor(0, SCREEN_TOP);
  StickCP2.Display.printf("Connected:\n%s", WiFi.localIP().toString().c_str());

  // vTaskDelay(pdMS_TO_TICKS(5000));

  // pinMode(BTN_A, INPUT_PULLUP); // Use pull-up resistor
  // attachInterrupt(digitalPinToInterrupt(BTN_A), wakeUp, FALLING);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED)
  {
    Serial.println("A valid wakeup source is enabled");
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
  }
  else
  {
    Serial.println("A valid wakeup source is NOT enabled");
  }

  // #define BTN_A 37 // BIG
  // #define BTN_B 39 // TOP
  // #define BTN_C 35 // BOOT

  esp_sleep_enable_ext1_wakeup(GPIO_SEL_37, ESP_EXT1_WAKEUP_ALL_LOW);
  //  esp_sleep_enable_ext1_wakeup(GPIO_SEL_35 | GPIO_SEL_37 | GPIO_SEL_39, ESP_EXT1_WAKEUP_ANY_HIGH);
  //   pinMode(GPIO_NUM_4, OUTPUT);
  //   digitalWrite(GPIO_NUM_4, HIGH);

  if (true)
  {
    i2sInit();
  }

  xTaskCreate(
      [](void *pvParameters)
      {
        int updateCount = 0;
        while (true)
        {

          if (!is_recording)
          {
            m5::rtc_time_t time;
            auto gotTime = M5.Rtc.getTime(&time);
            if (gotTime)
            {
              // Serial.printf("Current Time: %02d:%02d:%02d\n", time.hours, time.minutes, time.seconds);
            }
            else
            {
              Serial.printf("Failed to get time\n");
            }

            // auto TimeStruct = M5.Rtc.getTime();
            //  M5.Rtc.getTime(&TimeStruct);

            // Print the time and date

            // Your code here, which runs every 1000ms
            auto volts = StickCP2.Power.getBatteryVoltage();

            // prevCurrent = current;
            prevVoltage[(prevVoltagePos % 10)] = volts;
            prevVoltagePos++;

            int totalV = 0;
            for (int x = 0; x < 10; x++)
            {
              totalV += prevVoltage[x];
            }

            auto avgV = totalV / 10.0;

            if (millis() < 10000)
            {
              avgV = totalV / ((millis() / 1000)) - 1;
            }

            const int lowV = 3100;
            const int highV = 4210;

            auto percent = (int)(100.0 * ((avgV - lowV) / (highV - lowV)));

            // Serial.printf("avgV %f, totalV %d, volts %d, percent %d\n", avgV, totalV, volts, percent);

            auto isCharging = false;
            if (volts > avgV)
            {
              isCharging = true;
            }

            percent = (int)std::min(100, percent);
            percent = (int)std::max(0, percent);

            StickCP2.Display.setTextColor(GREEN);

            if (percent < 25)
            {
              StickCP2.Display.setTextColor(RED);
            }
            else if (percent < 50)
            {
              StickCP2.Display.setTextColor(YELLOW);
            }

            // Serial.printf("BAT: %dmv\n", vol);

            if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
            {
              StickCP2.Display.clear();
              StickCP2.Display.setCursor(0, SCREEN_TOP);

              if ((updateCount % 2) == 0)
              {
                StickCP2.Display.printf("BAT: %d%%\nuptime: %ld\nBusy: %d", percent, millis() / 1000l, busySecs / 1000);
              }
              else
              {
                StickCP2.Display.printf("BAT: %.1fv\nuptime: %ld\nBusy: %d", (avgV / 1000.0), millis() / 1000l, busySecs / 1000);
              }

              StickCP2.Display.setBrightness(BRIGHTNESS);
              xSemaphoreGive(_mutex);
            }
          }
          // busyWait(500);

          vTaskDelay(1000 / portTICK_PERIOD_MS);
          updateCount++;

          // showStartReason();
        }
      },
      "TaskName",
      10000,
      NULL,
      1,
      NULL);
}

volatile int recording_part_number = 0;
void stop_recording_callback(void *arg)
{
  is_recording = false;
  ESP_LOGI(TAG, "Recording stopped by timer");
}

#define BUFFER_SIZE 1024
#define SEND_INTERVAL 1000 // Send data every 5 seconds

int16_t buffer[BUFFER_SIZE];
uint8_t *sendBuffer = nullptr;
size_t sendBufferSize = 0;
unsigned long lastSendTime = 0;
auto recordingNumber = 0;

void sendAudioData(bool sendEmpty)
{

  if (sendBufferSize == 0)
  {
    if (!sendEmpty)
    {
      return;
    }
    else
    {
      Serial.println("sending empty");
    }
  }

  HTTPClient http;
  http.begin("http://10.0.0.64:5000/save-file"); // Replace with your server URL
  http.addHeader("Content-Type", "application/octet-stream");

  char buffer[20];
  sprintf(buffer, "%d", recordingNumber);
  http.addHeader("Recording-Number", buffer);

  char buffer2[20];
  sprintf(buffer2, "%d", recording_part_number++);
  http.addHeader("Recording-Part-Number", buffer2);

  Serial.printf("Sending %d bytes\n", sendBufferSize);
  int httpResponseCode = http.POST(sendBuffer, sendBufferSize);

  if (httpResponseCode > 0)
  {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
  }
  else
  {
    Serial.printf("Error code: %d\n", httpResponseCode);
  }

  http.end();

  // Clear the buffer after sending
  free(sendBuffer);
  sendBuffer = nullptr;
  sendBufferSize = 0;
}

void recordAndSendAudio(int duration)
{
  esp_timer_handle_t timer;
  esp_timer_create_args_t timer_args = {
      .callback = &stop_recording_callback,
      .name = "record_timer"};
  esp_timer_create(&timer_args, &timer);
  esp_timer_start_once(timer, duration * 1000000); // Convert to microseconds

  recordingNumber++;

  recording_part_number = 0;

  lastSendTime = millis();
  is_recording = true;
  while (is_recording)
  {
    size_t bytesRead;
    i2s_read(I2S_PORT, buffer, BUFFER_SIZE * sizeof(int16_t), &bytesRead, portMAX_DELAY);

    // Accumulate data in sendBuffer
    size_t newSize = sendBufferSize + bytesRead;
    uint8_t *newBuffer = (uint8_t *)realloc(sendBuffer, newSize);
    if (newBuffer)
    {
      sendBuffer = newBuffer;
      memcpy(sendBuffer + sendBufferSize, buffer, bytesRead);
      sendBufferSize = newSize;
    }

    // Send data every SEND_INTERVAL milliseconds
    if (millis() - lastSendTime > SEND_INTERVAL)
    {
      sendAudioData(false);
      lastSendTime = millis();
    }
  }

  if (sendBufferSize > 0)
  {
    // send final data
    sendAudioData(false);
  }

  // send empty data;
  sendAudioData(true);

  esp_timer_delete(timer);
}

long lastClick = 0L;

void loop()
{

  M5.update();
  ArduinoOTA.handle();
  int buttonAState = digitalRead(BTN_A);
  int buttonBState = digitalRead(BTN_B);
  int buttonCState = digitalRead(BTN_C);

  if (M5.BtnA.wasPressed())
  {
    if (lastClick >= millis() - 500)
    {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, SCREEN_TOP);
      Serial.printf("Recording with free heap: %d\n", ESP.getFreeHeap());
      M5.Lcd.println("Recording...");

      tone(BUZZER_PIN, 2000, 300);

      is_recording = true;
      // recordAndSendAudio(5);
      delay(5000);
      is_recording = false;

      tone(BUZZER_PIN, 2000, 800);

      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.println("Recording finished");
      Serial.printf("Recording finished\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
      // M5.Power.lightSleep(60 * 1000 * 1000, true);
      return;
    }
    else
    {
      lastClick = millis();
    }
  }

  if (buttonAState == LOW)
  {
    Serial.println("Button A Pressed!");
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
    {
      StickCP2.Display.clear();
      StickCP2.Display.setCursor(0, SCREEN_TOP);
      StickCP2.Display.printf("Press %d\n", ++btnCounter);
      StickCP2.Display.setBrightness(BRIGHTNESS);
      xSemaphoreGive(_mutex);
    }
    // busyWait(33);
  }

  if (buttonBState == LOW)
  {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
    {
      esp_sleep_wakeup_cause_t wakeup_reason;

      int sleepCount = 0;
      Serial.printf("Button B Pressed, sleeping...\n");
      M5.Lcd.fillScreen(BLACK);
      StickCP2.Display.setCursor(0, SCREEN_TOP);
      M5.Lcd.println("Sleeping");
      vTaskDelay(pdMS_TO_TICKS(500));
      M5.Lcd.fillScreen(BLACK);
      StickCP2.Display.setBrightness(0);
      vTaskDelay(pdMS_TO_TICKS(500));
      xSemaphoreGive(_mutex);

      do
      {
        if ((sleepCount++ % 5) == 0)
        {
          if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
          {
            StickCP2.Display.setBrightness(BRIGHTNESS);
            M5.Lcd.fillScreen(BLACK);
            StickCP2.Display.setCursor(0, SCREEN_TOP);
            M5.Lcd.println("Zzzz...");
            tone(BUZZER_PIN, 2000, 300);
            vTaskDelay(pdMS_TO_TICKS(2000));
            StickCP2.Display.setBrightness(0);
            vTaskDelay(pdMS_TO_TICKS(500));
            xSemaphoreGive(_mutex);
          }
        }

        // if (WiFi.status() != WL_CONNECTED)
        // {
        //   Serial.println("not connected");
        // }
        // else
        // {
        //   Serial.println("connected");
        // }
        // delay(100);

        // M5.update();
        // verifyWiFi();
        // Serial.println("a");
        // // ArduinoOTA.begin();
        // Serial.println("b");
        // delay(1000);
        // Serial.println("c");
        // ArduinoOTA.handle();
        // Serial.println("d");
        // delay(1000);

        M5.Power.lightSleep(10 * 1000 * 1000, true);
        wakeup_reason = esp_sleep_get_wakeup_cause();
        Serial.printf("woke up, resetReason %d\n", resetReason);
        delay(16);

        // if (WiFi.status() != WL_CONNECTED)
        // {
        //   Serial.println("2 not connected");
        // }
        // else
        // {
        //   Serial.println("2 connected");
        // }
        // delay(100);

      } while (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER);

      WiFi.setSleep(WIFI_PS_NONE);
      WiFi.mode(WIFI_AP_STA);
      verifyWiFi();

      // M5.Power.deepSleep(10 * 1000 * 1000, true);
      // Serial.printf("deepSleep return\n");

      StickCP2.Display.setBrightness(BRIGHTNESS);
    }
    // M5.Power.lightSleep(0, true);
  }

  if (buttonCState == LOW)
  {
    Serial.println("Button C Pressed, deepSleep!!!");
    M5.Power.deepSleep(10 * 1000 * 1000, true);
    // Serial.printf("deepSleep return\n");
    // wifiManager.resetSettings();
    // delay(100);
    // ESP.restart();
  }

  delay(16);

  verifyWiFi();
}
