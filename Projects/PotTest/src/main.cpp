#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_log.h"

// #include <Servo.h>
//  #include <ESP32Servo.h>

// #define POT_PIN 15
#define POT_PIN 0
// #define SERVO_PIN 4
#define SERVO_PIN 2

void list_pwm_pins()
{
  // Initialize LEDC if needed (replace with your actual initialization code)
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_HIGH_SPEED_MODE,  // Or LEDC_LOW_SPEED_MODE if needed
      .duty_resolution = LEDC_TIMER_8_BIT, // Adjust resolution if necessary
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 5000, // Adjust frequency if needed
  };
  ledc_timer_config(&ledc_timer);

  for (int pin = 0; pin < GPIO_NUM_MAX; ++pin)
  {
    ledc_channel_config_t channel_config = {};
    channel_config.gpio_num = pin;
    channel_config.speed_mode = ledc_timer.speed_mode;
    channel_config.channel = LEDC_CHANNEL_0; // Assuming you only need one channel
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.timer_sel = ledc_timer.timer_num;
    channel_config.duty = 0; // Start with 0% duty cycle

    esp_err_t err = ledc_channel_config(&channel_config);
    if (err == ESP_OK)
    {
      ESP_LOGI("PWM", "Pin %d supports PWM (LEDC/RMT)", pin);
    }
    else
    {
      ESP_LOGE("PWM", "Error configuring pin %d for PWM: %s", pin, esp_err_to_name(err));
    }

    // Deinitialize the LEDC channel
    ledc_stop(channel_config.speed_mode, channel_config.channel, 0); // Stop the channel
    // ledc_del_channel(channel_config.channel);                        // Delete the channel
  }
}

// Servo myServo;
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  // myServo.attach(SERVO_PIN); // Attach the servo to the pin

  delay(3000);

  Serial.println("go1!");
  //list_pwm_pins();
  Serial.println("go2!");

  Serial.println("go!");
}

int readPotentiometer(int pin, int numReadings, int delayMs)
{
  long sum = 0;
  for (int i = 0; i < numReadings; i++)
  {
    sum += analogRead(pin);
    delay(delayMs); // Delay between readings
  }
  return sum / numReadings;
}

auto lastValue = 0;
void loop()
{
  Serial.println("Running !!!");
  delay(1000);
  return;

  /*
    if (!myServo.attached())
    {
      Serial.println("no attached");
      myServo.attach(SERVO_PIN, 1000, 2000); // Attach the servo after it has been detatched
    }

    // put your main code here, to run repeatedly:

    auto potValue = readPotentiometer(POT_PIN, 10, 5);

    auto factor = 1;
    potValue = potValue * factor;

    if (potValue != lastValue)
    {
      // auto mappedPotValue = map(potValue, 0, 4095, 0, 100);
      // int angle = map(potValue, 0, 4095, 0, 180);
      auto mappedPotValue = constrain(map(potValue, 28, 1024, 0, 100), 0, 100);
      auto angle = constrain(map(potValue, 28, 1024, 0, 180), 0, 180);

      Serial.print("potValue:");
      Serial.println(potValue);
      Serial.print("mappedPotValue:");
      Serial.println(mappedPotValue);
      Serial.print("anglex:");
      Serial.println(angle);
      Serial.println();

      lastValue = potValue;
      myServo.write(angle); // Set the servo position
    }
    */

  delay(16);
}
