#ifndef MY_DisplayUpdater
#define MY_DisplayUpdater

#include "MyIoTHelper.h"
#include "TempRecorder.h"

struct DisplayParameters;

class DisplayUpdater
{
public:
    DisplayUpdater(MyIoTHelper *helper, TempRecorder *tempRecorder);
    //~TempHelper(); // Destructor

    // OneWire *oneWire;
    // DallasTemperature *sensors;
    MyIoTHelper *ioTHelper;
    TempRecorder *tempRecorder;
    void begin();

    Arduino_GFX *gfx;

    portMUX_TYPE screenLock = portMUX_INITIALIZER_UNLOCKED;

    SemaphoreHandle_t mutex;

    static void updateDisplay(void *parameter);

    // static void _renderClickIcon(void *parameter);
    // void renderClickIcon(bool showRenderClickIcon);

    void showIcon(DisplayParameters *displayParameters);
    void hideIcon(DisplayParameters *displayParameters);
    void flashIcon(DisplayParameters *displayParameters);

    // bool _showRenderClickIcon = false;

private:
};

struct DisplayParameters
{
    int flashDuration;
    int flashInterval;
    bool flashLeaveOn;
    const unsigned char *icon; // Assuming 'icon' points to an array of bytes representing the icon data
    int16_t x;
    int16_t y;
    DisplayUpdater *displayUpdater;
    TaskHandle_t taskHandle;
};
#endif