#ifndef MY_A
#define MY_A
#include <Arduino.h>

class B;
class A
{
public:
    A();
    void setup(B *b);

    void hello(bool callChild);

private:
    B *b;
};



#endif