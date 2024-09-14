#ifndef THREAD_SAFE_SERIAL_H // Header guard
#define THREAD_SAFE_SERIAL_H

#include <Arduino.h>
#include <mutex>

class ThreadSafeSerial {
public:
    ThreadSafeSerial() {}

    void print(const String &message) {
        serialMutex.lock();
        Serial.print(message);
        serialMutex.unlock();
    }    

    void print(const char* message) {
        serialMutex.lock();
        Serial.print(message);
        serialMutex.unlock();
    }

    void println(const char* message) {
        serialMutex.lock();
        Serial.println(message);
        serialMutex.unlock();
    }

    void println(const String &message) {
        serialMutex.lock();
        Serial.println(message);
        serialMutex.unlock();
    }

    void println(long long message) {
        serialMutex.lock();
        Serial.println(message);
        serialMutex.unlock();
    }

    // Note: printf can be more complex to implement thread-safely due to its variable arguments. 
    // This implementation assumes a simple format string with a single argument.
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);

        serialMutex.lock();
        Serial.printf(format, args);
        serialMutex.unlock();

        va_end(args);
    }

private:
    std::mutex serialMutex;
};

extern ThreadSafeSerial safeSerial; 

#endif