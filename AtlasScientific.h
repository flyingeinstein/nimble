
#pragma once


#include "I2CBus.h"

namespace AtlasScientific {

    typedef enum {
    Success,
    Failed,
    Pending,
    NoData
  } ProbeResult;

  
  class pHProbe : public I2CDevice
  {
    public:
      pHProbe(short id, SensorAddress busId, short address=99);
     
      virtual void handleUpdate();

      unsigned long measurementTime;

    protected:
      char ph_data[20];

      ProbeResult readResponse();
      void sendCommand(const char* cmd);
  
  };

}
