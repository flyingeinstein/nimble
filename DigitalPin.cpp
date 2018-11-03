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
  bool v = digitalRead(pin) ? true : false;
  (*this)[0] = SensorReading(pinType, reversePolarity ? !v : v);
  state = Nominal;
}
