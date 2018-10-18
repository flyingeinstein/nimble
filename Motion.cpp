#include "Motion.h"

MotionIR::MotionIR(short id, int pin)
  : DigitalPin(id, Motion, pin, 250)
{
}
