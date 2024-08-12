#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

const char *connectToWiFi(Adafruit_SSD1306 &display, int flashLight);

void handleBackgroundTask(Adafruit_SSD1306 &display);

void backgroundTask(void *pvParameters);

void showVersion(void *pvParameters);

class ProgressBarFPS
{
public:
    // ProgressBarFPS();

    static void drawProgressBar(Adafruit_SSD1306 &display, int x, int y, int progress, bool clearAndDisplay);
    void doProgressBarFPS(Adafruit_SSD1306 &display);

private:
    int _loopCounter = 0;
    bool _up = true;
};
