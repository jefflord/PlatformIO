#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "utils.h"
#include "M5StickCPlus2.h"
#include "esp32/ulp.h"
#include <chrono>
#include "esp_sleep.h"

#define BTN_A 37
#define BTN_B 39
#define BTN_C 35
#define BTN_C 35

#define BRIGHTNESS 128

auto _mutex = xSemaphoreCreateMutex();
int btnCounter = 0;
int busySecs = 0;
// hw_timer_t *timer = NULL;

// void IRAM_ATTR onTimer()
// {
//   // Perform your tasks here (if any)

//   // Go back to light sleep (adjust duration as needed)
//   esp_sleep_enable_timer_wakeup(1 * 1000000ULL); // 5 seconds
//   esp_light_sleep_start();
// }

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

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  M5.begin();
  auto cfg = M5.config();

  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(1);
  StickCP2.Display.setTextColor(RED);
  StickCP2.Display.setTextDatum(middle_center);
  // StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.setBrightness(BRIGHTNESS);

  WiFi.begin("DarkNet", "7pu77ies77");

  StickCP2.Display.setCursor(0, 0);
  while (!WiFi.isConnected())
  {
    Serial.print(".");
    StickCP2.Display.print(".");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  ArduinoOTA.begin();

  StickCP2.Display.clear();
  StickCP2.Display.setCursor(0, 0);
  StickCP2.Display.print(WiFi.localIP().toString());
  Serial.printf("\n%s\n", WiFi.localIP().toString().c_str());
  vTaskDelay(pdMS_TO_TICKS(5000));

  // StickCP2.Power.deepSleep(0);
  // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); // Disable all wakeup sources.
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  // StickCP2.Power.Axp192.set
  // StickCP2.Power.Axp2101.setlow
  // M5.Power.Axp192;
  // M5.Power.Axp2101;

  // xTaskCreatePinnedToCore(timerCallback, "TaskName0", 10000, NULL, 1, NULL, 0);
  // xTaskCreatePinnedToCore(timerCallback, "TaskName1", 10000, NULL, 1, NULL, 1);

  // TimerHandle_t myTimer = xTimerCreate(
  //     "My Timer",
  //     pdMS_TO_TICKS(500), // Convert milliseconds to ticks
  //     pdTRUE,             // Auto-reload
  //     (void *)0,          // Timer ID (not used in this example)
  //     timerCallback);

  // delay(100);
  // if (myTimer != NULL)
  // {
  //   // Start the timer
  //   xTimerStart(myTimer, 0);
  // }
  // else
  // {
  //   Serial.println("Failed to create timer!");
  // }

  // Set up timer interrupt
  // timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(timer, &onTimer, true);
  // timerAlarmWrite(timer, 30 * 1000000, true); // 1-second interval (adjust as needed)
  // timerAlarmEnable(timer);

  // Ensure the screen stays on

  // StickCP2.Power.Axp192.sc
  // StickCP2.Power.Axp2101.
  // M5.Axp.ScreenBreath(255); // Set the screen brightness to maximum
  // // Prevent the device from sleeping
  // M5.Axp.SetSleepDisable();

  // pinMode(BTN_A, INPUT);
  // pinMode(BTN_B, INPUT);
  // pinMode(BTN_C, INPUT);

  xTaskCreate(
      [](void *pvParameters)
      {
        while (true)
        {
          // Your code here, which runs every 1000ms
          int vol = StickCP2.Power.getBatteryVoltage();
          StickCP2.Display.setTextColor(GREEN);

          if (vol < 3000)
          {
            StickCP2.Display.setTextColor(RED);
          }
          else if (vol < 4000)
          {
            StickCP2.Display.setTextColor(YELLOW);
          }

          if (vol < 2000)
          {
            for (int i = 0; i < 5; i++)
            {
              if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
              {
                StickCP2.Display.setTextColor(PURPLE);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                StickCP2.Display.setTextColor(RED);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                xSemaphoreGive(_mutex);
              }
            }
          }

          Serial.printf("BAT: %dmv\n", vol);

          if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
          {
            StickCP2.Display.clear();
            StickCP2.Display.setCursor(0, 30);
            StickCP2.Display.printf("BAT: %dmv\nuptime: %ld\nBusy: %d", vol, millis() / 1000l, busySecs / 1000);
            StickCP2.Display.setBrightness(BRIGHTNESS);
            xSemaphoreGive(_mutex);
          }

          vTaskDelay(1000 / portTICK_PERIOD_MS);

          // showStartReason();
        }
      },
      "TaskName",
      10000,
      NULL,
      1,
      NULL);
}

void loop()
{

  ArduinoOTA.handle();
  int buttonAState = digitalRead(BTN_A);
  int buttonBState = digitalRead(BTN_B);
  int buttonCState = digitalRead(BTN_C);

  if (buttonAState == LOW)
  {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
    {
      Serial.println("Button A Pressed!");
      StickCP2.Display.clear();
      StickCP2.Display.setCursor(0, 30);
      StickCP2.Display.printf("Press %d\n", ++btnCounter);
      StickCP2.Display.setBrightness(BRIGHTNESS);
      xSemaphoreGive(_mutex);
    }
    busyWait(33);
  }

  if (buttonBState == LOW)
  {
    Serial.println("Button B Pressed!");
  }

  if (buttonCState == LOW)
  {
    Serial.println("Button C Pressed!");
    ESP.restart();
  }

  delay(16);
}
