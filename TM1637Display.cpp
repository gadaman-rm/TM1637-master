
//  Author: avishorp@gmail.com
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

extern "C" {
  #include <stdlib.h>
  #include <string.h>
  #include <inttypes.h>
}

#include <TM1637Display.h>
#include <Arduino.h>

#define TM1637_I2C_COMM1    0x40 //01
#define TM1637_I2C_COMM2    0xC0 //11
#define TM1637_I2C_COMM3    0x80 //10




//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
const uint8_t digitToSegment[] = {
 // XGFEDCBA
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,    	// 0
  SEG_B|SEG_C,    						  	// 1
  SEG_A|SEG_B|SEG_G|SEG_E|SEG_D,    	  	// 2
  SEG_A|SEG_B|SEG_G|SEG_C|SEG_D,    	  	// 3
  SEG_F|SEG_G|SEG_B|SEG_C,    			  	// 4
  SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,          	// 5
  SEG_G|SEG_C|SEG_D|SEG_E|SEG_F|SEG_A,   	// 6
  SEG_A|SEG_B|SEG_C,   				      	// 7
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,// 8
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,      // 9
  SEG_A|SEG_B|SEG_G|SEG_F|SEG_C|SEG_E,      // A
  SEG_F|SEG_G|SEG_C|SEG_D|SEG_E,    		// b
  SEG_A|SEG_F|SEG_E|SEG_D, 				    // C
  SEG_B|SEG_C|SEG_D|SEG_E|SEG_G,  		    // d
  SEG_A|SEG_F|SEG_G|SEG_E|SEG_D, 		    // E
  SEG_A|SEG_F|SEG_G|SEG_E   			    // F
  };


TM1637Display::TM1637Display(uint8_t pinClk, uint8_t pinDIO)
{
	// Copy the pin numbers
	m_pinClk = pinClk;
	m_pinDIO = pinDIO;

	// Set the pin direction and default value.
	// Both pins are set as inputs, allowing the pull-up resistors to pull them up
    pinMode(m_pinClk, INPUT);
    pinMode(m_pinDIO,INPUT);
	digitalWrite(m_pinClk, LOW);
	digitalWrite(m_pinDIO, LOW);
}

void TM1637Display::setBrightness(uint8_t brightness, bool on)
{
	m_brightness = (brightness & 0x7) | (on? 0x08 : 0x00);
}

void TM1637Display::setSegments(const uint8_t segments[], uint8_t length, uint8_t pos)
{
    // Write COMM1
	start();
	writeByte(TM1637_I2C_COMM1);
	stop();

	// Write COMM2 + first digit address
	start();
	writeByte(TM1637_I2C_COMM2 + (pos & 0x03));

	// Write the data bytes
	for (uint8_t k=0; k < length; k++)
	 if(k==0) 
		writeByte(segments[3]);
	else if(k==1)
		writeByte(segments[2]);
	else if(k==2)
		writeByte(segments[1]);
	else if(k==3)
		writeByte(segments[0]);
	else if(k==4)
		writeByte(0x00);
	else if(k==5)
		writeByte(segments[4]);
	
	
	stop();

	// Write COMM3 + brightness
	start();
	writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
	stop();
}

void TM1637Display::showNumberDec(int num, bool leading_zero, uint8_t length, uint8_t pos)
{
  showNumberDecEx(num, 0, leading_zero, length, pos);
}

void TM1637Display::showNumberDecEx(int num, uint8_t dots, bool leading_zero,
                                    uint8_t length, uint8_t pos)
{
  uint8_t digits[4];
	const static int divisors[] = { 1, 10, 100, 1000 };
	bool leading = true;

	for(int8_t k = 0; k < 4; k++) {
	    int divisor = divisors[4 - 1 - k];
		int d = num / divisor;
    uint8_t digit = 0;

		if (d == 0) {
		  if (leading_zero || !leading || (k == 3))
		      digit = encodeDigit(d);
	      else
		      digit = 0;
		}
		else {
			digit = encodeDigit(d);
			num -= d * divisor;
			leading = false;
		}
    
    // Add the decimal point/colon to the digit
    digit |= (dots & 0x80); 
    dots <<= 1;
    
    digits[k] = digit;
	}

	setSegments(digits + (4 - length), length, pos);
}


void TM1637Display::bitDelay()
{
	delayMicroseconds(100);
}

void TM1637Display::start()
{
  pinMode(m_pinDIO, OUTPUT);
  bitDelay();
}

void TM1637Display::stop()
{
	pinMode(m_pinDIO, OUTPUT);
	bitDelay();
	pinMode(m_pinClk, INPUT);
	bitDelay();
	pinMode(m_pinDIO, INPUT);
	bitDelay();
}

bool TM1637Display::writeByte(uint8_t b)
{
  uint8_t data = b;

  // 8 Data Bits
  for(uint8_t i = 0; i < 8; i++) {
    // CLK low
    pinMode(m_pinClk, OUTPUT);
    bitDelay();

	// Set data bit
    if (data & 0x01)
      pinMode(m_pinDIO, INPUT);
    else
      pinMode(m_pinDIO, OUTPUT);

    bitDelay();

	// CLK high
    pinMode(m_pinClk, INPUT);
    bitDelay();
    data = data >> 1;
  }

  // Wait for acknowledge
  // CLK to zero
  pinMode(m_pinClk, OUTPUT);
  pinMode(m_pinDIO, INPUT);
  bitDelay();

  // CLK to high
  pinMode(m_pinClk, INPUT);
  bitDelay();
  uint8_t ack = digitalRead(m_pinDIO);
  if (ack == 0)
    pinMode(m_pinDIO, OUTPUT);


  bitDelay();
  pinMode(m_pinClk, OUTPUT);
  bitDelay();

  return ack;
}

uint8_t TM1637Display::encodeDigit(uint8_t digit)
{
	return digitToSegment[digit & 0x0f];
}
