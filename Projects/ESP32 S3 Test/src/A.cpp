#include "A.h"
#include "B.h"

A::A() {}

void A::setup(B *b)
{
    this->b = b;
}

void A::hello(bool callChild)
{
    Serial.println("Hello from A");
    if (callChild)
    {
        b->hello(false);
    }
}