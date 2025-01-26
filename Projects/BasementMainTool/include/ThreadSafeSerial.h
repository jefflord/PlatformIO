#ifndef THREAD_SAFE_SERIAL_H // Header guard
#define THREAD_SAFE_SERIAL_H

#include <Arduino.h>
#include <mutex>

class ThreadSafeSerial
{
public:
    int (&pinModes)[40];

    ThreadSafeSerial(int (&modes)[40]) : pinModes(modes) {}

    bool useBorkMode = false;

    void print(const String &message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print("try_lock_for failed:");
            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void print(const int message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }
            Serial.print("try_lock_for failed:");
            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void print(const char *message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }

            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print("try_lock_for failed:");
            Serial.print(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void println(const char *message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }

            serialMutex.unlock();
        }
        else
        {
            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print("try_lock_for failed:");
            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void println(const String &message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print("try_lock_for failed:");
            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void println(long long message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }

            Serial.print("try_lock_for failed:");
            Serial.println(message);

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

    void printf(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        char buffer[256]; // Adjust size as needed
        int len = vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            if (len > 0)
            {

                if (useBorkMode)
                {
                    pinMode(1, INPUT_PULLUP);
                    Serial.begin(115200);
                }

                Serial.write(buffer, len); // Or Serial.print(buffer)

                if (useBorkMode)
                {
                    delay(10);
                    Serial.end();
                    pinMode(1, pinModes[1]);
                }
            }
            serialMutex.unlock();
        }
        else
        {

            if (useBorkMode)
            {
                pinMode(1, INPUT_PULLUP);
                Serial.begin(115200);
            }
            Serial.print("try_lock_for failed:");
            Serial.write(buffer, len); // Or Serial.print(buffer)

            if (useBorkMode)
            {
                delay(10);
                Serial.end();
                pinMode(1, pinModes[1]);
            }
        }
    }

private:
    std::timed_mutex serialMutex;
};

extern ThreadSafeSerial safeSerial;

#endif