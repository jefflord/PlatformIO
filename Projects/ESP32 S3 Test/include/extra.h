#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

String getLatestVersion()
{
    WiFiClient client;
    String response = "";

    if (client.connect("10.0.0.64", 80))
    {
        client.println("GET /IoT2/version.txt HTTP/1.1");
        client.println("Host: 10.0.0.64");
        client.println("Connection: close");
        client.println();

        // Read the HTTP response
        bool isHeader = true;
        while (client.connected() || client.available())
        {
            if (client.available())
            {
                String line = client.readStringUntil('\n');
                if (line == "\r" || line == "\n")
                {
                    if (isHeader)
                    {
                        isHeader = false; // End of headers
                    }
                    else
                    {
                        break; // End of body
                    }
                }
                else if (!isHeader)
                {
                    response += line;
                }
            }
        }
        client.stop();
    }
    else
    {
        Serial.println("Failed to connect");
        return "NA";
    }

    response.trim(); // Remove any leading or trailing whitespace
    return response;
}

HTTPUpdateResult performUpdate()
{
    HTTPUpdate httpUpdate;
    WiFiClient client;

    t_httpUpdate_return ret = httpUpdate.update(client, "http://10.0.0.64/IoT2/firmware.bin");

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("Update Failed. Error: %s\n", httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("No Update Available.");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("Update Successful. Rebooting...");
        ESP.restart();
        break;
    }

    return ret;
}