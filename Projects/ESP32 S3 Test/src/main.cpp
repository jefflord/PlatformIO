#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include "extra.h"

#include "A.h"
#include "B.h"
#include "X.h"

// #define configGENERATE_RUN_TIME_STATS 1
// #define portGET_RUN_TIME_COUNTER_VALUE() (esp_task_get_run_time_stats())

const char *ssid = "DarkNet";
const char *password = "7pu77ies77";

// Create AsyncWebServer object on port 80
AsyncWebServer server(8222);

// #include <NeoPixelBus.h>
const uint16_t PixelCount = 1; // Number of LEDs
const uint8_t PixelPin = 48;   // GPIO pin connected to the LED strip

// #define TOUCH_PIN 12
#define TOUCH_PIN 10

int counter = 0;
unsigned long *healthTimes;

// NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod> strip(PixelCount, PixelPin);

unsigned long lastMillis;
int current = 0;
int sleepTime = 1000;
int workTime = 50;
int bufferTime = 20;
int maxTasks = 100;
int modWorkCheck = 4;
int workDone = 0;
int delayBetweenStarting = 250;
bool doStressTest = false;

TaskHandle_t ledTaskHandle = NULL;

void testTasks(void);

void IRAM_ATTR touchISR()
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(ledTaskHandle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

bool ledState = false;
bool expectedLedState = false;

void ledControlTask(void *pvParameters)
{

  for (;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      Serial.printf("new ledState: %s\n", ledState ? "true" : "false");
    }
  }
}

void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  Serial.println("json!");
  // Parse incoming JSON
  JsonDocument jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, data);

  if (error)
  {
    Serial.println("Error parsing JSON");
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Extract data from JSON
  const char *name = jsonDoc["name"];
  int age = jsonDoc["age"];

  if (jsonDoc.containsKey("ledState"))
  {
    int newLedState = (int)jsonDoc["ledState"];

    if (newLedState == 2)
    {
      newLedState = !ledState;
      ledState = newLedState;
    }

    digitalWrite(LED_BUILTIN, newLedState);
    Serial.printf("force ledState to %d -- %d\n", newLedState, ledState);
  }
  else
  {
    Serial.printf("no new ledState\n");
  }

  // Print received data
  // Serial.printf("Name: %s, Age: %d\n", name, age);

  // int currentLedState = digitalRead(LED_BUILTIN);
  // sprintf()

  JsonDocument doc;

  // Add values in the document
  doc["sensor"] = "gps";
  doc["status"] = "success";
  doc["ledState"] = ledState;
  doc["millis"] = millis();

  String responseData = "";
  serializeJsonPretty(doc, responseData);
  // Send a JSON response with CSP header
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseData);

  // Add Content Security Policy (CSP) header
  response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");

  // Send the response
  request->send(response);
}

void setupServer()
{

  server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("html!");
              String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web Server!!!!</title></head><body><h1>Hello from ESP32!!!!</h1></body></html>";
              
              // Send a JSON response with CSP header
              AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);

              // Add Content Security Policy (CSP) header
              response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");
              response->addHeader("Test", "!!!");

              // Send the response
              request->send(response); });

  server.on("/htmlx", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("htmlx!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmlx!</title></head><body><h1>Hello from htmlx!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

  server.on("/htmly", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("htmly!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmly!</title></head><body><h1>Hello from htmly!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

  // server.on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
  //           { handleJsonPost(request, data, len, index, total); }

  server.on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleJsonPost);

  // Start the server

  server.begin();
}

Preferences preferences;

String currentVersion = "0.0.0.0";

void setup()
{
  Serial.begin(115200);

  while (!Serial)
    continue;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(".");
  }

  Serial.printf("Connected %s\n", WiFi.localIP().toString());

  setupServer();

  Serial.printf("setupServer done\n");

  lastMillis = millis();

  // strip.Begin();
  // strip.Show();

  if (doStressTest)
  {
    Serial.println("Waiting");
    delay(1000);
    Serial.println("Starting doStressTest");
    testTasks();
  }

  xTaskCreate(ledControlTask, "LED Control", 2048, NULL, 1, &ledTaskHandle);
  touchAttachInterrupt(TOUCH_PIN, touchISR, 8191 * 0.6);

  preferences.begin("my_app_settings", false);

  currentVersion = preferences.getString("currentVersion", currentVersion);

  ArduinoOTA.begin();
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

  int lastCoreId = -1;
  int currentCoreId = -1;

  int workCounter = 0;
  int num = (*(int *)p) - 1;

  createCpuLoad(100);
  for (int x = 0;; x++)
  {

    currentCoreId = (int)xPortGetCoreID();

    // if ((x % 20) == 0)
    // {
    //   Serial.printf("alive -> %s on %d\n", taskName, currentCoreId);
    // }

    if (currentCoreId != lastCoreId)
    {
      // Serial.printf("%s (%d of %d) from %d to %d\n", taskName, num, maxx, lastCoreId, currentCoreId);
      lastCoreId = currentCoreId;
    }
    if ((workCounter++ % modWorkCheck) == 0)
    {
      workDone += workTime;
      // Serial.printf("%d is doing work\n", num);
      createCpuLoad(workTime);
    }
    else
    {
      // Serial.println("not doing work");
    }
    healthTimes[num] = millis();

    vTaskDelay(sleepTime / portTICK_PERIOD_MS);
  }
}

void healthTimesCheck(void *p)
{
  bool allHealth = true;
  int waitAllowed = (sleepTime + workTime + bufferTime);

  printf("Starting healthTimesCheck, waitAllowed:%d\n", waitAllowed);

  for (int x = 0;; x++)
  {

    auto currentTime = millis();
    allHealth = true;
    for (int i = 0; i < maxTasks; i++)
    {
      auto diff = currentTime - healthTimes[i];
      if (diff < 0 || diff > 10000)
      {
        continue;
      }

      if (diff > waitAllowed)
      {
        allHealth = false;
        Serial.printf("### %d is taking too long %d should be < %d\n", i, diff, waitAllowed);
      }

      // Serial.printf("%d is taking too long %d (%d-%d)\n", i, diff, currentTime, healthTimes[i]);
    }

    if (allHealth)
    {
      Serial.printf("All %d are good, workDone: %d\n", maxTasks, workDone);
    }
    vTaskDelay(sleepTime / portTICK_PERIOD_MS);
  }
}

void testTasks(void)
{
  printf("Starting testTasks...\n");

  healthTimes = (unsigned long *)malloc(maxTasks * sizeof(unsigned long)); // Allocate memory for the array

  current = 0;

  bool startedChecker = false;

  while (1)
  {
    char buffer[20]; // Buffer to store the formatted result
    sprintf(buffer, "doWork%d", current);

    if (current < maxTasks)
    {
      current++;
      xTaskCreate(doWork, buffer, 2048, &current, 1, NULL);
    }
    else
    {
      if (!startedChecker)
      {
        printf("Starting healthTimesCheck...\n");
        xTaskCreate(healthTimesCheck, "healthTimesCheck", 2048, NULL, 1, NULL);
        startedChecker = true;
      }
    }
    vTaskDelay(delayBetweenStarting / portTICK_PERIOD_MS); // Wait for 1 second
  }
}

int lastTouchValue = 0;

void vDelay(unsigned long delayMs)
{
  vTaskDelay(pdMS_TO_TICKS(delayMs));
}

int loopCounter = 0;

void loop()
{

  B b;
  A a;
  X x;

  a.setup(&b);
  b.setup(&a);

  x.setup(&a, &b);

  Serial.println(1);
  a.hello(true);
  Serial.println(2);
  b.hello(true);
  Serial.println(3);
  x.hello(true);

  if ((loopCounter++ % 10) == 0)
  {
    auto version = getLatestVersion();

    if (currentVersion != version)
    {
      Serial.printf("got new version! => [%s]\n", version.c_str());

      auto result = performUpdate();
      if (result == HTTP_UPDATE_OK)
      {
        currentVersion = version;
        preferences.putString("currentVersion", currentVersion);
      }
    }
    else
    {
      Serial.printf("SAME => [%s]\n", version.c_str());
    }
  }

  Serial.println("...");
  ArduinoOTA.handle();
  vDelay(3000);
  return;
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
