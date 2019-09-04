
#pragma once

#include <Wire.h>

#include "NimbleAPI.h"

namespace Nimble {

class I2CBus : public Module
{
  public:
    I2CBus(short id);
    I2CBus(const I2CBus& copy);
    I2CBus& operator=(const I2CBus& copy);

    virtual const char* getDriverName() const;

    virtual void handleUpdate();

  public:
    TwoWire* wire;
};



class I2CDevice : public Module
{
  public:
    I2CDevice(short _id, short _address, short _slots);
    I2CDevice(short _id, SensorAddress _busId, short _address, short _slots);
    I2CDevice(const I2CDevice& copy);
    I2CDevice& operator=(const I2CDevice& copy);

    void setBus(SensorAddress _busId);

  protected:
    SensorAddress busId;  // i2c bus this device lives on expressed as a device:slot location
    short address;      // i2c bus address

    TwoWire* bus;  // cached i2c bus from busid
    TwoWire* getWire();
};

} // ns:Nimble
