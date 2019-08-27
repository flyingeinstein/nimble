
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

#include "NimbleAPI.h"

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


class DisplayPage
{
  public:
    DisplayPage();
    DisplayPage(const char* code, bool copy_mem=true);
    DisplayPage(String p);
    DisplayPage(const DisplayPage& copy);
    virtual ~DisplayPage();

    DisplayPage& operator=(const DisplayPage& copy);

    void clear();

    inline bool isValid() const { return _code!=NULL; }

    inline const char* code() const { return _code; }

  protected:
    const char* _code;
    bool owns_mem;
};

class Display : public Device
{
  public:
    Adafruit_SSD1306 display;
    const FontInfo* fonts;
    short nfonts;
    
  public:
  	Display(short id=1);
    virtual ~Display();

    virtual const char* getDriverName() const;

    virtual void begin();

  	template<size_t N>
  	void setFontTable(const FontInfo (&_fonts)[N]) { 
  	  fonts = _fonts; 
  	  nfonts = (short)N; 
  	}

    short addPage(const DisplayPage& page);

    short loadPageFromFS(short page_number);
    short savePageToFS(short page_number);
    
    short loadAllPagesFromFS();

    virtual void handleUpdate();
   
  	short& getRegister(char reg);
  
  	// parse a program
  	bool execute(const char* input, ParseException* pex=NULL);
  
	protected:
    DisplayPage* pages;
    short npages;
    short activePage;

		// list of gcode registers
		short G, D, S, _F, X, Y, U, P, R, T, C, W, H;

		// working var, also target of invalid register
		short w;

		// holds the most recent string argument
		const char* str;
    short strLength;

    // grid size
    short gx, gy;

    // coordinate mode
    bool relativeCoords;  // default: false

    // reset parser and display getting ready for a new page draw
    void reset();

    // execute based on the value of the registers
    bool exec();

    void print(const char* str, short strLength);
    void print(SensorReading r);
    void setCursorRC(short r, short c);

    // Rest interface
    void httpPageSetActivePage();
    void httpPageGetFonts();
    void httpPageGetCode();
    void httpPageSetCode();
};
