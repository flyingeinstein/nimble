
#pragma once


#define SSD1306 1
#define LCD SSD1306

// Libraries OLED
#if (LCD == SSD1306)
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

#include "DeviceManager.h"

typedef enum {
	SyntaxError,
	InvalidRegister,
	ExpectedRegister,
	ExpectedNumeric
} ParseExceptionCode;

typedef struct _ParseException {
	short line;
	short position;
	ParseExceptionCode code;
} ParseException;

const char* ParseExceptionCodeToString(ParseExceptionCode code);


class Display
{
  public:
    Adafruit_SSD1306 display;
    Devices* devices;
    const GFXfont** fonts;
    
  public:
  	Display();
  
  	void begin(Devices& _devices, const GFXfont** _fonts=NULL);

    void clear();
    
  	short& getRegister(char reg);
  
  	// parse a program
  	bool execute(const char* input, ParseException* pex=NULL);
  
	protected:
		// list of gcode registers
		short G, D, S, _F, X, Y, U, P, R, T, C, W, H;

		// working var, also target of invalid register
		short w;

		// holds the most recent string argument
		const char* str;
    short strLength;

    // execute based on the value of the registers
    bool exec();

    void print(const char* str, short strLength);
    void print(SensorReading r);
    void setCursorRC(short r, short c);
};
