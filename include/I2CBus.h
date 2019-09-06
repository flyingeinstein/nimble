
#pragma once

#include <Wire.h>

#include "NimbleAPI.h"

namespace Nimble {
namespace I2C {

class Bus : public ModuleSet
{
  public:
    Bus(short id);
    Bus(const Bus& copy) = delete;
    Bus& operator=(const Bus& copy) = delete;

    virtual const char* getDriverName() const;

    virtual void handleUpdate();

  public:
    TwoWire* wire;
};



class Device : public Module
{
  public:
    Device(short _id, short _address, short _slots);
    Device(short _id, SensorAddress _busId, short _address, short _slots);
    Device(const Device& copy);
    Device& operator=(const Device& copy);

    void setBus(SensorAddress _busId);

  protected:
    SensorAddress busId;  // i2c bus this device lives on expressed as a device:slot location
    short address;      // i2c bus address

    TwoWire* bus;  // cached i2c bus from busid
    TwoWire* getWire();
};

} // ns:I2C
} // ns:Nimble
