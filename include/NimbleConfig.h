
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

class Device;
class Devices;
class SensorReading;

typedef enum SensorType {
  Invalid,
  ChildDevice,   // slot is a child-device (such as a device on a bus)
  Numeric,       // general numeric value
  Timestamp,     // a unix timestamp
  Milliseconds,  // a measurement in milliseconds
  Humidity,      // Relative Humidity (RH)
  Hygrometer,
  Temperature,   // Celcius or Farenheit
  HeatIndex,     // Celcius or Farenheit (adjusted with RH)
  Illuminance,   // Lux
  pH,
  ORP,           // unitless
  DissolvedOxygen, // mg/L
  Conductivity,  // microS/cm (Atlas Scientific)
  CO2,           // ppm
  Pressure,      // KPa?
  Flow,          // liquid or gas flow
  Altitude,      // meters or feet
  AirPressure,
  AirQuality,
  Voltage,
  Current,
  Watts,
  Motion,
  FirstSensorType=Humidity,   // types before this are considered generic such as configuration values
  LastSensorType=Motion
} SensorType;

const char* SensorTypeName(SensorType st);


typedef enum  {
  Offline,
  Degraded,
  Nominal
} DeviceState;

const char* DeviceStateName(DeviceState st);


typedef struct _InfluxTarget {
  String database;
  String measurement;
} InfluxTarget;


typedef struct _SensorInfo {
  String channelName;
  String driver;
  // influx target?
  uint8_t pin;  //todo: should be pinmap
  unsigned long updateFrequency;
} SensorInfo;


typedef Device* (*DriverFactory)(SensorInfo* info);

typedef struct _DeviceDriverInfo {
  const char* name;
  const char* category;
  DriverFactory factory;
} DeviceDriverInfo;
