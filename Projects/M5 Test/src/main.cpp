#include <Arduino.h>
#include "M5StickCPlus2.h"
#include "esp32/ulp.h"
#include <chrono>

#define BTN_A 37
#define BTN_B 39
#define BTN_C 35
#define BTN_C 35

void showStartReason()
{
  esp_reset_reason_t reason = esp_reset_reason();

  Serial.print("Reset reason: ");
  long delayMs = 0;

  switch (reason)
  {
  case ESP_RST_POWERON:
    Serial.println("Power on reset");
    break;
  case ESP_RST_EXT:
    Serial.println("External reset");
    break;
  case ESP_RST_SW:
    Serial.println("Software reset");
    break;
  case ESP_RST_PANIC:
    Serial.println("Exception/panic reset");
    delayMs = 1000;
    break;
  case ESP_RST_INT_WDT:
    Serial.println("Interrupt watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("Task watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_WDT:
    Serial.println("Other watchdog reset");
    delayMs = 1000;
    break;
  case ESP_RST_DEEPSLEEP:
    Serial.println("Deep sleep reset");
    break;
  case ESP_RST_BROWNOUT:
    Serial.println("Brownout reset");
    break;
  case ESP_RST_SDIO:
    Serial.println("SDIO reset");
    break;
  default:
    Serial.println("Unknown reset reason");
    break;
  }

  // delay(delayMs);
}

hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer()
{
  // Perform your tasks here (if any)

  // Go back to light sleep (adjust duration as needed)
  esp_sleep_enable_timer_wakeup(1 * 1000000ULL); // 5 seconds
  esp_light_sleep_start();
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

  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(1);
  StickCP2.Display.setTextColor(RED);
  StickCP2.Display.setTextDatum(middle_center);
  // StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.setBrightness(50);

  M5.begin();

  xTaskCreatePinnedToCore(timerCallback, "TaskName0", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(timerCallback, "TaskName1", 10000, NULL, 1, NULL, 1);

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
              StickCP2.Display.setTextColor(PURPLE);
              vTaskDelay(500 / portTICK_PERIOD_MS);
              StickCP2.Display.setTextColor(RED);
              vTaskDelay(500 / portTICK_PERIOD_MS);
            }
          }

          Serial.printf("BAT: %dmv\n", vol);
          StickCP2.Display.clear();
          StickCP2.Display.setCursor(0, 30);
          StickCP2.Display.printf("BAT: %dmv\nuptime: %ld", vol, millis() / 1000l);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          StickCP2.Display.setBrightness(50);
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

  int buttonAState = digitalRead(BTN_A);
  int buttonBState = digitalRead(BTN_B);
  int buttonCState = digitalRead(BTN_C);

  if (buttonAState == LOW)
  {
    Serial.println("Button A Pressed!");
    // Wake up after 5 seconds
    // esp_sleep_enable_timer_wakeup(5 * 1000000); // 5 seconds * 1,000,000 microseconds/second
    // esp_deep_sleep_start();
  }

  if (buttonBState == LOW)
  {
    Serial.println("Button B Pressed!");
  }

  if (buttonCState == LOW)
  {
    Serial.println("Button C Pressed!");
  }

  busyWait(33);
  delay(16);
}
