
#include <base_helpers.h>

bool dnsReady = false;
AsyncWebServer server(8222);

#define ld2450_02_TX 17
#define ld2450_02_RX 16

#define ld2450_01_TX 25
#define ld2450_01_RX 26

#ifdef DXSENSOR_NODE
LD2450 ld2450_01;
LD2450 ld2450_02;
#endif

extern GlobalState globalState;
extern ThreadSafeSerial safeSerial;

// Create a struct_message to hold incoming data

bool espNowWorking = false;

int OnDataRecvCounter = 0;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{

    if (!globalState.isNode)
    {
        safeSerial.println("I am not a node, I will not process this message");
        return;
    }

    OnDataRecvCounter++;

    // if (OnDataRecvCounter % 100 == 0)
    // {
    //     safeSerial.printf("OnDataRecvCounter: %d\r\n", OnDataRecvCounter);
    // }

    /*
        globalState.lastNowMessage.update_MacAddress: the mac address of the node that CHANGING token status
        globalState.lastNowMessage.update_status: the status of the node that is CHANGING token status, 0 = update_MacAddress no token, 1 = update_MacAddress has token
        globalState.lastNowMessage.tokenHolder_MacAddress: the mac address of the node that CURRENTLY has the token, according to the node that sent this message
    */

    if (len < sizeof(struct_message) || len > struct_message_MAX_EXPECTED_DATA_SIZE)
    {
        // print "invalid message size" and include len, and sizeof(struct_message), and struct_message_MAX_EXPECTED_DATA_SIZE
        Serial.printf("invalid message size %i, %i, %i\n", len, sizeof(struct_message), struct_message_MAX_EXPECTED_DATA_SIZE);
        return;
    }

    memcpy(&globalState.lastNowMessage, incomingData, sizeof(globalState.lastNowMessage));

    // check type, make sure it's "bmt"
    if (strncmp(globalState.lastNowMessage.type, DATA_TYPE_NAME, 3) != 0)
    {
        Serial.println("invalid message type");
        return;
    }
    else
    {
        // Serial.println("type:OK");
    }

    // make sure the version is correct
    if (globalState.lastNowMessage.version != DATA_VERSION)
    {
        Serial.println("invalid message version");
        return;
    }
    else
    {
        // Serial.println("version:OK");
    }

    // safeSerial.printf("DEBUG OnDataRecv: %s %d, %s \r\n", convertMacAddressToString(globalState.lastNowMessage.update_MacAddress), globalState.lastNowMessage.update_status, convertMacAddressToString(globalState.lastNowMessage.tokenHolder_MacAddress));

    if (globalState.lastNowMessage.update_status == 1)
    {
        globalState.nodes.FreeTokenEverywhere();
    }

    auto node = globalState.nodes.GetByMac(globalState.lastNowMessage.update_MacAddress);
    auto nodeExists = node != nullptr;
    if (node == nullptr)
    {
        safeSerial.printf("OnDataRecv: someone is new to the party => %s\r\n", convertMacAddressToString(globalState.lastNowMessage.update_MacAddress));
        globalState.nodes.AddNode(new Node("new", globalState.lastNowMessage.update_MacAddress, globalState.lastNowMessage.update_status, 0));
    }
    else
    {
        // safeSerial.printf("OnDataRecv: updating node: %s, status: %d\n", convertMacAddressToString(globalState.lastNowMessage.update_MacAddress), globalState.lastNowMessage.update_status);
        globalState.nodes.SetStatusByMac(globalState.lastNowMessage.update_MacAddress, globalState.lastNowMessage.update_status);
    }

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // safeSerial.println("A!");
    //  check to see if broadcastAddress is the same as globalState.lastNowMessage.tokenHolder_MacAddress

    if (globalState.lastNowMessage.update_status == 0 && memcmp(broadcastAddress, globalState.lastNowMessage.tokenHolder_MacAddress, 6) == 0)
    {
        // they are not handing off the token and who they think has it is the broadcast address (or not us)
        safeSerial.printf("%s does not have the token and does not know who does!!!\r\n", convertMacAddressToString(globalState.lastNowMessage.update_MacAddress));
    }
    else if (globalState.lastNowMessage.update_status == 0 && memcmp(broadcastAddress, globalState.lastNowMessage.tokenHolder_MacAddress, 6) != 0)
    {
        // if this is not saying who the token is for, then we need to check to see who they think has the token, maybe it's us!
        if (memcmp(globalState.macAddress, globalState.lastNowMessage.tokenHolder_MacAddress, 6) == 0)
        {
            // the don't have the token, but they think we do!
            auto me = globalState.nodes.GetByMac(globalState.macAddress);
            if (me->status != 1)
            {
                // they think we have the token, but we don't, so we should take it!
                auto timeSinceLastWork = millis() - me->lastWorkTime;
                if (timeSinceLastWork > 1000)
                {
                    safeSerial.printf("Last work %ld ms ago, I will take the token. (status:%d)\r\n", timeSinceLastWork, globalState.lastNowMessage.update_status);
                    me->status = 1;
                }
                else
                {
                    // safeSerial.printf("I worked just %ld ms ago, I will NOT take the token yet. (status:%d)\r\n", timeSinceLastWork, globalState.lastNowMessage.update_status);
                }
            }
        }
    }

    // Serial.printf("bmt command received: %s\r\n", globalState.lastNowMessage.action);
    globalState.lastNowMessageReady = true;
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    return;
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Message sent successfully: " : "Message failed to send to: ");

    for (int i = 0; i < 6; i++)
    {
        Serial.printf("%02X", mac[i]);
        if (i < 5)
            Serial.print(":"); // Add colon between bytes
    }
    Serial.println("");
}

void getPeer(uint8_t *dest)
{
    uint8_t bmtool1[] = {0x08, 0xA6, 0xF7, 0xBC, 0x91, 0xCC};
    uint8_t bmtool2[] = {0x4C, 0x11, 0xAE, 0x73, 0xF1, 0x4C};
    uint8_t currentMac[6];
    WiFi.macAddress(currentMac);

    bool matches = true;
    for (int i = 0; i < 6; i++)
    {
        if (currentMac[i] != bmtool1[i])
        {
            matches = false;
            break;
        }
    }

    memcpy(dest, matches ? bmtool2 : bmtool1, 6);
}

void wiFiBegin()
{
    // Try to connect to the saved WiFi credentials

    WiFi.mode(WIFI_STA);

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

    Serial.printf("\nWiFi Connected %s, %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());

    WiFiUDP ntpUDP;
    NTPClient *timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 1000);
    timeClient->begin();

    auto updated = timeClient->update();

    Serial.print("Current time: ");
    Serial.println(timeClient->getFormattedTime());

    // // Print MAC address
    // uint8_t mac[6];
    // WiFi.macAddress(mac);
    // Serial.print("MAC Address: ");
    // for (int i = 0; i < 6; i++)
    // {
    //     Serial.printf("%02X", mac[i]);
    //     if (i < 5)
    //         Serial.print(":");
    // }
    // Serial.println();

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    else
    {
        Serial.println("ESP-NOW ready");

        // Register callback
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_register_send_cb(onDataSent);

        registerEspNowPeer();
    }
}

char *convertMacAddressToString(const uint8_t mac[6])
{
    char *macString = new char[18];
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return macString;
}

void registerEspNowPeer()
{

    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    // getPeer(peerInfo.peer_addr);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer  
    WiFi.mode(WIFI_STA);

    delay(1000);

    esp_log_level_set("*", ESP_LOG_VERBOSE); // Set the logging level to verbose

    Serial.print("ADDING PEER:");
    for (int i = 0; i < 6; i++)
    {

        Serial.printf("%02X", peerInfo.peer_addr[i]);
        if (i < 5)
            Serial.print(":"); // Add colon between bytes
    }

    Serial.println("");

    esp_err_t result;
    if (!esp_now_is_peer_exist(peerInfo.peer_addr))
    {
        if ((result = esp_now_add_peer(&peerInfo)) != ESP_OK)
        {
            Serial.println("Failed to add peer.");
            Serial.println(esp_err_to_name(result));
        }
        else
        {
            Serial.println("added peer!");
        }
    }
    else
    {
        Serial.println("peer already exists!");
    }
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
    auto myDnsName = (char *)p;

    if (myDnsName == NULL)
    {
        static char defaultName[] = "bmtool";
        myDnsName = defaultName;
    }

    auto waitTimeout = millis() + 100;
    // Serial.println("MDNS setup waiting start");
    while (waitTimeout > millis() || WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        // Serial.println("MDNS setup waiting!");
    }
    // Serial.println("MDNS setup waiting done");

    if (!MDNS.begin(myDnsName))
    {
        Serial.printf("Error setting up MDNS responder for %s.local\r\n", myDnsName);
    }
    else
    {
        Serial.printf("Device can be accessed at %s.local\r\n", myDnsName);
        dnsReady = true;
        vTaskDelete(NULL);
    }
}

#ifdef DXSENSOR_NODE
void handleJsonPost_Node(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    Serial.println("json!");

    // sendEspNowAction("js!!");

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
    JsonDocument jsonDoc;
    jsonDoc["sensorInit"] = globalState.sensorInit;

    if (jsonRequestDoc["sensorOn"].is<JsonVariant>())
    {
        bool sensorOn = jsonRequestDoc["sensorOn"];
        if (sensorOn == false)
        {
            globalState.sensorInit = false;
            digitalWrite(globalState.xBJT_PIN, LOW);
        }
        else
        {
            // digitalWrite(globalState.xBJT_PIN, HIGH);
        }

        jsonDoc["_old_sensorOn"] = globalState.sensorOn;
        globalState.sensorOn = sensorOn;
        jsonDoc["sensorOn"] = globalState.sensorOn;
        jsonDoc["sensorInit"] = globalState.sensorInit;
    }

    // Add values in the document

    auto found_targets = -1;
    if (jsonRequestDoc["update"].is<bool>() && jsonRequestDoc["update"].as<bool>() == true)
    {
        unsigned long startTime = 0;
        bool didInit = false;

        if (globalState.sensorOn)
        {
            if (globalState.sensorInit == false)
            {
                digitalWrite(globalState.xBJT_PIN, HIGH);
                delay(100);

                startTime = millis();
                Serial.printf("Sensor INIT (1) AT %lu ms\n", millis() - startTime);
                Serial2.begin(256000, SERIAL_8N1, ld2450_01_RX, ld2450_01_TX); // TX=17, RX=16

                Serial.printf("Sensor INIT (2) AT %lu ms\n", millis() - startTime);

                ld2450_01.begin(Serial2); // Pass Serial2 to the library

                Serial.printf("Sensor INIT (3) AT %lu ms\n", millis() - startTime);

                ld2450_01.waitForSensorMessage(false);
                Serial.printf("Sensor INIT (4) AT %lu ms\n", millis() - startTime);

                Serial.printf("Sensor INIT (5) AT %lu ms\n", millis() - startTime);

                // ld2450_01.waitForSensorMessage(false);
                Serial.printf("Sensor INIT (6a) AT %lu ms\n", millis() - startTime);

                auto maxReadTries = 50;
                auto readTries = 0;
                while (found_targets == -1 && readTries < maxReadTries)
                {
                    readTries++;
                    found_targets = ld2450_01.read();
                    if (found_targets == -1)
                    {
                        delay(100);
                        Serial.printf("Sensor INIT (delay) AT readTries:%i AT %lu ms\n", readTries, millis() - startTime);
                    }
                    else
                    {
                        Serial.printf("Sensor INIT (break) AT readTries:%i AT %lu ms\n", readTries, millis() - startTime);
                        break;
                    }
                }

                Serial.printf("Sensor INIT (6b) found_targets:%d, readTries:%i AT %lu ms\n", found_targets, readTries, millis() - startTime);

                globalState.sensorInit = true;
                didInit = true;
            }
            else
            {
                startTime = millis();
            }
        }

        if (found_targets > 0 || (found_targets = ld2450_01.read()) > 0)
        {
            unsigned long duration = millis() - startTime;
            Serial.printf("Sensor read took %lu ms\n", duration);
            // const int found_targets = ld2450_01.read();
            // if (found_targets > 0)
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
#endif
void handleJsonPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{

#ifdef DXSENSOR_NODE
    handleJsonPost_Node(request, data, len, index, total);
#endif
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
            button:disabled {
                background-color: #cccccc;
                cursor: not-allowed;
                opacity: 0.6;
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
                let $btn = $(this);
                $btn.prop('disabled', true);
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
                        $btn.prop('disabled', false);
                        console.log('Request completed');
                    }
                });
            });
            $('#sensorOff').click(function() {
                let $btn = $(this);
                $btn.prop('disabled', true);
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
                        $btn.prop('disabled', false);
                        console.log('Request completed');
                    }
                });
            });

            $('#update').click(function() {
                let $btn = $(this);
                $btn.prop('disabled', true);
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
                        $btn.prop('disabled', false);
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

struct_message lastNowMessage;

bool sendEspNodeUpdate(Node *node)
{
    return sendEspNodeUpdate(node, nullptr);
}

bool sendEspNodeUpdate(Node *node, Node *nodeWithToken)
{

    uint8_t dest[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    uint8_t macAddr[6];
    auto xx = WiFi.macAddress(macAddr);
    // 4C:11:AE:73:F1:4C

    memcpy(dest, broadcastAddress, 6);

    memcpy(lastNowMessage.update_MacAddress, node->macAddress, 6);
    lastNowMessage.update_status = node->status;

    if (nodeWithToken != nullptr)
    {
        memcpy(lastNowMessage.tokenHolder_MacAddress, nodeWithToken->macAddress, 6);
    }
    else
    {
        // just "clear" with broadcast
        memcpy(lastNowMessage.tokenHolder_MacAddress, broadcastAddress, 6);
    }

    strcpy(lastNowMessage.action, ("from:" + WiFi.localIP().toString()).c_str());

    // Serial.printf("Sending [%s]\n", lastNowMessage.action);

    esp_err_t result = esp_now_send(dest, (uint8_t *)&lastNowMessage, sizeof(lastNowMessage));
    // esp_err_t result = esp_now_send(ownMac, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK)
    {
        // Serial.printf("Sent %s with success\n", lastNowMessage.action);
        return true;
    }
    else
    {
        Serial.printf("Error sending [%s]\n", lastNowMessage.action);
        Serial.println(esp_err_to_name(result));
        return false;
    }
}

bool sendEspNowAction(const char *action)
{

    uint8_t dest[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    uint8_t macAddr[6];
    auto xx = WiFi.macAddress(macAddr);
    // 4C:11:AE:73:F1:4C

    memcpy(dest, broadcastAddress, 6);
    // getPeer(dest);

    // Serial.print("Sending to: ");
    // for (int i = 0; i < 6; i++)
    // {
    //     Serial.printf("%02X", dest[i]);
    //     if (i < 5)
    //         Serial.print(":"); // Add colon between bytes
    // }

    // Serial.println("");

    strcpy(lastNowMessage.action, ("from:" + WiFi.localIP().toString()).c_str());

    // Serial.printf("Sending [%s]\n", lastNowMessage.action);

    esp_err_t result = esp_now_send(dest, (uint8_t *)&lastNowMessage, sizeof(lastNowMessage));
    // esp_err_t result = esp_now_send(ownMac, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK)
    {
        // Serial.printf("Sent %s with success\n", lastNowMessage.action);
        return true;
    }
    else
    {
        Serial.printf("Error sending [%s]\n", lastNowMessage.action);
        Serial.println(esp_err_to_name(result));
        return false;
    }
}