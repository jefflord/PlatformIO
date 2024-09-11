/*
Notes:
  - Temp senor needs 5v and 4.6kΩR (pull-up I think)
  - Activation button has a extra 10kΩR (pull-up)

To Do:
  - Log start time, boot reason
  - Reboot from web
*/
#include <Arduino.h>
#include <ESP32Servo.h>
#include "helper.h"

#define USE_MUTEX true
#define USE_LOCK false

/***************************************************/

class SharedClass
{
private:
  int sharedVariable;
  SemaphoreHandle_t mutex;
  portMUX_TYPE sharedObjectMutex;

public:
  double objectCore = 0;

  SharedClass()
  {

    objectCore = round(xPortGetCoreID());
    Serial.printf("SharedClass in  %d\n", objectCore);

    if (USE_LOCK)
    {

      sharedObjectMutex = portMUX_INITIALIZER_UNLOCKED;

      Serial.println("TEST portENTER_CRITICAL");
      portENTER_CRITICAL(&sharedObjectMutex);

      Serial.println("TEST portEXIT_CRITICAL");
      portEXIT_CRITICAL(&sharedObjectMutex);

      Serial.println("TEST Done");
    }
    else if (USE_MUTEX)
    {

      mutex = xSemaphoreCreateMutex();

      if (mutex == NULL)
      {
        Serial.println("Failed to create mutex");
        while (1)
          ; // Halt execution if mutex creation fails
      }
      Serial.println("Create mutex success");
    }
  }

  ~SharedClass()
  {
    if (mutex != NULL)
    {
      vSemaphoreDelete(mutex);
    }
  }

  void incrementSharedVariable(int value)
  {

    if (USE_LOCK)
    {
      portENTER_CRITICAL(&sharedObjectMutex);
      sharedVariable += value;
      portEXIT_CRITICAL(&sharedObjectMutex);
    }
    else if (USE_MUTEX)
    {
      // portMAX_DELAY
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
      { // Timeout of 100ms
        sharedVariable += value;
        xSemaphoreGive(mutex);
      }
      else
      {
        Serial.println("Failed to take mutex in setSharedVariable");
      }
    }
    else
    {
      sharedVariable += value;
    }
  }

  void setSharedVariable(int value)
  {

    if (USE_LOCK)
    {
      portENTER_CRITICAL(&sharedObjectMutex);
      sharedVariable = value;
      portEXIT_CRITICAL(&sharedObjectMutex);
    }
    else if (USE_MUTEX)
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
      { // Timeout of 100ms
        sharedVariable = value;
        xSemaphoreGive(mutex);
      }
      else
      {
        Serial.println("Failed to take mutex in setSharedVariable");
      }
    }
    else
    {
      sharedVariable = value;
    }
  }

  int getSharedVariable()
  {
    int value = -1; // Default value

    if (USE_LOCK)
    {
      portENTER_CRITICAL(&sharedObjectMutex);
      value = sharedVariable;
      portEXIT_CRITICAL(&sharedObjectMutex);
    }
    else if (USE_MUTEX)
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
      { // Timeout of 100ms
        value = sharedVariable;
        xSemaphoreGive(mutex);
      }
      else
      {
        Serial.println("Failed to take mutex in getSharedVariable");
      }
      return value;
    }
    else
    {
      return sharedVariable;
    }
  }
};

struct TaskParamsHolder
{
  void *sharedObj;
};

void task1(void *parameter)
{

  // void **params = static_cast<void **>(parameter);
  // void *paramsX = *params;
  // SharedClass *sharedObjectx = static_cast<SharedClass *>(paramsX);

  // SharedClass *sharedObjectx = ((TaskParamsHolder *)parameter)->sharedObj;
  SharedClass *sharedObjectx = static_cast<SharedClass *>(((TaskParamsHolder *)parameter)->sharedObj);

  // SharedClass* sharedObj = static_cast<SharedClass*>(paramsX);

  Serial.println("task1");
  for (;;)
  {
    Serial.println("Resetting");
    sharedObjectx->setSharedVariable(100);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void task2(void *parameter)
{

  // SharedClass *sharedObjectx = ((TaskParamsHolder *)parameter)->sharedObj;
  SharedClass *sharedObjectx = static_cast<SharedClass *>(((TaskParamsHolder *)parameter)->sharedObj);

  Serial.println("task2");
  for (;;)
  {
    int value = sharedObjectx->getSharedVariable();
    Serial.print("Shared Value: ");
    Serial.println(value);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task3(void *parameter)
{

  // SharedClass *sharedObjectx = ((TaskParamsHolder *)parameter)->sharedObj;
  SharedClass *sharedObjectx = static_cast<SharedClass *>(((TaskParamsHolder *)parameter)->sharedObj);

  Serial.println("task3");
  for (;;)
  {
    // Serial.println("task3!!!");
    //  sharedObject->setSharedVariable(sharedObject->getSharedVariable() + 1);
    sharedObjectx->incrementSharedVariable(1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// SharedClass *sharedObject;
TaskParamsHolder paramsX;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    /* code */
  }

  paramsX.sharedObj = new SharedClass();
  auto params = &paramsX;

  // void *paramsX; // <--- must go outside this scope.
  // paramsX = new SharedClass();
  // auto params = &paramsX;

  // void *params = NULL;

  auto thisCore = xPortGetCoreID();
  auto theOtherCore = thisCore;
  if (theOtherCore == 1)
  {
    theOtherCore = 0;
  }

  Serial.printf("thisCore %d\n", thisCore);
  Serial.printf("theOtherCore %d\n", theOtherCore);
  // Serial.printf("objectCore %d\n", sharedObject->objectCore);
  // sharedObject->objectCore = thisCore;

  // SharedClass *sharedObjectx = &sharedObject;
  // Serial.println("TEST");
  // sharedObjectx->setSharedVariable(1002);
  // Serial.println(sharedObjectx->getSharedVariable());
  // delay(1000);

  xTaskCreatePinnedToCore(task1, "Task1", 10000, params, 1, NULL, thisCore);

  delay(100);
  xTaskCreatePinnedToCore(task2, "Task2", 10000, params, 1, NULL, theOtherCore);

  delay(2000);
  Serial.println("Task3a");
  xTaskCreatePinnedToCore(task3, "Task3a", 10000, params, 1, NULL, theOtherCore);

  delay(2000);
  Serial.println("Task3b");
  xTaskCreatePinnedToCore(task3, "Task3b", 10000, params, 1, NULL, thisCore);

  delay(2000);
  Serial.println("Task3c");
  xTaskCreatePinnedToCore(task3, "Task3c", 10000, params, 1, NULL, theOtherCore);
}

void loop()
{
  Serial.println("loop");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
/***************************************************/

// const int button1Pin = 2; // update config
// const int button2Pin = 4; // kill wifi

// const int servoPin = 32;
// const int potPin = 34;

// int button1State = 0;
// int button2State = 0;

// int potValue = 0;

// int pressDownHoldTime = 250;
// int startAngle = 90;

// Servo myServo; // Create a Servo object

MyIoTHelper helper("SmartAC");
TempHelper *tempHelper;

// bool btn1IsDown = false;
// bool btn2IsDown = true;

void _setup()
{

  helper.Setup();

  Serial.print("Running on core: ");
  Serial.println(xPortGetCoreID());

  showStartReason();

  helper.wiFiBegin("DarkNet", "7pu77ies77");

  tempHelper = new TempHelper(&helper);

  tempHelper->begin();

  // pinMode(button1Pin, INPUT_PULLUP);
  // pinMode(button2Pin, INPUT_PULLUP);
  // myServo.attach(servoPin);

  // Serial.printf("helper.servoHomeAngle: %d\n", helper.servoHomeAngle);
  // myServo.write(helper.servoHomeAngle);
}

int lastPotValue = -1;

void _loop()
{

  // // Read the state of the button
  // button1State = digitalRead(button1Pin);
  // button2State = digitalRead(button2Pin);

  // // Serial.printf("btn1 %d -- btn2 %d\n", button1State, button2State);
  // if (button2State == HIGH)
  // {
  //   // Serial.println("btn2 low");
  //   btn2IsDown = false;
  // }
  // else
  // {

  //   if (!btn2IsDown)
  //   {
  //     Serial.println("btn2 down!!!!!");
  //     helper.chaos("wifi");
  //     btn2IsDown = true;
  //   }
  // }

  // //   if (button1State == HIGH)
  // //   {
  // //     // Serial.println("btn2 low");
  // //     btn1IsDown = false;
  // //   }
  // //   else
  // //   {

  // //     if (!btn1IsDown)
  // //     {
  // //       Serial.println("btn1 down!!!!");
  // //     }
  // //     btn1IsDown = true;
  // //   }

  // if (button1State == LOW)
  // {
  //   if (!btn1IsDown)
  //   {

  //     helper.updateConfig();

  //     Serial.print("angle:");
  //     Serial.println(helper.servoAngle);
  //     Serial.print("pressDownHoldTime:");
  //     Serial.println(helper.pressDownHoldTime);

  //     myServo.write(helper.servoAngle);
  //     delay(helper.pressDownHoldTime);
  //     myServo.write(helper.servoHomeAngle);

  //     btn1IsDown = true;
  //   }
  // }
  // else
  // {
  //   btn1IsDown = false;
  //   // Serial.print("pot:");
  //   // Serial.print(potValue);
  //   // Serial.println();
  //   // Serial.println("Button not pressed.");
  // }

  // auto endTime = micros();
  // auto timeTaken = endTime - startTime;
  // if (timeTaken > 16666)
  // {
  //   return;
  // }
  // delayMicroseconds(16666 - timeTaken); // Small delay to avoid bouncing issues
}
