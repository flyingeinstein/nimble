
#pragma once


#include "DeviceManager.h"

class DigitalPin : public Device
{
  public:
    DigitalPin(short id, SensorType _pinType, int _pin, unsigned long _updateInterval=1000, bool _reversePolarity=false);
    DigitalPin(const DigitalPin& copy);
    DigitalPin& operator=(const DigitalPin& copy);

    virtual void handleUpdate();

  public:
    SensorType pinType;
    int pin;
    bool reversePolarity;
};
