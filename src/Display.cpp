
#include "Display.h"
#include "ModuleManager.h"

#include <ctype.h>
#include <FS.h>   // Include the SPIFFS library

#define OLED_RESET LED_BUILTIN        // as per https://maker.pro/arduino/projects/oled-i2c-display-arduinonodemcu-tutorial

using namespace Nimble;


const char* ParseExceptionCodeToString(ParseExceptionCode code) {
  switch(code) {
    case InvalidRegister: return "invalid register"; break;
    case ExpectedRegister: return "expected register"; break;
    case ExpectedNumeric: return "expected number"; break;
    default: return "syntax error"; break;
  }
}

Display::Display(short id)
	: Module(id, 0, 500, MF_DISPLAY), display(OLED_RESET), fonts(NULL), nfonts(0), pages(NULL), npages(6), activePage(0),
	  G(0), D(0), S(0), _F(0), X(0), Y(0), U(0), P(1), R(0), T(0), C(0), W(0), H(0),
	  w(0), str(NULL), gx(6), gy(9), relativeCoords(false)
{
  pages = (DisplayPage*)calloc(npages, sizeof(DisplayPage));
}

Display::~Display()
{
  ::free(pages);
}

const char* Display::getDriverName() const
{
  return "display";
}

void Display::begin()
{
  #if (LCD == SSD1306)
  // Initiate the LCD and disply the Splash Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(255); // Where arg is a value from 0 to 255 (sets contrast e.g. brightness)
  display.display();
  display.clearDisplay();
#endif

  onHttp("fonts", HTTP_GET, std::bind(&Display::httpPageGetFonts, this));
  onHttp("page/active", HTTP_POST, std::bind(&Display::httpPageSetActivePage, this));
  onHttp("page/code", HTTP_GET, std::bind(&Display::httpPageGetCode, this));
  onHttp("page/code", HTTP_POST, std::bind(&Display::httpPageSetCode, this));

  loadAllPagesFromFS();
}

void Display::reset()
{
  G=D=S=_F=X=Y=W=H=U=R=T=C=w=0;
  P=1;
  str = NULL;
  strLength=-1;
  gx = 6; gy = 9;
  relativeCoords = false;
  display.clearDisplay();
  display.setCursor(0,0);
  display.setFont(NULL);
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

short Display::addPage(const DisplayPage& page)
{
  for(int i=0; i<npages; i++) {
    if(!pages[i].isValid()) {
      // use this page slot
      pages[i] = page;
      return i;
    }
  }
  return -1;
}

short Display::loadPageFromFS(short page_number)
{
  if(page_number<0 || page_number>=npages)
    return -1;
    
  String fname("/display/page/");
  fname += page_number;
   
  File f = SPIFFS.open(fname, "r");
  if(f) {
    String contents = f.readString();
    pages[ page_number ] = DisplayPage(contents);
    f.close();
  }
  return page_number;
}

short Display::savePageToFS(short page_number)
{
  if(page_number<0 || page_number>=npages)
    return -1;
    
  String fname("/display/page/");
  fname += page_number;

  const DisplayPage& page = pages[page_number];
  if(page.isValid()) {
    File f = SPIFFS.open(fname, "w");
    if(f) {
      f.print(page.code());
      f.close();
    }
    Serial.print("saved page file ");
    Serial.println(fname);
  } else {
    // delete the file
    SPIFFS.remove(fname);
    Serial.print("removed page file ");
    Serial.println(fname);
  }
  return page_number;
}

short Display::loadAllPagesFromFS()
{
  short loaded=0;
  Dir dir = SPIFFS.openDir("/display/page");
  while (dir.next()) {
    const char* id = strrchr(dir.fileName().c_str(), '/');
    if(id && isdigit(*++id)) {
      short n = atoi(id);
      if(loadPageFromFS(n) >=0)
        loaded++;
    }
  }
  if(loaded==0)
    Serial.println("no display pages found in SPIFFS://display/page");
  return loaded;
}

void Display::handleUpdate()
{
  if(activePage>=0 && activePage<npages && pages[activePage].isValid()) {
    ParseException pex;
    execute(pages[activePage].code(), &pex);
    state = Nominal;
  } else
    state = Offline;
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
        display.fillCircle(display.getCursorX(), display.getCursorY()-4, 4, WHITE);
      else
        display.drawCircle(display.getCursorX(), display.getCursorY()-4, 4, WHITE);
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
      if(owner!=NULL)
        print( owner->getReading(D, S) );
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
      case 'G': // some G codes are instant
          switch(i) {
            case 90: relativeCoords = false; break;
            case 91: relativeCoords = true; break;
          }
        break;
      case 'R':
        if(G<80) {
          if(relativeCoords)
            display.setCursor(display.getCursorX(), (display.getCursorY()/gy)*gy + R*gy );
          else
            display.setCursor(display.getCursorX(), R*gy);
        }
        break;
      case 'C':
        if(G<80) {
          if(relativeCoords)
            display.setCursor((display.getCursorX()/gx)*gx + C*gx, display.getCursorY());
          else
            display.setCursor(C*gx, display.getCursorY());
        }
        break;
      case 'X':
        if(G<80) {
          if(relativeCoords)
            display.setCursor(display.getCursorX() + X,display.getCursorY());
          else
            display.setCursor(X,display.getCursorY());
        }
        break;
      case 'Y':
        if(G<80) {
          if(relativeCoords)
            display.setCursor(display.getCursorX(),display.getCursorY() + Y);
          else
            display.setCursor(display.getCursorX(),Y);
        }
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

void Display::httpPageGetFonts() {
  String s('[');
  for(short i=0; i < nfonts; i++) {
      if(i>0) s+=',';
      s += '"';
      s += fonts[i].name;
      s += '"';
  }
  s += ']';
  http().send(200, "application/json", s);
}

void Display::httpPageSetActivePage() {
  WebServer& server = http();
  String pageN = server.arg("n");
  int n = pageN.toInt();
  if(n>=0 && n < npages) {
    activePage = n;
    server.send(200, "text/plain", String("{ page: ")+n+" }");
  } else
    server.send(400, "text/pain", "invalid page");
}



void Display::httpPageGetCode() {
  WebServer& server = http();
  String pageN = server.arg("n");
  int n = pageN.toInt();
  if(n>=0 && n < npages) {
    const DisplayPage& page = pages[n];
    server.send(200, "text/plain", page.code());
  } else
    server.send(400, "text/pain", "invalid page");
}

void Display::httpPageSetCode() {
  WebServer& server = http();
  String pageN = server.arg("n");
  int n = pageN.toInt();
  String fs = server.arg("fs");
  String code = server.arg("plain");
  if(n>=0 && n < npages) {
    DisplayPage& page = pages[n];
    page = DisplayPage( code.c_str() );
    if(fs!="false" && page.isValid())
      savePageToFS( n );
    server.send(200, "text/plain", page.code());
  } else
    server.send(400, "text/pain", "invalid page");
}




DisplayPage::DisplayPage()
  : _code(NULL), owns_mem(false)
{
}

DisplayPage::DisplayPage(String p)
  : _code(strdup(p.c_str())), owns_mem(true)
{
}

DisplayPage::DisplayPage(const char* code, bool copy_mem)
  : _code(copy_mem ? strdup(code) : code), owns_mem(copy_mem)
{
}

DisplayPage::DisplayPage(const DisplayPage& copy)
  : _code(copy._code), owns_mem(copy.owns_mem)
{
  // we must copy the memory if our source owns the memory as it may free it
  if(owns_mem) {
    _code = strdup(copy._code);
  }
}

DisplayPage::~DisplayPage()
{
  clear();
}

void DisplayPage::clear()
{
  if(owns_mem && _code) {
    ::free((void*)_code);
  }
}

DisplayPage& DisplayPage::operator=(const DisplayPage& copy)
{
  clear();  // ensure we dont already own memory
  _code = copy._code;
  owns_mem = copy.owns_mem;
  if(owns_mem) {
    _code = strdup(copy._code);
  }
  return *this;
}
