#include "ButtonServerHelper.h"
#include "MyIoTHelper.h"
#include "DisplayUpdater.h"

ButtonServerHelper *ButtonServerHelper::global = nullptr;

ButtonServerHelper::ButtonServerHelper()
{
    ButtonServerHelper::global = this;
}

ButtonServerHelper *ButtonServerHelper::GetButtonServerHelper()
{
    return ButtonServerHelper::global;
}

void ButtonServerHelper::begin(MyIoTHelper *iotHelper, DisplayUpdater *displayUpdater)
{
    myServo.attach(SERVO_PIN);
    this->iotHelper = iotHelper;
    this->displayUpdater = displayUpdater;

    myServo.write(iotHelper->servoHomeAngle);

    touchAttachInterrupt(TOUCH_PIN, ButtonServerHelper::pushServoButton, 50);
}

void ButtonServerHelper::updateLoop()
{
    if (millis() > lastTouch + 500)
    {
        // over 500ms since last time we saw touch, mark as OK to go AND pretend we just pressed it by setting lastPress = now.
        okToGo = true;
        lastPress = millis();
    }
}

void ButtonServerHelper::pushServoButtonX(void *pvParameters)
{

    ButtonServerHelper *buttonServerHelper = static_cast<ButtonServerHelper *>(pvParameters);
    MyIoTHelper *iotHelper = buttonServerHelper->iotHelper;

    safeSerial.println("pushServoButtonX");
    buttonServerHelper->servoMoving = true;

    buttonServerHelper->actuationCounter++;

    pinMode(LED_ONBOARD, OUTPUT);

    DisplayParameters params = {-1, 250, false, epd_bitmap_icons8_natural_user_interface_2_13, 0, 0, NULL};
    buttonServerHelper->displayUpdater->flashIcon(&params);

    // safeSerial.println("pushServoButtonX B");

    digitalWrite(LED_ONBOARD, HIGH);
    buttonServerHelper->myServo.write(iotHelper->servoAngle);
    vTaskDelay(pdMS_TO_TICKS(iotHelper->pressDownHoldTime));
    buttonServerHelper->myServo.write(iotHelper->servoHomeAngle);
    digitalWrite(LED_ONBOARD, LOW);

    // safeSerial.println("pushServoButtonX C");

    buttonServerHelper->displayUpdater->hideIcon(&params);

    buttonServerHelper->servoMoving = false;

    // safeSerial.println("pushServoButtonX D");

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    safeSerial.printf("Task 'pushServoButtonX' high-water mark: %u bytes\n", uxHighWaterMark);

    vTaskDelete(NULL);
}

void ButtonServerHelper::pushServoButton()
{

    auto buttonServerHelper = ButtonServerHelper::GetButtonServerHelper();

    // record each time we are touched
    buttonServerHelper->lastTouch = millis();

    if (millis() > buttonServerHelper->lastPress + 3000)
    {
        // reset this each time
        buttonServerHelper->lastPress = millis();

        // we only want to do this one per, so we use okToGo
        if (buttonServerHelper->okToGo)
        {
            safeSerial.printf("okToGo 1");
            // buttonServerHelper->iotHelper->chaos("wifi");

            buttonServerHelper->iotHelper->configHasBeenDownloaded = false;
            buttonServerHelper->iotHelper->updateConfig();
            safeSerial.println("okToGo 4");
            buttonServerHelper->okToGo = false;
        }
    }

    if (!buttonServerHelper->servoMoving)
    {
        xTaskCreate(pushServoButtonX, "pushServoButtonX", 512 * 4, buttonServerHelper, 1, NULL);
    }
    else
    {
        // safeSerial.println("servoMoving already true");
    }
}
