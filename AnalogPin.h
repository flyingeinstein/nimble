
#pragma once


#include "DeviceManager.h"

struct NoConversion {
  inline short operator()(short value) const { return value; }
};

template<class Converter=NoConversion>
class AnalogPin : public Device
{
  public:
    AnalogPin(short id, SensorType _pinType, int _pin=0)
      : Device(id, 1, 250), pinType(_pinType), pin(_pin)
    {
        pinMode(pin, INPUT);
    }
    
    AnalogPin(const AnalogPin& copy)
      : Device(copy), pinType(copy.pinType), pin(copy.pin)
    {
    }
    
    AnalogPin& operator=(const AnalogPin& copy)
    {
      Device::operator=(copy);
      pinType = copy.pinType;
      pin = copy.pin;
      return *this;
    }
    
    virtual void handleUpdate()
    {
      short v = analogRead(pin);
      decltype(converter()) v2 = converter(v);
      readings[0] = SensorReading(pinType, v2);
      state = Nominal;
    }

  protected:
    Converter converter;
    SensorType pinType;
    int pin;
};

struct MoistureConversion {
  inline float operator()(short value) const { return (1024 - value)/512.0; }
};

typedef AnalogPin<MoistureConversion> MoistureSensor;
