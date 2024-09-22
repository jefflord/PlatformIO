#include "WebServerHelper.h"
#include "ButtonServerHelper.h"
#include "MyIoTHelper.h"

WebServerHelper::WebServerHelper()
{
    server = new AsyncWebServer(8222);
}

void WebServerHelper::handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    auto buttonServerHelper = ButtonServerHelper::GetButtonServerHelper();
    safeSerial.println("json!");
    // Parse incoming JSON
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, data);

    if (error)
    {
        safeSerial.println("Error parsing JSON");
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // Extract data from JSON
    String action = String((const char *)jsonDoc["action"]);

    if (action == "chaos")
    {
        safeSerial.println("chaos !!!!!!");
        buttonServerHelper->iotHelper->chaos("wifi");
        
    }

    if (action == "hitSwitch")
    {
        safeSerial.println("pushServoButton start !!!!!!");
        buttonServerHelper->pushServoButton();
        safeSerial.println("pushServoButton end");
    }

    // Print received data
    // Serial.printf("Name: %s, Age: %d\n", name, age);

    // int currentLedState = digitalRead(LED_BUILTIN);
    // sprintf()

    JsonDocument doc;

    // Add values in the document
    doc["sensor"] = "gps";
    doc["status"] = "success";
    doc["millis"] = millis();

    String responseData = "";
    serializeJsonPretty(doc, responseData);
    // Send a JSON response with CSP header
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseData);

    // Add Content Security Policy (CSP) header
    response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");
    response->addHeader("Access-Control-Allow", "*");        // Allow all origins
    response->addHeader("Access-Control-Allow-Origin", "*"); // Allow all origins

    // Send the response
    request->send(response);
}

void WebServerHelper::begin()
{

    SPIFFS.begin(true);

    // esp_task_wdt_dump();

    server->on("/js/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
               {              
              safeSerial.println("main.js request");
              request->send(SPIFFS, "/wwwroot/js/main.js", "text/html"); });

    server->on("/spiffs", HTTP_GET, [](AsyncWebServerRequest *request)
               {              
              safeSerial.println("spiffs request");
              request->send(SPIFFS, "/wwwroot/index.html", "text/html"); });

    // server.on("/spiffsgz", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {
    //   AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html.gz", "text/html");
    //   response->addHeader("Content-Encoding", "gzip");
    //   request->send(response); });

    server->on("/html", HTTP_GET, [](AsyncWebServerRequest *request)
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

    server->on("/htmlx", HTTP_GET, [](AsyncWebServerRequest *request)
               {
              Serial.println("htmlx!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmlx!</title></head><body><h1>Hello from htmlx!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

    server->on("/htmly", HTTP_GET, [](AsyncWebServerRequest *request)
               {
              Serial.println("htmly!");
    String html = "<!DOCTYPE html><html><head><link rel='icon' href='data:,'><title>ESP32 Web htmly!</title></head><body><h1>Hello from htmly!</h1></body></html>";
    
    // Send the HTML response
    request->send(200, "text/html", html); });

    // server.on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    //           { handleJsonPost(request, data, len, index, total); }

    server->on("/json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, WebServerHelper::handleJsonPost);
    server->on("/json", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
               {
        AsyncWebServerResponse *response = request->beginResponse(204);  // 204 No Content
        response->addHeader("Access-Control-Allow-Origin", "*");  // Allow any origin
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        response->addHeader("Access-Control-Max-Age", "3600");  // Cache the preflight for 1 hour
        request->send(response); });

    server->begin();
}
