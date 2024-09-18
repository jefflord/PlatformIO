#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void checkpsram();

int anInt = 0;
class Person
{
private:
  std::string name;
  int age;
  std::string gender;

public:
  // Constructor
  Person(std::string personName, int personAge, std::string personGender)
      : name(personName), age(personAge), gender(personGender) {}

  // Getters
  std::string getName() const
  {
    return name;
  }

  int getAge() const
  {
    return age;
  }

  std::string getGender() const
  {
    return gender;
  }

  // Setters
  void setName(std::string personName)
  {
    name = personName;
  }

  void setAge(int personAge)
  {
    age = personAge;
  }

  void setGender(std::string personGender)
  {
    gender = personGender;
  }
};

void test(void *pvParameters)
{

  Person *person = (Person *)pvParameters;

  int *value = (int *)pvParameters;
  int core = xPortGetCoreID();

  for (;;)
  {
    anInt++;
    if (person->getAge() > 100)
    {
      Serial.printf("OLD!!! name %s\n", person->getName().c_str());
    }
    else
    {
      Serial.printf("name %s\n", person->getName().c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int x = 5;

// Person person("bob", 47, "viz");
Person *person;
void setup()
{


  person = new Person("bob", 47, "viz");

  Serial.begin(115200);
  while (!Serial)
    continue;

  // checkpsram();

  Serial.println("start");
  // delay(1000);

  int *a = (int *)malloc(sizeof(int));
  *a = 555; // Assign the value
  int *b = (int *)malloc(sizeof(int));
  *b = 666; // Assign the value

  // delay(3000);
  Serial.printf("a:%d, b:%d \n", *a, *b);
  xTaskCreatePinnedToCore(test, "test1", 2048, person, 1, NULL, 0);

  vTaskDelay(pdMS_TO_TICKS(500));
  // xTaskCreatePinnedToCore(test, "test2", 2048, person, 1, NULL, 1);
}

void loop()
{

  x--;
  Serial.printf("1 - anInt:%d,  x:%d \n", anInt, x);
  int divX = 0;
  if (x != 0)
  {
    divX = 5000 / x;
  }

  // auto xxxx = 10000 / x;
  // Serial.printf("2 - anInt:%d,  x:%d, xxxx:%d \n", anInt, x, xxxx);

  anInt++;
  int *a = (int *)malloc(sizeof(int));
  *a = 555; // Assign the value
  int *b = (int *)malloc(sizeof(int));
  *b = 666; // Assign the value

  // Serial.printf("a:%d, b:%d, anInt:%d, divX:%d, x:%d \n", *a, *b, anInt, divX, x);

  // Serial.printf("loop %d\n", anInt);
  //  vTaskDelay(pdMS_TO_TICKS(2000));
  delay(2000);
}

void checkpsram()
{

  // Check if PSRAM is available
  if (psramFound())
  {
    Serial.println("PSRAM found!");
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize() / 1024); // Convert to KB
    Serial.println(" KB");
  }
  else
  {
    Serial.println("PSRAM not found.");
  }
  Serial.println("Started");
}