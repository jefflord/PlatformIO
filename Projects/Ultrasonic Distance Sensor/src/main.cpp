#include <Arduino.h>
#include <NewPing.h>

//#define LED_PIN 2 // GPIO2

#define TRIG_PIN 18 //
#define ECHO_PIN 5 //
// #define MAX_DISTANCE 400

// NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

//#define IR_SENSE_PIN 5 // D1-GPIO5

void setup()
{
  Serial.begin(115200);
  // pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // pinMode(IR_SENSE_PIN, INPUT);

  // digitalWrite(LED_PIN, HIGH); // OFF
}

int loopCounter = 0;
int lastDistance = 0;
int lastMontionSense = false;
bool testMotion = false;
bool test_HC_SR04_Distance = true;
bool test_HC_SRF05_Distance = false;

void loop()
{
  // Serial.println("Loop");

  if (test_HC_SRF05_Distance)
  {
    delay(100);
    unsigned int uS = 0;
    // unsigned int uS = sonar.ping();
    int distance = uS / US_ROUNDTRIP_CM;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    return;
  }

  // if (testMotion)
  // {
  //   Serial.print((loopCounter++ % 8));
  //   Serial.print(": ");

  //   int sensorValue = digitalRead(IR_SENSE_PIN);
  //   if (sensorValue == HIGH)
  //   {
  //     // if (lastMontionSense)
  //     {
  //       Serial.println("Movement detected.");
  //       digitalWrite(LED_PIN, LOW); // ON
  //     }

  //     lastMontionSense = true;
  //   }
  //   else
  //   {
  //     // if (!lastMontionSense)
  //     {
  //       Serial.println("No movement detected!");
  //       digitalWrite(LED_PIN, HIGH); // OFF
  //     }
  //     lastMontionSense = false;
  //   }
  //   delay(1000);
  //   return;
  // }

  if (test_HC_SR04_Distance)
  {
    long duration = 0;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);

    float distance = duration * 0.034 / 2; // Calculate distance in centimeters
    int distanceRounded = round(distance);

    // Serial.print("Distance: ");
    // Serial.print(distance);
    // Serial.println(" cm");

    if (lastDistance != distanceRounded)
    {
      lastDistance = distanceRounded;
      Serial.print("Distance: ");
      Serial.print(distanceRounded);
      Serial.println(" cm");
    }

    delay(1000); // Adjust delay as needed
    return;
  }
}
