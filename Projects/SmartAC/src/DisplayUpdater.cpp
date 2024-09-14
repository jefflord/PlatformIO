#include "MyIoTHelper.h"
#include "DisplayUpdater.h"

void _showIcon(void *parameter)
{
    DisplayParameters *dp = static_cast<DisplayParameters *>(parameter);
    DisplayUpdater *me = dp->displayUpdater;
    auto gfx = me->gfx;

    // me->_showRenderClickIcon = true;

    if (dp->flashInterval == 0)
    {
        dp->flashInterval = 500;
    }

    auto stopTime = millis() + dp->flashDuration;

    while (dp->flashDuration <= 0 || (dp->flashDuration > 0 && millis() < stopTime))
    {

        if (dp->flashLeaveOn)
        {
            xSemaphoreTake(me->mutex, portMAX_DELAY);
            gfx->fillRect(dp->x, dp->y, 13, 13, BLACK);
            xSemaphoreGive(me->mutex);
        }
        else
        {
            xSemaphoreTake(me->mutex, portMAX_DELAY);
            gfx->fillRect(dp->x, dp->y, 13, 13, BLACK);
            gfx->drawXBitmap(dp->x, dp->y, dp->icon, 13, 13, WHITE);
            xSemaphoreGive(me->mutex);
        }
        vTaskDelay(dp->flashInterval / portTICK_PERIOD_MS);

        if (dp->flashLeaveOn)
        {
            xSemaphoreTake(me->mutex, portMAX_DELAY);
            gfx->fillRect(dp->x, dp->y, 13, 13, BLACK);
            gfx->drawXBitmap(dp->x, dp->y, dp->icon, 13, 13, WHITE);
            xSemaphoreGive(me->mutex);
        }
        else
        {
            xSemaphoreTake(me->mutex, portMAX_DELAY);
            gfx->fillRect(dp->x, dp->y, 13, 13, BLACK);
            xSemaphoreGive(me->mutex);
        }
        vTaskDelay(dp->flashInterval / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void DisplayUpdater::hideIcon(DisplayParameters *displayParameters)
{
    displayParameters->displayUpdater = this;

    if (displayParameters->taskHandle != NULL)
    {
        // stop the flashing task
        vTaskDelete(displayParameters->taskHandle);
        displayParameters->taskHandle = NULL;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    gfx->fillRect(displayParameters->x, displayParameters->y, 13, 13, BLACK);
    xSemaphoreGive(mutex);
}

void DisplayUpdater::flashIcon(DisplayParameters *displayParameters)
{
    displayParameters->displayUpdater = this;
    
    // try to stop if flashing...
    if (displayParameters->taskHandle != NULL)
    {
        // stop the flashing task
        vTaskDelete(displayParameters->taskHandle);
        displayParameters->taskHandle = NULL;
    }

    xTaskCreate(_showIcon, "showIcon", 2048 * 4, displayParameters, 1, &displayParameters->taskHandle);

    if (displayParameters->flashDuration > 0)
    {
        // don't return this since it will stop itself.
        displayParameters->taskHandle = NULL;
    }
}

void DisplayUpdater::showIcon(DisplayParameters *displayParameters)
{
    displayParameters->displayUpdater = this;

    TaskHandle_t taskHandle = NULL; // Initialize the task handle
                                    //_showRenderClickIcon = false;

    if (displayParameters->taskHandle != NULL)
    {
        // stop the flashing task
        vTaskDelete(displayParameters->taskHandle);
        displayParameters->taskHandle = NULL;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    gfx->fillRect(displayParameters->x, displayParameters->y, 13, 13, BLACK);
    // safeSerial.println("gfx->drawXBitmap1 BBBBBB");
    gfx->drawXBitmap(displayParameters->x, displayParameters->y, displayParameters->icon, 13, 13, WHITE);
    // safeSerial.println("gfx->drawXBitmap2 BBBBB");
    xSemaphoreGive(mutex);
}

void DisplayUpdater::updateDisplay(void *parameter)
{
    DisplayUpdater *me = static_cast<DisplayUpdater *>(parameter);

    /**/
    auto animationShowing = false;
    auto forceUpdate = false;
    const int SET_CUR_TOP_Y = 16 - 16;
    const int FONT_SIZE = 2;
    /**/
    auto gfx = me->gfx;

    gfx->begin();

    gfx->setTextSize(FONT_SIZE);
    gfx->fillScreen(BLACK);

    long loopDelayMs = 1000;
    // long lastRun = 0;

    double lastT1 = 0;
    double lastT2 = 0;
    double lastT3 = 0;

    auto lastUpdateTimeMillis = millis();
    for (;;)
    {

        auto startTime = millis();
        if (animationShowing)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            forceUpdate = true;
            continue;
        }

        // sprintf(timeString, "%02d:%02d:%02d %s", hours, minutes, seconds, ampm.c_str());
        // sprintf(timeString, "%4.1f\u00B0C", temperature_1_C);

        // temperature_0_C = round(temperature_0_C * 10) / 10.0;
        // temperature_1_C = round(temperature_1_C * 10) / 10.0;
        // temperature_2_C = round(temperature_2_C * 10) / 10.0;

        auto temperature_0_C = me->tempRecorder->temperatureC[0];
        auto temperature_1_C = me->tempRecorder->temperatureC[1];
        auto temperature_2_C = me->tempRecorder->temperatureC[2];

        temperature_0_C = round(temperature_0_C);
        temperature_1_C = round(temperature_1_C);
        temperature_2_C = round(temperature_2_C);

        auto temperatureF1 = (temperature_0_C * (9.0 / 5.0)) + 32;
        auto temperatureF2 = (temperature_1_C * (9.0 / 5.0)) + 32;
        auto temperatureF3 = (temperature_2_C * (9.0 / 5.0)) + 32;

        auto timeSinceLast = millis() - lastUpdateTimeMillis;
        if (timeSinceLast > 5000 || forceUpdate || lastT1 != temperatureF1 || lastT2 != temperatureF2 || lastT3 != temperatureF3)
        {
            // safeSerial.println("Display update...");

            lastUpdateTimeMillis = millis();
            forceUpdate = false;
            lastT1 = temperatureF1;
            lastT2 = temperatureF2;
            lastT3 = temperatureF3;

            xSemaphoreTake(me->mutex, portMAX_DELAY);

            gfx->setTextColor(WHITE);

            gfx->fillRect(0, SET_CUR_TOP_Y + 16, 96, 64, BLACK);
            // char randomChar = (char)random(97, 127);
            gfx->setCursor(0, SET_CUR_TOP_Y + 16);

            // if (uploadingData)
            // {
            //   renderUploadIcon();
            // }

            // safeSerial.println("getFormattedTime() start");
            gfx->println(me->ioTHelper->getFormattedTime());
            // safeSerial.println("getFormattedTime() back");

            gfx->setCursor(gfx->getCursorX(), gfx->getCursorY() + 6);

            char bufferForNumber[20];

            gfx->setTextColor(BLUE);
            sprintf(bufferForNumber, "%2.0f", temperatureF1);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF1));
            gfx->setTextSize(FONT_SIZE);

            auto y = gfx->getCursorY();
            gfx->setCursor(gfx->getCursorX() + 12, y);
            gfx->setTextColor(RED);
            sprintf(bufferForNumber, "%2.0f", temperatureF2);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF2));
            gfx->setTextSize(FONT_SIZE);

            gfx->setCursor(gfx->getCursorX() + 12, y);
            gfx->setTextColor(GREEN);
            sprintf(bufferForNumber, "%2.0f", temperatureF3);
            gfx->print(bufferForNumber);
            // gfx->setTextSize(FONT_SIZE - 1);
            // gfx->print(getDecimalPart(temperatureF3));
            gfx->setTextSize(FONT_SIZE);

            xSemaphoreGive(me->mutex);
        }

        vTaskDelay(loopDelayMs - (millis() - startTime) / portTICK_PERIOD_MS);
    }
}

void DisplayUpdater::begin()
{
    Arduino_DataBus *bus = new Arduino_HWSPI(OLED_DC, OLED_CS, OLED_SCL, OLED_SDA);
    gfx = new Arduino_SSD1331(bus, OLED_RES);

    xTaskCreate(
        updateDisplay,   // Function to run on the new thread
        "updateDisplay", // Name of the task (for debugging)
        8192 * 2,        // Stack size (in bytes) // 8192
        this,            // Parameter passed to the task
        1,               // Priority (0-24, higher number means higher priority)
        NULL             // Handle to the task (not used here)
    );
}

DisplayUpdater::DisplayUpdater(MyIoTHelper *_helper, TempRecorder *_tempRecorder)
{
    mutex = xSemaphoreCreateMutex();
    ioTHelper = _helper;
    tempRecorder = _tempRecorder;
}
