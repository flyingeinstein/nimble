
#pragma once


#include "I2CBus.h"

namespace AtlasScientific {

    typedef enum {
    Success,
    Failed,
    Pending,
    NoData
  } ProbeResult;

  
  class Probe : public I2CDevice
  {
    public:
      Probe(short id, SensorType ptype, short address=0);

      void begin();
      virtual void handleUpdate();

      unsigned long measurementTime;

    protected:
      SensorType sensorType;
      char ph_data[20];

      ProbeResult readResponse();
      void sendCommand(const char* cmd);
  
  };

}
