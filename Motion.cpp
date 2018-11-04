#include "Motion.h"

MotionIR::MotionIR(short id, int pin)
  : DigitalPin(id, Motion, pin, 250)
{
}

const char* MotionIR::getDriverName() const
{
  return "motion";
}
