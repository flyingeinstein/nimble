
#pragma once


#include "I2CBus.h"

namespace AtlasScientific {

    typedef enum {
    Success,
    Failed,
    Pending,
    NoData
  } EzoProbeResult;

  
  class EzoProbe : public I2CDevice
  {
    public:
      EzoProbe(short id, SensorType ptype, short address=0);

      virtual const char* getDriverName() const;

      virtual void handleUpdate();

      unsigned long measurementTime;

    protected:
      SensorType sensorType;
      char ph_data[20];

      EzoProbeResult readResponse();
      void sendCommand(const char* cmd);
  
  };

}
