#include "I2CBus.h"

#include <Wire.h>

namespace Nimble {

I2CBus::I2CBus(short id)
  : Module(id, 1, 60000, MF_I2C_BUS), wire(&Wire)
{
}

I2CBus::I2CBus(const I2CBus& copy)
  : Module(copy), wire(copy.wire)
{
}

const char* I2CBus::getDriverName() const
{
  return "i2c-bus";
}

I2CBus& I2CBus::operator=(const I2CBus& copy)
{
  Module::operator=(copy);
  wire = copy.wire;
  return *this;
}

void I2CBus::handleUpdate()
{
  state = Nominal;
}



I2CDevice::I2CDevice(short id, short _address, short _slots)
  : Module(id, _slots, 1000), address(_address), bus(NULL)
{
}

I2CDevice::I2CDevice(short id, SensorAddress _busId, short _address, short _slots)
  : Module(id, _slots, 1000), busId(_busId), address(_address), bus(NULL)
{
}

I2CDevice::I2CDevice(const I2CDevice& copy)
  : Module(copy), busId(copy.busId), address(copy.address), bus(NULL)
{
}

I2CDevice& I2CDevice::operator=(const I2CDevice& copy)
{
  Module::operator=(copy);
  busId = copy.busId;
  address = copy.address;
  bus = NULL;
  return *this;
}

void I2CDevice::setBus(SensorAddress _busId)
{
  busId = _busId;
}


TwoWire* I2CDevice::getWire()
{
  if(bus!=NULL)
    return bus; // cached value
    
  if(owner) {
    I2CBus& i2c = (I2CBus&)owner->find(busId.device);
    if(i2c) {
      // todo: get the Nth i2c bus based on the slot, for now we only support 1 bus
      return bus = i2c.wire;
    } else
    return bus = &Wire; // return default i2c bus
  }
  return NULL;
}

} // ns:Nimble
