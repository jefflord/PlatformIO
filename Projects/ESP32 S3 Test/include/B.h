#ifndef MY_B
#define MY_B
#include <Arduino.h>

class A;
class B
{
public:
    B();
    void setup(A *a);
    void hello(bool callChild);

private:
    A *a;
};

#endif