#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_PIN 97 // Define the pin where the LED is connected

int current = 0;
int max = 20;

void createCpuLoad(int durationMs)
{
    // Convert duration from milliseconds to microseconds
    uint64_t startTime = esp_timer_get_time();
    uint64_t endTime = startTime + (durationMs * 1000);

    while (esp_timer_get_time() < endTime)
    {
        volatile uint64_t counter = 0;

        // Perform a computation-intensive task
        for (uint64_t i = 0; i < 1000000; i++)
        {
            counter += i;
        }
    }

    // Print completion message (replace with ESP-IDF logging if needed)
    // printf("CPU load completed.\n");
}

void doWork(void *p)
{
    TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
    const char *taskName = pcTaskGetName(currentTaskHandle);

    int *num = (int *)p;

    int lastCoreId = -1;
    int currentCoreId;

    createCpuLoad(1); // Create CPU load for 50 milliseconds
    for (;;)
    {
        currentCoreId = xPortGetCoreID();
        if (currentCoreId != lastCoreId)
        {
            printf("%s (%d of %d) from %d to %d\n", taskName, *num, max, lastCoreId, currentCoreId);
            lastCoreId = currentCoreId;
            // printf("Task: %s Core:%d\n", taskName, currentCoreId);
        }

        // createCpuLoad(1);                     // Create CPU load for 50 milliseconds
        vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 1 second
    }
}

void app_main(void)
{

    printf("Starting LED blink program...\n");

    max = 50;
    current = 0;

    while (1)
    {
        char buffer[20]; // Buffer to store the formatted result
        sprintf(buffer, "doWork%d", current);

        if (current < max)
        {
            current++;
            xTaskCreate(doWork, buffer, 2048, &current, 1, NULL);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 1 second
    }
}