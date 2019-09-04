
#pragma once


#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif
#include <WString.h>
#include <assert.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <mDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#else
#pragma error(This SDK requires an ESP8266 or ESP32)
#endif

// the Restfully library is available in the board manager or at https://github.com/flyingeinstein/Restfully
#include <Restfully.h>


#define MAX_SLOTS     256

namespace Nimble {

class Module;
class ModuleSet;
class SensorReading;

} // ns:Nimble


