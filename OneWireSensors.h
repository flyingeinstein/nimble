
#pragma once


#include "DeviceManager.h"

// Dallas 1wire temperature sensor resolution
#define SENSOR_RESOLUTION 9

#include <OneWire.h>
#include <DallasTemperature.h>

class OneWireSensor : public Device
{
  public:
    OneWireSensor(short id, int _pin);
    OneWireSensor(const OneWireSensor& copy);
    OneWireSensor& operator=(const OneWireSensor& copy);

    virtual void handleUpdate();

  public:
    int pin;
    OneWire oneWire;
    DallasTemperature DS18B20;
};
