
#pragma once

#include "DigitalPin.h"

class MotionIR : public DigitalPin
{
  public:
    MotionIR(short id, int pin=0);
    virtual const char* getDriverName() const;
};
