
#include "Display.h"

//#include <string.h>
#include <ctype.h>

#define OLED_RESET LED_BUILTIN        // as per https://maker.pro/arduino/projects/oled-i2c-display-arduinonodemcu-tutorial


const char* ParseExceptionCodeToString(ParseExceptionCode code) {
  switch(code) {
    case InvalidRegister: return "invalid register"; break;
    case ExpectedRegister: return "expected register"; break;
    case ExpectedNumeric: return "expected number"; break;
    default: return "syntax error"; break;
  }
}

Display::Display()
	: display(OLED_RESET), fonts(NULL), nfonts(0),
	  G(0), D(0), S(0), _F(0), X(0), Y(0), U(0), P(1), R(0), T(0), C(0), W(0), H(0),
	  w(0), str(NULL), gx(6), gy(9)
{
}

void Display::begin(Devices& _devices)
{
  devices = &_devices;
  
  #if (LCD == SSD1306)
  // Initiate the LCD and disply the Splash Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(255); // Where arg is a value from 0 to 255 (sets contrast e.g. brightness)
  display.display();
  display.clearDisplay();
#endif
}

void Display::reset()
{
  G=D=S=_F=X=Y=W=H=U=R=T=C=w=0;
  P=1;
  str = NULL;
  strLength=-1;
  gx = 6; gy = 9;
  display.clearDisplay();
  display.setCursor(0,0);
  display.setFont(NULL);
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

short& Display::getRegister(char reg)
{
	switch(toupper(reg)) {
		case 'G': return G;
		case 'D': return D;
		case 'S': return S;
		case 'F': return _F;
		case 'X': return X;
		case 'Y': return Y;
		case 'W': return W;
		case 'H': return H;
		case 'U': return U;
    case 'P': return P;
		case 'R': return R;
    case 'T': return T;
		case 'C': return C;
		default: return w;
	}
}

void Display::print(SensorReading r) {
  switch(r.valueType) {
    case VT_NULL: display.print("--"); break;
    case VT_CLEAR: break;
    case VT_INVALID: display.print("**"); break;
    case VT_FLOAT: display.print(r.f, P); break;
    case VT_INT: display.print(r.l); break;
    case VT_BOOL: 
      if(r.b)
        display.fillCircle(X, Y-4, 4, WHITE);
      else
        display.drawCircle(X, Y-4, 4, WHITE);
      break;
  }
}

void Display::print(const char* str, short strLength) {
  while(strLength--)
    display.print(*str++);
}

bool Display::exec() 
{
  SensorReading r;
  
	switch(G) {
    case 0: // move to X,Y or R,C (does nothing since the XYRC regs already set the move)
      break;
    case 1: // draw string at R,C (text row/column)
      print( str, strLength );
      break;
		case 2: // draw reading to X,Y
      print( devices->getReading(D, S) );
		  break;
    case 4: // draw line
      break;
    case 5: // draw rect
    case 6: // fill rect
      break;
    case 7: // draw circle
    case 8: // fill circle
      break;
    case 50: // set grid size
      gx = W;
      gy = H;
      break;
		default:
		  Serial.print("? G");
		  Serial.println(G);
		  return false;
	}
	return true;
}

bool Display::execute(const char* input, ParseException* _pex)
{
	// we are ready for command processing
	char c;
	char reg = 0;
	short* preg;
	short line_reg_count=0; // the number of registers set on this line
	bool negative;
	short i = 0;
	ParseException pex;

	memset(&pex, 0, sizeof(ParseException));

  reset();

	while ((c = *input++))
	{
    if(c == ' ') {
      continue;
	  } else if(c == '\'') {
			// a string argument, read until end of line
			str = input;

			// now read until end of line, then let the processor handle the \n
			while(*input && *input!='\n')
				input++;
      strLength = input - str;
			continue;
		} else if(c =='\n') {
			if(line_reg_count>0)
				exec();
			pex.line++;
			line_reg_count=0;
			continue;
		} else if(c =='\r')
			continue;	// ignore CR 

		pex.position++;
		negative = false;
		i = 0;

		// expect register name
		if(!isalpha(c)) {
			pex.code = ExpectedRegister;
			goto syntax_error;
		}
		
		// resolve the register
		preg = &getRegister(reg = c);
		if(preg == &w) {
			pex.code = InvalidRegister;
			goto syntax_error;
		}
	
		// check if a negative value
		if(*input =='-') {
			negative=true;
			input++;
			pex.position++;
		}
		
		// read a value
		while(isdigit(*input)) {
			i = i*10 + (*input++ - '0');
			pex.position++;
		}
    if(negative) i *= -1;

		// todo: check for floats

		// set the register
		*preg = i;
    switch(reg) {
      case 'R':
        display.setCursor(display.getCursorX(), Y=R*gy);
        break;
      case 'C':
        display.setCursor(X=C*gx,display.getCursorY());
        break;
      case 'X':
        if(G<80)
          display.setCursor(X,display.getCursorY());
        break;
      case 'Y':
        if(G<80)
          display.setCursor(display.getCursorX(),Y);
        break;
      case 'T':
        display.setTextSize(T);
        break;
      case 'F':
        display.setFont( 
          (fonts!=NULL && _F > 0 && _F <= nfonts) 
            ? fonts[_F-1].font
            : NULL
        );
        break;
    }

		// done parsing a register
		line_reg_count++;
	}

	if(line_reg_count>0) {
		// must execute the last line
		exec();
	}

syntax_error:
  // can be error or not, we set PEX so caller gets line and position
	if(_pex) *_pex = pex;
  display.display();
	return pex.code==0;
}



/***** OLD GARBAGE CODE ******/


#define LCD_COL1  45
#define LCD_COL2  85

//unsigned long long lastMotion = 0;

#if 0
void OLED_Display()
{
  int y = 16;
  unsigned long alone = millis() - lastMotion;

  if(alone > 30000) {
    display.clearDisplay();
    display.display();
    return;
  }
  display.dim( alone > 15000 );

  //display.ssd1306_command(SSD1306_SETCONTRAST);
  //display.ssd1306_command(255); // Where arg is a value from 0 to 255 (sets contrast e.g. brightness)

  #if 0
  // Update OLED values
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setFont(&TITLE_FONT);
  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(data.temperature[0], 1);

  //display.setTextSize(1);
  display.setFont(NULL);
  display.drawCircle(display.getCursorX()+3, 8, 1, WHITE);
  display.setCursor(display.getCursorX()+1, 8);
  display.print(" F");

  display.setFont(&FreeSans9pt7b);
  //display.setTextSize(1);
  y += FreeSans9pt7b.yAdvance; //LINE_HEIGHT;
  display.setCursor(0, y);
  display.print(hostname);
#endif

  // display motion devices
  int x=120;
  SensorReading r;
  Devices::ReadingIterator itr = DeviceManager.forEach(Motion);
  while( r = itr.next() ) {
    if(r.b)
      display.fillCircle(x, y-4, 4, WHITE);
    else
      display.drawCircle(x, y-4, 4, WHITE);
    x -= 8;
  }
  
  display.setFont(NULL);

#if 0
#if 0
  //display.setTextSize(1);
  y = 64 - LINE_HEIGHT;
  display.setCursor(0, y);
  display.print("HI");
  display.setCursor(LCD_COL1, y);
  display.print(data.heatIndex, 1);
  display.print("\xB0C");
  
  y -= LINE_HEIGHT;
  display.setCursor(0, y);
  display.print("RH");
  display.setCursor(LCD_COL1, y);
  display.print(data.humidity, 1);
  display.print("%");
#else
  y = 0;
  display.setCursor(LCD_COL2, y);
  display.print("HI");
  display.setCursor(LCD_COL2+15, y);
  display.print(data.heatIndex, 1);
  
  y += LINE_HEIGHT;
  display.setCursor(LCD_COL2, y);
  display.print("RH");
  display.setCursor(LCD_COL2+15, y);
  display.print(data.humidity, 1);
  //display.print("%");  
#endif
#endif

  display.display();
}
#endif
