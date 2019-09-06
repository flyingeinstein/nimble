#include "I2CBus.h"

#include <Wire.h>

namespace Nimble {
namespace I2C {

Bus::Bus(short id)
  : ModuleSet(id, 128), wire(&Wire)
{
  flags |= MF_I2C_BUS;
}

#if 0
I2CBus::I2CBus(const I2CBus& copy)
  : ModuleSet(copy), wire(copy.wire)
{
}

I2CBus& I2CBus::operator=(const I2CBus& copy)
{
  Module::operator=(copy);
  wire = copy.wire;
  return *this;
}
#endif

const char* Bus::getDriverName() const
{
  return "i2c-bus";
}

void Bus::handleUpdate()
{
  state = Nominal;
}



Device::Device(short id, short _address, short _slots)
  : Module(id, _slots), address(_address), bus(NULL)
{
}

Device::Device(short id, SensorAddress _busId, short _address, short _slots)
  : Module(id, _slots, 1000), busId(_busId), address(_address), bus(NULL)
{
}

Device::Device(const Device& copy)
  : Module(copy), busId(copy.busId), address(copy.address), bus(NULL)
{
}

Device& Device::operator=(const Device& copy)
{
  Module::operator=(copy);
  busId = copy.busId;
  address = copy.address;
  bus = NULL;
  return *this;
}

void Device::setBus(SensorAddress _busId)
{
  busId = _busId;
}


TwoWire* Device::getWire()
{
  if(bus!=NULL)
    return bus; // cached value
    
  if(owner && owner->hasFlags(MF_I2C_BUS) ) {
    Bus& i2c = *(Bus*)owner;
    if(i2c.wire) {
      // todo: get the Nth i2c bus based on the slot, for now we only support 1 bus
      return bus = i2c.wire;
    } else
      return bus = &Wire; // return default i2c bus
  }
  return NULL;
}

} // ns:I2C
} // ns:Nimble
