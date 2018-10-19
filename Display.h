
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

#define FONT(name)  { &name, #name }

typedef struct _FontInfo {
  const GFXfont* font;
  const char* name;
} FontInfo;


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
    const FontInfo* fonts;
    short nfonts;
    
  public:
  	Display();

    void begin(Devices& _devices);

  	template<size_t N>
  	void begin(Devices& _devices, const FontInfo (&_fonts)[N]) { 
  	  begin(_devices); 
  	  fonts = _fonts; 
  	  nfonts = (short)N; 
  	}
   
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

    // reset parser and display getting ready for a new page draw
    void reset();

    // execute based on the value of the registers
    bool exec();

    void print(const char* str, short strLength);
    void print(SensorReading r);
    void setCursorRC(short r, short c);
};
