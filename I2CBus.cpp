#include "I2CBus.h"

#include <Wire.h>

I2CBus::I2CBus(short id)
  : Device(id, 1, 60000, DF_I2C_BUS), wire(&Wire)
{
}

I2CBus::I2CBus(const I2CBus& copy)
  : Device(copy), wire(copy.wire)
{
}

I2CBus& I2CBus::operator=(const I2CBus& copy)
{
  Device::operator=(copy);
  wire = copy.wire;
  return *this;
}

void I2CBus::handleUpdate()
{
  state = Nominal;
}




I2CDevice::I2CDevice(short id, SensorAddress _busId, short _address, short _slots)
  : Device(id, _slots, 1000), bus(_busId), address(_address)
{
}

I2CDevice::I2CDevice(const I2CDevice& copy)
  : Device(copy), address(copy.address)
{
}

I2CDevice& I2CDevice::operator=(const I2CDevice& copy)
{
  Device::operator=(copy);
  address = copy.address;
  return *this;
}

TwoWire* I2CDevice::getWire()
{
  if(owner) {
    I2CBus& i2c = (I2CBus&)owner->find(bus.device);
    if(i2c) {
      // todo: get the Nth i2c bus based on the slot, for now we only support 1 bus
      return i2c.wire;
    }
  }
  return NULL;
}
