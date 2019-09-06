/**
 * @file AtlasScientific.h
 * @author Colin F. MacKenzie (nospam2@colinmackenzie.net)
 * @brief Reads pH, ORP, Oxygen or other sensors from Atlas Scientific.
 * @version 0.1
 * @date 2019-08-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once


#include "I2CBus.h"

namespace AtlasScientific {
  /// @brief Result of last sensor reading 
  typedef enum {
    Success,
    Failed,
    Pending,
    NoData
  } EzoProbeResult;

  /// @brief Atlas Scientific pH, ORP, Dissolved Oxygen or Conductivity probe as a device
  class EzoProbe : public Nimble::I2C::Device
  {
    public:
      /**
       * @brief Construct a new Ezo Probe device
       * 
       * @param id The device id.
       * @param ptype Type of Ezo probe, typically pH, ORP, DissolvedOxygen or Conductivity.
       * @param address I2C address of Ezo probe.
       */
      EzoProbe(short id, Nimble::SensorType ptype, short address=0);

      /**
       * @brief Get the Driver Name
       * 
       * @return The string "AtlasScientific-EZO"
       */
      virtual const char* getDriverName() const;

      /**
       * @brief Called by the framework to take another measurement of the Ezo probe
       * 
       */
      virtual void handleUpdate();

      /// @brief we will wait this long between successive measurements
      unsigned long measurementTime;

    protected:
      Nimble::SensorType sensorType;
      char ph_data[20];

      EzoProbeResult readResponse();
      void sendCommand(const char* cmd);
  
  };

}
