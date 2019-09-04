
#pragma once


#include "NimbleAPI.h"

class DigitalPin : public Module
{
  public:
    DigitalPin(short id, SensorType _pinType, int _pin, unsigned long _updateInterval=1000, bool _reversePolarity=false);
    DigitalPin(const DigitalPin& copy);
    DigitalPin& operator=(const DigitalPin& copy);

    virtual const char* getDriverName() const;

    virtual void handleUpdate();

  public:
    SensorType pinType;
    int pin;
    bool reversePolarity;
};
