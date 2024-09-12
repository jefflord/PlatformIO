#include <Arduino.h>

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
