#include <Arduino.h>

// #define configGENERATE_RUN_TIME_STATS 1
// #define portGET_RUN_TIME_COUNTER_VALUE() (esp_task_get_run_time_stats())

// #include <NeoPixelBus.h>
const uint16_t PixelCount = 1; // Number of LEDs
const uint8_t PixelPin = 48;   // GPIO pin connected to the LED strip

#define TOUCH_PIN 12

int counter = 0;
// NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod> strip(PixelCount, PixelPin);

unsigned long lastMillis;

void setup()
{
  Serial.begin(115200);

  while (!Serial)
    continue;

  // strip.Begin();
  // strip.Show();
  Serial.println("Go");

  pinMode(LED_BUILTIN, OUTPUT);

  lastMillis = millis();
}

void createCpuLoad(int durationMs)
{
  unsigned long startTime = millis();

  while (millis() - startTime < durationMs)
  {
    volatile unsigned long counter = 0;

    // Perform a computation-intensive task
    for (unsigned long i = 0; i < 1000000; i++)
    {
      counter += i;
    }
  }

  // Serial.println("CPU load completed.");
}

void doWork(void *p)
{
  TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
  const char *taskName = pcTaskGetName(currentTaskHandle);

  int lastCoreId = xPortGetCoreID();
  int currentCoreId;

  for (;;)
  {

    currentCoreId = xPortGetCoreID();
    if (currentCoreId != lastCoreId)
    {
      Serial.printf("CORE CHANGED from %d to %d\n", lastCoreId, currentCoreId);
      lastCoreId = currentCoreId;
      Serial.print("Task: ");
      Serial.print(taskName);
      Serial.print(" ");
      Serial.print((uint32_t)currentTaskHandle, HEX); // Print handle in hexadecimal format
      Serial.print(" - Core:");
      Serial.println(currentCoreId);
    }

    // // Get CPU usage for the loop task
    // UBaseType_t uxTaskNumber = uxTaskGetTaskNumber(currentTaskHandle);
    // TaskStatus_t taskStatus;
    // vTaskGetInfo(currentTaskHandle, &taskStatus, pdTRUE, eInvalid);
    // uint32_t ulRunTimeCounter = taskStatus.ulRunTimeCounter;
    // uint32_t totalTime = esp_timer_get_time() / 1000; // Get total time in milliseconds
    // float cpuUsage = (ulRunTimeCounter / (float)totalTime) * 100.0;

    // Serial.print("Task ");
    // Serial.print(uxTaskNumber);
    // Serial.print(" CPU Usage: ");
    // Serial.print(cpuUsage);
    // Serial.println("%");

    createCpuLoad(50);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
int workCounter = 0;
void loop()
{

  auto currentMillis = millis();
  auto elapsedMillis = currentMillis - lastMillis;

  int touchValue = touchRead(12);
  // Serial.print("touchValue:");
  // Serial.println(val);

  // Serial.println("LED_BUILTIN");
  // Serial.println(LED_BUILTIN);

  if (touchValue > 35000)
  { // Example threshold for detecting touch

    digitalWrite(LED_BUILTIN, HIGH);
    if (elapsedMillis < 1000)
    {
      // Serial.println("Too soon!");
    }
    else
    {

      char buffer[20]; // Buffer to store the formatted result
      workCounter++;
      sprintf(buffer, "doWork%d", workCounter);

      Serial.print("Touched -> ");
      Serial.println(buffer);

      xTaskCreate(doWork, buffer, 2048, NULL, 1, NULL);

      lastMillis = millis();
    }
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    // Serial.println("Not Touched");
  }

  delay(100);
}

// void loopLights()
// {
//   counter++;

//   Serial.println("loop: ");

//   auto brightness = 0.05;

//   delay(100);
//   strip.SetPixelColor(0, RgbColor(255 * brightness, 0, 0)); // Red
//   strip.Show();
//   delay(300);

//   strip.SetPixelColor(0, RgbColor(128 * brightness, 0, 0)); // Red
//   strip.Show();
//   delay(300);

//   strip.SetPixelColor(0, RgbColor(64 * brightness, 0, 0)); // Red
//   strip.Show();
//   delay(300);

//   strip.SetPixelColor(0, RgbColor(0, 0, 0)); // Red
//   strip.Show();
// }
