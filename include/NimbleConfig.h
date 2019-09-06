
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

/**
 * @brief Defines the maximum number of slots in a module and modules in a set.
 * Setting this to 32 is a good balance for small 32bit devices. Most devices will request exactly the number of
 * slots they require so this number is usually just an upper limit. Since a ModuleSet is also a module and uses
 * the same slot collection to store sub-modules this number is the default number of entries for a ModuleSet so
 * we want to keep it somewhat low.
 */
#define MAX_SLOTS     32

namespace Nimble {

class Module;
class ModuleSet;
class SensorReading;

using WebServer = ESP8266WebServer;
using Endpoints = typename RestRequestHandler::Endpoints;
//typedef Esp8266RestRequestHandler RestRequestHandler;
//typedef Esp8266RestRequest RestRequest;  // RestRequestHandler::RequestType RestRequest;
//typedef typename RestRequestHandler::HandlerType HandlerType;

} // ns:Nimble


