#include "B.h"
#include "A.h"

B::B()
{
}

void B::setup(A *a)
{
    this->a = a;
}

void B::hello(bool callChild)
{
    Serial.println("Hello from B");
    if (callChild)
    {
        a->hello(false);
    }
}
