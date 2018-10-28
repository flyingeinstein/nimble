
#pragma once

#include <Wire.h>

#include "DeviceManager.h"


class I2CBus : public Device
{
  public:
    I2CBus(short id);
    I2CBus(const I2CBus& copy);
    I2CBus& operator=(const I2CBus& copy);

    virtual void handleUpdate();

  public:
    TwoWire* wire;
};



class I2CDevice : public Device
{
  public:
    I2CDevice(short _id, short _address, short _slots);
    I2CDevice(short _id, SensorAddress _busId, short _address, short _slots);
    I2CDevice(const I2CDevice& copy);
    I2CDevice& operator=(const I2CDevice& copy);

  protected:
    SensorAddress busId;  // i2c bus this device lives on expressed as a device:slot location
    short address;      // i2c bus address

    TwoWire* bus;  // cached i2c bus from busid
    TwoWire* getWire();
};
