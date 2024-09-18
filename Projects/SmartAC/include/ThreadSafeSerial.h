#ifndef THREAD_SAFE_SERIAL_H // Header guard
#define THREAD_SAFE_SERIAL_H

#include <Arduino.h>
#include <mutex>

class ThreadSafeSerial
{
public:
    ThreadSafeSerial() {}

    void print(const String &message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.print(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.print(message);
        }
    }

    void print(const int message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.print(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.print(message);
        }
    }

    void print(const char *message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.print(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.print(message);
        }
    }

    void println(const char *message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.println(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.println(message);
        }
    }

    void println(const String &message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.println(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.println(message);
        }
    }

    void println(long long message)
    {
        if (serialMutex.try_lock_for(std::chrono::milliseconds(100)))
        {
            Serial.println(message);
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.println(message);
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
                Serial.write(buffer, len); // Or Serial.print(buffer)
            }
            serialMutex.unlock();
        }
        else
        {
            Serial.print("try_lock_for failed:");
            Serial.write(buffer, len); // Or Serial.print(buffer)
        }
    }

private:
    std::timed_mutex serialMutex;
};

extern ThreadSafeSerial safeSerial;

#endif