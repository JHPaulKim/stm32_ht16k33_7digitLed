/*
 * ht16k33.c
 *
 *  Created on: 2021. 3. 17.
 *      Author: ST
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#include "HT16K33.h"
#include "i2c.h"


#define HT16K33_ADDRESS 0x70

// Commands
#define HT16K33_ON              0x21  // 0=off 1=on
#define HT16K33_STANDBY         0x20  // bit xxxxxxx0


// bit pattern 1000 0xxy
// y    =  display on / off
// xx   =  00=off     01=2Hz     10=1Hz     11=0.5Hz
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80
#define HT16K33_BLINKON0_5HZ    0x87
#define HT16K33_BLINKON1HZ      0x85
#define HT16K33_BLINKON2HZ      0x83
#define HT16K33_BLINKOFF        0x81

//#define HT16K33_DIM 0xE0|0x00 // Set dim from 0x00 (1/16th duty ccycle) to 0x0F (16/16 duty cycle)
#define HT16K33_DIM 0xE0|0x00

// bit pattern 1110 xxxx
// xxxx    =  0000 .. 1111 (0 - F)
#define HT16K33_BRIGHTNESS      0xE0

  uint8_t _addr;
  uint8_t _displayCache[5];                 // for performance
  bool    _cache = true;
  uint8_t _digits = 0;
  uint8_t _bright = 0x0F;

//
//  HEX codes 7 segment
//
//      01
//  20      02
//      40
//  10      04
//      08
//
static const uint8_t charmap[] = {  // TODO PROGMEM ?

  0x3F,   // 0
  0x06,   // 1
  0x5B,   // 2
  0x4F,   // 3
  0x66,   // 4
  0x6D,   // 5
  0x7D,   // 6
  0x07,   // 7
  0x7F,   // 8
  0x6F,   // 9
  0x77,   // A
  0x7C,   // B
  0x39,   // C
  0x5E,   // D
  0x79,   // E
  0x71,   // F
  0x00,   // space
  0x40,   // minus
};


void HT16K33_Init(void)
{
	  displayOn();
	  displayClear();
	  setDigits(1);
	  clearCache();
	  brightness(8);
}

void blinkon(void)
{
	  writeCmd(HT16K33_BLINKON0_5HZ);
}

void reset()
{
  displayOn();
  displayClear();
  setDigits(1);
  clearCache();
  brightness(8);
}

void clearCache()
{
  for (uint8_t i = 0; i < 5; i++)
  {
    _displayCache[i] = HT16K33_NONE;
  }
}

void displayOn()
{
  writeCmd(HT16K33_ON);
  writeCmd(HT16K33_DISPLAYON);
  brightness(_bright);
}


void displayOff()
{
  writeCmd(HT16K33_DISPLAYOFF);
  writeCmd(HT16K33_STANDBY);
}


void refresh()
{
  _refresh();
}


void blink(uint8_t val)
{
  if (val > 0x03) val = 0x00;
  writeCmd(HT16K33_BLINKOFF | (val << 1) );
}


void brightness(uint8_t val)
{
  if (val == _bright) return;
  _bright = val;
  if (_bright > 0x0F) _bright = 0x0F;
  writeCmd(HT16K33_BRIGHTNESS | _bright);
}


void setDigits(uint8_t val)
{
  _digits = val > 4 ? 4 : val;
}


void suppressLeadingZeroPlaces(uint8_t val)
{
  _digits = val > 4 ? 0 : 4 - val;
}


//////////////////////////////////////////
//
// display functions
//
void displayClear()
{
	uint16_t buffer[8];
	uint8_t i;
	for (i=0; i<8; i++)
	{
		buffer[i] = HT16K33_SPACE;
	}
	uint8_t buffer_t[17];
	buffer_t[0] = 0x00;
	for(int i = 0; i < 8; i++) {
	 	  buffer_t[2*i+1] = buffer[i] >> 8;
	   	  buffer_t[2*i+2] = buffer[i] & 0xFF;
	}
	uint8_t addr = HT16K33_ADDRESS << 1;
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c3, addr, buffer_t, 17,1000);
	if(status != HAL_OK) {
	        //Error_Handler(__FILE__, __LINE__);
	    }
  /*
  uint8_t x[4] = {HT16K33_SPACE, HT16K33_SPACE, HT16K33_SPACE, HT16K33_SPACE};
  display(x);
  displayColon(false);
  */
}

// DIV10 & DIV100 optimize?
bool displayInt(int n)
{
  bool inRange = ((-1000 < n) && (n < 10000));
  uint8_t x[4], h, l;
  bool neg = (n < 0);
  if (neg) n = -n;
  h = n / 100;
  l = n - h * 100;
  x[0] = h / 10;
  x[1] = h - x[0] * 10;
  x[2] = l / 10;
  x[3] = l - x[2] * 10;

  if (neg)
  {
    if (_digits >= 3)
    {
      x[0] = HT16K33_MINUS;
    }
    else
    {
      int i = 0;
      for (i = 0; i < (4 - _digits); i++)
      {
        if (x[i] != 0) break;
        x[i] = HT16K33_SPACE;
      }
      x[i-1] = HT16K33_MINUS;
    }
  }
  display(x);
  return inRange;
}


// 0000..FFFF
bool displayHex(uint16_t n)
{
  uint8_t x[4], h, l;
  h = n >> 8;
  l = n & 0xFF;
  x[3] = l & 0x0F;
  x[2] = l >> 4;
  x[1] = h & 0x0F;;
  x[0] = h >> 4;
  display(x);
  return true;
}


// 00.00 .. 99.99
bool displayDate(uint8_t left, uint8_t right)
{
  bool inRange = ((left < 100) && (right < 100));
  uint8_t x[4];
  x[0] = left / 10;
  x[1] = left - x[0] * 10;
  x[2] = right / 10;
  x[3] = right - x[2] * 10;
  display2(x, 1);
  displayColon(false);
  return inRange;
}


// 00:00 .. 99:99
bool displayTime(uint8_t left, uint8_t right, bool colon)
{
  bool inRange = ((left < 100) && (right < 100));
  uint8_t x[4];
  x[0] = left / 10;
  x[1] = left - x[0] * 10;
  x[2] = right / 10;
  x[3] = right - x[2] * 10;
  display(x);
  displayColon(colon);
  return inRange;
}


// seconds / minutes max 6039 == 99:99
bool displaySeconds(uint16_t seconds, bool colon)
{
  uint8_t left = seconds / 60;
  uint8_t right = seconds - left * 60;
  return displayTime(left, right, colon);
}


bool displayFloat(float f, uint8_t decimals)
{
  bool inRange = ((-999.5 < f) && (f < 9999.5));

  bool neg = (f < 0);
  if (neg) f = -f;

  if (decimals == 2) f = round(f * 100) * 0.01;
  if (decimals == 1) f = round(f * 10) * 0.1;
  if (decimals == 0) f = round(f);

  int whole = f;
  int point = 3;
  if (whole < 1000) point = 2;
  if (whole < 100) point = 1;
  if (whole < 10) point = 0;

  if (f >= 1)
  {
    while (f < 1000) f *= 10;
    whole = round(f);
  }
  else
  {
    whole = round(f * 1000);
  }

  uint8_t x[4], h, l;
  h = whole / 100;
  l = whole - h * 100;
  x[0] = h / 10;
  x[1] = h - x[0] * 10;
  x[2] = l / 10;
  x[3] = l - x[2] * 10;
  if (neg) // corrections for neg => all shift one position
  {
    x[3] = x[2];
    x[2] = x[1];
    x[1] = x[0];
    x[0] = HT16K33_MINUS;
    point++;
  }
  // add leading spaces
  while (point + decimals < 3)
  {
    x[3] = x[2];
    x[2] = x[1];
    x[1] = x[0];
    x[0] = HT16K33_SPACE;
    point++;
  }

  display2(x, point);

  return inRange;
}


/////////////////////////////////////////////////////////////////////
//
// EXPERIMENTAL
//
bool displayFixedPoint0(float f)
{
  bool inRange = ((-999.5 < f) && (f < 9999.5));
  displayFloat(f, 0);
  return inRange;
}

bool displayFixedPoint1(float f)
{
  bool inRange = ((-99.5 < f) && (f < 999.95));
  displayFloat(f, 1);
  return inRange;
}

bool displayFixedPoint2(float f)
{
  bool inRange = ((-9.95 < f) && (f < 99.995));
  displayFloat(f, 2);
  return inRange;
}

bool displayFixedPoint3(float f)
{
  bool inRange = ((0 < f) && (f < 9.9995));
  displayFloat(f, 3);
  return inRange;
}

/////////////////////////////////////////////////////////////////////

void displayTest(void)
{
  for (int k = 0; k < 18; k++)
  {
		uint8_t buf_tx[10];

		buf_tx[0]=0x00;
		buf_tx[1]=charmap[k];
		buf_tx[1]|= 0x80;
		buf_tx[2]=0x01;
		buf_tx[3]=charmap[k];
		buf_tx[3]|= 0x80;
		buf_tx[4]=0x00;
		buf_tx[5]=k%3;//colon
		buf_tx[6]=0x00;
		buf_tx[7]=charmap[k];//3
		buf_tx[7]|= 0x80;
		buf_tx[8]=0x00;
		buf_tx[9]=charmap[k];//4
		buf_tx[9]|= 0x80;

		writePosAllDigit(buf_tx);

    HAL_Delay(100);
  }
}


void displayRaw(uint8_t *arr, bool colon)
{
  writePos(0, arr[0]);
  writePos(1, arr[1]);
  writePos(3, arr[2]);
  writePos(4, arr[3]);
  writePos(2, colon ? 255 : 0);
}


bool displayVULeft(uint8_t val)
{
  bool inRange = (val < 9); // can display 0..8  bars
  uint8_t ar[4];
  for (int idx = 3; idx >=0; idx--)
  {
    if (val >= 2)
    {
      ar[idx] = 0x36;       //   ||
      val -= 2;
    }
    else if (val == 1)
    {
      ar[idx] = 0x06;        //   _|
      val = 0;
    }
    else ar[idx] = 0x00;     //   __
  }
  displayRaw(ar, true);
  return inRange;
}


bool displayVURight(uint8_t val)
{
  bool inRange = (val < 9);
  uint8_t ar[4];
  for (uint8_t idx = 0; idx < 4; idx++)
  {
    if (val >= 2)
    {
      ar[idx] = 0x36;       //   ||
      val -= 2;
    }
    else if (val == 1)
    {
      ar[idx] = 0x30;        //   |_
      val = 0;
    }
    else ar[idx] = 0x00;     //   __
  }
  displayRaw(ar, true);
  return inRange;
}


void display(uint8_t *arr)
{
  for (uint8_t i = 0; i < (4 - _digits); i++)
  {
    if (arr[i] != 0) break;
    arr[i] = HT16K33_SPACE;
  }
  writePos(0, charmap[arr[0]]);
  writePos(1, charmap[arr[1]]);
  writePos(3, charmap[arr[2]]);
  writePos(4, charmap[arr[3]]);
  writePosAllDigit(ht16k33_Buffer);
 // debug to Serial
  // dumpSerial(arr, 0);
}

void display2(uint8_t *arr, uint8_t pnt)
{
  // debug to Serial
  // dumpSerial(arr, pnt);

  writePos2(0, charmap[arr[0]], pnt == 0);
  writePos2(1, charmap[arr[1]], pnt == 1);
  writePos2(3, charmap[arr[2]], pnt == 2);
  writePos2(4, charmap[arr[3]], pnt == 3);
  writePosAllDigit(ht16k33_Buffer);
}



void displayColon(uint8_t on)
{
  writePos(2, on ? 2 : 0);
}

void writeCmd(uint8_t cmd)
{
	uint8_t buf_tx[10];
	buf_tx[0]=cmd;
	uint8_t addr = HT16K33_ADDRESS << 1;
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c3,addr, buf_tx,10,1000);
	if(status != HAL_OK) {
	        //Error_Handler(__FILE__, __LINE__);
	    }
}


void writePos(uint8_t pos, uint8_t mask)
{
  if (_cache && (_displayCache[pos] == mask)) return;
  ht16k33_Buffer[pos * 2 + 1] = mask;
  _displayCache[pos] = _cache ? mask : HT16K33_NONE;
}

void writePosAllDigit(uint8_t buf_tx[])
{
	buf_tx[0] = 0x00;
	uint8_t addr = HT16K33_ADDRESS << 1;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c3, addr, buf_tx, 17,1000);
	if(status != HAL_OK) {
	        //Error_Handler(__FILE__, __LINE__);
	    }
}

void writePos2(uint8_t pos, uint8_t mask, bool pnt)
{
  if (pnt) mask |= 0x80;
  // if (_overflow) mask |= 0x80;
  else mask &= 0x7F;
  writePos(pos, mask);
}
