#include "Motion.h"


DigitalPin::DigitalPin(short id, SensorType _pinType, int _pin=0, unsigned long _updateInterval, bool _reversePolarity)
  : Device(id, 1, _updateInterval), pinType(_pinType), pin(_pin), reversePolarity(_reversePolarity)
{
    pinMode(pin, INPUT);
}

DigitalPin::DigitalPin(const DigitalPin& copy)
  : Device(copy), pinType(copy.pinType), pin(copy.pin)
{
}

DigitalPin& DigitalPin::operator=(const DigitalPin& copy)
{
  Device::operator=(copy);
  pinType = copy.pinType;
  pin = copy.pin;
  return *this;
}

void DigitalPin::handleUpdate()
{
  readings[0] = SensorReading(pinType, digitalRead(pin) ? true : false);
  if(reversePolarity)
    readings[0].b != readings[0].b;
  state = Nominal;
}
