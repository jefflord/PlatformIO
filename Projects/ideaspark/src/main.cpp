#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C


u8g2(U8G2_R0, /*clock =*/22, /*data =*/21, U8X8_PIN_NONE);

#define TESTIMAGE2_32X16_HEIGHT 16
#define TESTIMAGE2_32X16_WIDTH 32

// array size is 64
static const unsigned char testimage2_32x1[] PROGMEM = {
    B11111111, B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000,

    B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000,
    B11111111, B00000000, B11111111, B00000000, B11111111
    // B11111111, B11111111, B01111111, B11111100,
    // B11111111, B11011111, B00111111, B11111001,
    // B11111111, B10011111, B10011111, B11110011,
    // B11111111, B00011111, B11000111, B11100111,
    // B11111110, B01011111, B11110111, B11001111,
    // B11111110, B11001111, B11111001, B10011111,
    // B11111100, B11101111, B11111100, B00111111,
    // B11111001, B11101111, B11111110, B01111111,
    // B11111011, B11101111, B11111110, B01111111,
    // B11110000, B00000111, B11111100, B10111111,
    // B11100111, B11110111, B11111001, B10011111,
    // B11101111, B11110111, B11110011, B11001111,
    // B11001111, B11110011, B11100111, B11100111,
    // B10011111, B11111001, B11001111, B11110011,
    // B10111111, B11111101, B11011111, B11111001,
    // B10111111, B11111111, B11111111, B11111111
};
void setup()
{
  Serial.begin(230400);

  while (!Serial)
    ;

  u8g2.begin();

  u8g2.setFont(u8g2_font_7x14_tr);
}

int counter = 0;
void loop()
{

  char numberString[10];
  snprintf(numberString, sizeof(numberString), "%d", counter++);

  u8g2.clearBuffer();

  // Convert the number to a string

  // u8g2.drawStr(0, 10, numberString);

  u8g2.drawBitmap(0, 0, 128, 1, testimage2_32x1);

  unsigned long startTime = micros(); // Get the start time in microseconds
  u8g2.sendBuffer();

  unsigned long endTime = micros();                // Get the end time in microseconds
  unsigned long elapsedTime = endTime - startTime; // Calculate elapsed time

  Serial.print("took: ");
  Serial.print(elapsedTime / 1000);
  Serial.println(" ms");

  delay(1000);
}
