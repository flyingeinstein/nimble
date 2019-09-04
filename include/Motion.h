
#pragma once

#include "DigitalPin.h"

class MotionIR : public Nimble::DigitalPin
{
  public:
    MotionIR(short id, int pin=0);
    virtual const char* getDriverName() const;
};
