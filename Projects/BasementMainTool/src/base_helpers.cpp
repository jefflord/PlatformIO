
#include <base_helpers.h>

bool dnsReady = false;
AsyncWebServer server(8222);

#define ld2450_01_TX 17
#define ld2450_01_RX 16

#define ld2450_02_TX 25
#define ld2450_02_RX 26
LD2450 ld2450_01;
LD2450 ld2450_02;

extern GlobalState globalState;

void wiFiBegin()
{
    // Try to connect to the saved WiFi credentials

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin();
    }

    // Wait for connection
    int timeout = 20000; // Timeout after 10 seconds
    int elapsed = 0;
    Serial.println("\nStarting WiFi...");

    // flash
    while (WiFi.status() != WL_CONNECTED && elapsed < timeout)
    {
        delay(1000);
        Serial.print("O");
        elapsed += 1000;
    }

    // If not connected, start SmartConfig
    if (WiFi.status() != WL_CONNECTED)
    {

        Serial.println("\nStarting SmartConfig...");
        WiFi.beginSmartConfig();

        timeout = 2 * 60 * 1000;
        elapsed = 0;

        // Wait for SmartConfig to finish
        while (!WiFi.smartConfigDone() && elapsed < timeout)
        {
            delay(1000);
            Serial.print("X");
            elapsed += 1000;
        }

        if (!WiFi.smartConfigDone())
        {
            Serial.println("smartConfigDone failed, rebooting.");
            ESP.restart();
        }

        Serial.println("\nSmartConfig done.");
        // Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
    }

    Serial.printf("\nWiFi Connected %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    WiFiUDP ntpUDP;
    NTPClient *timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
    timeClient->begin();

    auto updated = timeClient->update();

    Serial.print("Current time: ");
    Serial.println(timeClient->getFormattedTime());
}

OTAStatus *MySetupOTA()
{
    OTAStatus *otaStatus = new OTAStatus();

    ArduinoOTA.setTimeout(20000); // 3 minutes
    ArduinoOTA.onStart([otaStatus]()
                       {
                         String type;
                         if (ArduinoOTA.getCommand() == U_FLASH)
                         {
                           type = "program";
                         }
                         else
                         { // U_SPIFFS
                           type = "filesystem";
                         }

                         Serial.println("OTA Update Starting: " + type);

                         otaStatus->ArduinoOTARunning = true; });

    ArduinoOTA.onEnd([otaStatus]()
                     { 
                      otaStatus->ArduinoOTARunning = false;
                      Serial.println("\nOTA Update Finished"); });

    ArduinoOTA.onProgress([otaStatus](unsigned int progress, unsigned int total)
                          { if ((progress / (total / 100) != otaStatus->otaLastPercent)) {
                            otaStatus->otaLastPercent = (progress / (total / 100));
                            Serial.printf("Progress: %u%% (%u/%u)\n", otaStatus->otaLastPercent, progress, total);
                              } });

    ArduinoOTA.onError([otaStatus](ota_error_t error)
                       {
                        otaStatus->ArduinoOTARunning = false;
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        } });

    ArduinoOTA.begin();

    return otaStatus;
}

void mDNSSetup(void *p)
{
    auto waitTimeout = millis() + 100;
    // Serial.println("MDNS setup waiting start");
    while (waitTimeout > millis() || WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        // Serial.println("MDNS setup waiting!");
    }
    // Serial.println("MDNS setup waiting done");

    if (!MDNS.begin("basementtool"))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        Serial.println("Device can be accessed at basementtool.local");
        // vTaskDelay(pdMS_TO_TICKS(100));
        //  if (MDNS.addService("http", "tcp", 80))
        //  {
        //    Serial.println("addService 1 worked");
        //  }

        dnsReady = true;
        vTaskDelete(NULL);
    }
}

void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    Serial.println("json!");

    // Parse incoming JSON
    JsonDocument jsonRequestDoc;
    DeserializationError error = deserializeJson(jsonRequestDoc, data);

    if (error)
    {
        Serial.println("Error parsing JSON");
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // Extract data from JSON
    bool sensorOn = jsonRequestDoc["sensorOn"];
    bool update = jsonRequestDoc.containsKey("update") ? jsonRequestDoc["update"] : false;

    // sensorOn ? ld2450_01.start() : ld2450_01.stop();

    JsonDocument jsonDoc;
    jsonDoc["sensorInit"] = globalState.sensorInit;

    if (jsonRequestDoc.containsKey("sensorOn"))
    {
        if (sensorOn == false)
        {
            globalState.sensorInit = false;
        }

        jsonDoc["_old_sensorOn"] = globalState.sensorOn;
        globalState.sensorOn = sensorOn;
        jsonDoc["sensorOn"] = globalState.sensorOn;
        jsonDoc["sensorInit"] = globalState.sensorInit;
    }

    // Add values in the document

    if (update)
    {
        unsigned long startTime = 0;
        bool didInit = false;

        if (globalState.sensorInit == false)
        {
            startTime = millis();
            Serial.printf("Sensor INIT (1) AT %lu ms\n", millis() - startTime);
            Serial2.begin(256000, SERIAL_8N1, ld2450_01_RX, ld2450_01_TX); // TX=17, RX=16

            Serial.printf("Sensor INIT (2) AT %lu ms\n", millis() - startTime);

            ld2450_01.begin(Serial2); // Pass Serial2 to the library

            Serial.printf("Sensor INIT (3) AT %lu ms\n", millis() - startTime);

            ld2450_01.waitForSensorMessage(false);
            Serial.printf("Sensor INIT (4) AT %lu ms\n", millis() - startTime);

            Serial.printf("Sensor INIT (5) AT %lu ms\n", millis() - startTime);

            ld2450_01.waitForSensorMessage(false);
            Serial.printf("Sensor INIT (6a) AT %lu ms\n", millis() - startTime);

            auto testRead = -1;
            auto maxReadTries = 10;
            auto readTries = 0;
            while (testRead == -1 && readTries < maxReadTries)
            {
                readTries++;
                testRead = ld2450_01.read();
                if (testRead == -1)
                {
                    delay(20);
                }
            }

            Serial.printf("Sensor INIT (6b) AT %d, readTries:%i AT %lu ms\n", testRead, readTries, millis() - startTime);

            globalState.sensorInit = true;
            didInit = true;
        }
        else
        {
            startTime = millis();
        }

        if (ld2450_01.read() > 0)
        {
            unsigned long duration = millis() - startTime;
            Serial.printf("Sensor read took %lu ms\n", duration);
            const int found_targets = ld2450_01.read();
            if (found_targets > 0)
            {

                auto lastTargetMessage = ld2450_01.getLastTargetMessage();
                lastTargetMessage.trim();
                jsonDoc["lastTargetMessage"] = lastTargetMessage;
                jsonDoc["didInit"] = didInit;
                jsonDoc["initDuration"] = duration;

                JsonArray targets = jsonDoc["targets"].to<JsonArray>();

                // src/main.cpp: In function 'void handleGetHtml(AsyncWebServerRequest*)':
                // src/main.cpp:78:62: warning: 'ArduinoJson::V730PB22::JsonArray ArduinoJson::V730PB22::JsonDocument::createNestedArray(TChar*) [with TChar = const char]' is deprecated: use doc[key].to<JsonArray>() instead [-Wdeprecated-declarations]

                for (int i = 0; i < found_targets; i++)
                {
                    const LD2450::RadarTarget valid_target = ld2450_01.getTarget(i);
                    JsonObject target = targets.add<JsonObject>();
                    target["index"] = i;
                    target["distance"] = valid_target.distance;
                    target["x"] = valid_target.x;
                    target["y"] = valid_target.y;
                    target["speed"] = valid_target.speed;
                }
            }
        }
    }

    String responseData = "";
    serializeJsonPretty(jsonDoc, responseData);
    // Send a JSON response with CSP header
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseData);

    // Add Content Security Policy (CSP) header
    // response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");

    // response->addHeader("Access-Control-Allow", "*");        // Allow all origins
    // response->addHeader("Access-Control-Allow-Origin", "*"); // Allow all origins

    // Send the response
    request->send(response);
}

void handlePostXHtml(AsyncWebServerRequest *request)
{
    // not sure what we'd do here
    // Serial.println("handlePostXHtml!");
}

void handleGetHtml(AsyncWebServerRequest *request)
{
    Serial.println("html!");

    String currentTime = "";
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
    timeClient.begin();
    while (!timeClient.update())
    {
        delay(100);
    }
    currentTime = timeClient.getFormattedTime();

    String html = R"html(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <script src="https://code.jquery.com/jquery-3.7.1.min.js" 
                integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo=" 
                crossorigin="anonymous"></script>
        <title>Mobile-Friendly Page</title>
        <style>
            /* Add some basic styles for responsiveness */
            body {
                margin: 0;
                padding: 0;
                font-family: Arial, sans-serif;
                line-height: 1.6;
                background-color: #f4f4f9;
                color: #333;
            }
            .container {
                max-width: 600px;
                margin: 0 auto;
                padding: 20px;
            }
            h1, p {
                text-align: center;
            }
            button {
                display: block;
                width: 100%;
                padding: 10px;
                margin: 10px 0;
                font-size: 16px;
                border: none;
                background-color: #007BFF;
                color: white;
                border-radius: 5px;
                cursor: pointer;
            }
            button:hover {
                background-color: #0056b3;
            }
        </style>
    </head>
    <body>
        <div class="container">
            Loaded at <strong>)html" +
                  currentTime + R"html(</strong>
            <button id="update" type="button">Reload</button>
            <button id="sensorOff" type="button">Off</button>
            <button id="sensorOn" type="button">On</button>
            <br>
            <pre id="sensorData"></pre>
        </div>
        <script>
            // Add a script to the page
            console.log('Script added to the page');            
            $('#sensorOn').click(function() {
                $.ajax({
                    url: '/json',
                    type: 'POST',
                    data: JSON.stringify({ sensorOn: true }),
                    contentType: 'application/json; charset=utf-8',
                    dataType: 'json',
                    success: function(data) {
                        console.log('Success:', data);
                    },
                    error: function(xhr, status, error) {
                        console.error('Error:', status, error);
                    },
                    complete: function() {
                        console.log('Request completed');
                    }
                });
            });
            $('#sensorOff').click(function() {
                $.ajax({
                    url: '/json',
                    type: 'POST',
                    data: JSON.stringify({ sensorOn: false }),
                    contentType: 'application/json; charset=utf-8',
                    dataType: 'json',
                    success: function(data) {
                        console.log('Success:', data);
                    },
                    error: function(xhr, status, error) {
                        console.error('Error:', status, error);
                    },
                    complete: function() {
                        console.log('Request completed');
                    }
                });
            });

            $('#update').click(function() {
                $.ajax({
                    url: '/json',
                    type: 'POST',
                    data: JSON.stringify({ update: true }),
                    contentType: 'application/json; charset=utf-8',
                    dataType: 'json',
                    success: function(data) {
                        console.log('Success:', data);
                        $("#sensorData").text(JSON.stringify(data, null, 2));                        
                    },
                    error: function(xhr, status, error) {
                        console.error('Error:', status, error);
                    },
                    complete: function() {
                        console.log('Request completed');
                    }
                });
            });

        </script>
    </body>
    </html>
    )html";

    // Send a JSON response with CSP header
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);

    // Add Content Security Policy (CSP) header
    // response->addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'");
    // response->addHeader("Test", "!!!");

    // Send the response
    request->send(response);
}

void setupServer()
{

    server.on("/", HTTP_GET, handleGetHtml);

    server.on("/json", HTTP_POST, handlePostXHtml, NULL, handleJsonPost);

    server.begin();
}
