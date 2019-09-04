
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

class Module;
class ModuleSet;
class SensorReading;


/// @todo Create influx module
typedef struct _InfluxTarget {
    String database;
    String measurement;
} InfluxTarget;

/// @todo I think SensorInfo can be deprecated now that modules are more generic, possibly just need Module generic config stuff (and maybe keep as json)
typedef struct _SensorInfo {
    String channelName;
    String driver;
    // influx target?
    uint8_t pin;  //todo: should be pinmap
    unsigned long updateFrequency;
} SensorInfo;


typedef Module* (*ModuleFactory)(SensorInfo* info);

typedef struct _ModuleInfo {
    const char* name;
    const char* category;
    ModuleFactory factory;
} ModuleInfo;


