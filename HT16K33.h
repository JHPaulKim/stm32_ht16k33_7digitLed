#pragma once
//
//    FILE: HT16K33.h
//  AUTHOR: Rob Tillaart
// VERSION: 0.3.2
//    DATE: 2019-02-07
// PURPOSE: Arduino Library for HT16K33 4x7segment display
//          http://www.adafruit.com/products/1002
//     URL: https://github.com/RobTillaart/HT16K33.git
//


#include <stdbool.h>


#define HT16K33_LIB_VERSION         (F("0.3.2"))


// Characters
#define HT16K33_0                0
#define HT16K33_1                1
#define HT16K33_2                2
#define HT16K33_3                3
#define HT16K33_4                4
#define HT16K33_5                5
#define HT16K33_6                6
#define HT16K33_7                7
#define HT16K33_8                8
#define HT16K33_9                9
#define HT16K33_A                10
#define HT16K33_B                11
#define HT16K33_C                12
#define HT16K33_D                13
#define HT16K33_E                14
#define HT16K33_F                15
#define HT16K33_SPACE            16
#define HT16K33_MINUS            17
#define HT16K33_NONE             99

void HT16K33_Init();

  bool begin();
  void reset();

  bool isConnected();

  // default _cache is true as it is ~3x faster but if one has noise
  // on the I2C and wants to force refresh one can disable caching
  // for one or more calls.
  void clearCache();
  void cacheOn();
  void cacheOff();
  void refresh(); // force writing of cache to display
  
  void displayOn();
  void displayOff();

  void brightness(uint8_t val);             // 0 .. 15
  void blink(uint8_t val);                  // 0 .. 3     0 = off
  void blinkon();


  // 0,1,2,3,4 digits - will replace suppressLeadingZeroPlaces
  void setDigits(uint8_t val);
  // 0 = off, 1,2,3,4 digits  space iso 0
  void suppressLeadingZeroPlaces(uint8_t val);    // will be obsolete

  void displayClear();
  bool displayInt(int n);                   // -999 .. 9999
  bool displayHex(uint16_t n);              // 0000 .. FFFF

  // Date could be {month.day} or {day.hour}           . as separator
  // Time could be hh:mm or mm:ss or ss:uu (hundreds   : as separator
  bool displayDate(uint8_t left, uint8_t right);    // 00.00 .. 99.99
  bool displayTime(uint8_t left, uint8_t right, bool colon);    // 00:00 .. 99:99
  bool displaySeconds(uint16_t seconds, bool colon);    // 00:00 .. 99:99

  bool displayFloat(float f, uint8_t decimals); // -999 .. 0.000 .. 9999

  void display(uint8_t *arr);               // array with 4 elements
  void display2(uint8_t *arr, uint8_t pt);   // pt = digit with . (0..3)
  void displayColon(uint8_t on);            // 0 = off
  void displayRaw(uint8_t *arr, bool colon);  // max control

  bool displayVULeft(uint8_t val);          // 0..8
  bool displayVURight(uint8_t val);         // 0..8


  // DEBUG
  void    displayTest();
  void    dumpSerial2(uint8_t *arr, uint8_t pnt);  // array as numbers
  void    dumpSerial();                           // display cache in HEX format
  uint8_t getAddr();


  // EXPERIMENTAL
  bool    getOverflow();
  void    clrOverflow();

  bool    displayFixedPoint0(float f);
  bool    displayFixedPoint1(float f);
  bool    displayFixedPoint2(float f);
  bool    displayFixedPoint3(float f);

  void    _refresh();
  void    writeCmd(uint8_t cmd);
  void    writePos(uint8_t pos, uint8_t mask);
  void    writePos2(uint8_t pos, uint8_t mask, bool pnt);
  void    writePosAllDigit(uint8_t buf_tx[]);

  uint8_t ht16k33_Buffer[10];


// -- END OF FILE --
