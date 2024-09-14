#ifndef MY_X
#define MY_X
#include <Arduino.h>

class A;
class B;
class X
{
public:
    X();
    void setup(A *a, B *b);

    void hello(bool callChild);

private:
    A *a;
    B *b;
};

#endif