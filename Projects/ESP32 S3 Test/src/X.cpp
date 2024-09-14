#include "X.h"
#include "A.h"
#include "B.h"

X::X() {}

void X::setup(A *a, B *b)
{
    this->a = a;
    this->b = b;
}

void X::hello(bool callChild)
{
    Serial.println("Hello from X");
    if (callChild)
    {
        a->hello(false);
        b->hello(false);
    }
}