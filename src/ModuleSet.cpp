
// include core Wiring API
//#include <stdlib.h>
//#include <stdint.h>
//#include <HardwareSerial.h>


#include "ModuleSet.h"
#include "Module.h"

namespace Nimble {

ModuleSet::ModuleSet(short id, short maxModules)
  : Module(
      id,          // module set defaults to ID0 but will be changed when added to a parent ModuleSet
      maxModules,  // create maximum slots which hold the sub-module references
      0,           // no update interval, meaning we always immediately update each iteration
      MF_BUS       // set the flags on this module as containing sub-modules
  )
  , update_iterator(0) 
{
  // update all slot types to Module
  for(short i=0; i<slots; i++) {
    SensorReading& sr = readings[i].reading;
    sr.sensorType = SubModule;
    sr.valueType = VT_PTR;
  }
}

ModuleSet::~ModuleSet() {
}

#if 0
ModuleSet& ModuleSet::operator=(const ModuleSet& copy) {
    size=copy.size;
    count=copy.count;
    writePos=copy.writePos;
    Serial.print("ModuleSet.alloc assignment:"); Serial.println(size);
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
    memcpy(values, copy.values, copy.count*sizeof(SensorReading));
    return *this;
}
#endif


short ModuleSet::add(Module& dev)
{
  for(short i=0; i<slots; i++) 
  {
    // look for a free slot
    if(readings[i].reading.module==NULL) {
      SensorReading& sr = readings[i].reading;

      sr.module = &dev;
      dev.owner = this;
      dev.begin();
      return i;
    }
  }
  return -1;
}

void ModuleSet::remove(short deviceId) {
  for(short i=0; i<slots; i++) {
    if(readings[i].reading.module!=NULL && readings[i].reading.module->id == deviceId) {
      readings[i].reading.module = NULL;
    }
  }
}

void ModuleSet::remove(Module& dev) {
  for(short i=0; i<slots; i++) {
    if(readings[i].reading.module!=NULL && readings[i].reading.module == &dev) {
      readings[i].reading.module = NULL;
    }
  }
}

const Module& ModuleSet::getModule(short moduleID) const {
  // shortcut: check if indexed slot has module with matching ID
  if(moduleID < slots) {
    const SensorReading& sr = readings[moduleID].reading;
    if(sr.sensorType == SubModule && sr.valueType==VT_PTR && sr.module!=NULL && sr.module->id == moduleID)
      return *sr.module;
  } 

  // secondary check scans all slots
  for(short i=0; i<slots; i++) {
    const SensorReading& sr = readings[i].reading;
    if(sr.sensorType==SubModule && sr.valueType==VT_PTR && sr.module!=NULL && sr.module->id == moduleID)
      return *sr.module;
  }
  
  return NullModule;
}

Module& ModuleSet::getModule(short moduleID) {
  // shortcut: check if indexed slot has module with matching ID
  if(moduleID < slots) {
    SensorReading& sr = readings[moduleID].reading;
    if(sr.sensorType == SubModule && sr.valueType==VT_PTR && sr.module!=NULL && sr.module->id == moduleID)
      return *sr.module;
  }

  // secondary check scans all slots
  for(short i=0; i<slots; i++) {
    SensorReading& sr = readings[i].reading;
    if(sr.sensorType==SubModule && sr.valueType==VT_PTR && sr.module!=NULL && sr.module->id == moduleID)
      return *sr.module;
  }
  return NullModule;
}

const Module& ModuleSet::getModule(String alias) const {
  if(alias.length() >0) {
    const SensorReading& reading = Module::find(alias, SubModule);
    return (reading && reading.sensorType == SubModule && reading.module)
      ? *reading.module
      : NullModule;
  } else
    return NullModule;
}

Module& ModuleSet::getModule(String alias) {
  if(alias.length() >0) {
    SensorReading& reading = Module::find(alias, SubModule);
    return (reading && reading.sensorType == SubModule && reading.module)
      ? *reading.module
      : NullModule;
  } else
    return NullModule;
}

SensorReading ModuleSet::getReading(const SensorAddress& sa) const
{ 
  return getReading(sa.device, sa.slot); 
}

SensorReading ModuleSet::getReading(short deviceId, unsigned short slotId) const
{
  const Module& mod = getModule(deviceId);
  return mod
    ? mod[slotId]
    : NullReading;
}

ModuleSet::ReadingIterator::ReadingIterator(ModuleSet* _manager)
  : sensorTypeFilter(Invalid), valueTypeFilter(0), tsFrom(0), tsTo(0), 
    device(NULL), slot(0), manager(_manager), singleModule(false), deviceOrdinal(0)
{
}

ModuleSet::ReadingIterator& ModuleSet::ReadingIterator::OfType(SensorType st)
{
  sensorTypeFilter = st;
  return *this;
}

ModuleSet::ReadingIterator& ModuleSet::ReadingIterator::TimeBetween(unsigned long from, unsigned long to)
{
  tsFrom = from;
  tsTo = to;
  return *this;
}

ModuleSet::ReadingIterator& ModuleSet::ReadingIterator::Before(unsigned long ts)
{
  tsTo = ts;
  return *this;
}

ModuleSet::ReadingIterator& ModuleSet::ReadingIterator::After(unsigned long ts)
{
  tsFrom = (ts==0) ? 0 : ts-1;
  return *this;
}

SensorReading ModuleSet::ReadingIterator::next()
{
  if(manager==NULL)
    return InvalidReading;  // nothing to iterate
  if(device ==NULL) {
    // get first device
    deviceOrdinal=0; slot=0;
    while(manager->readings[deviceOrdinal].reading.sensorType != SubModule && manager->readings[deviceOrdinal].reading.module!=nullptr)
      deviceOrdinal++;
    device = manager->readings[deviceOrdinal].reading.module;
    if(device==NULL) {
      return InvalidReading;
    }
  } else
    slot++;

  while(device !=NULL) {
    // check if next slot is valid
    while(slot < device->slotCount()) {
      SensorReading r = (*device)[slot];
      if(r.sensorType!=Invalid && r.valueType!=VT_INVALID &&
        (sensorTypeFilter==Invalid || r.sensorType==sensorTypeFilter) &&
        (valueTypeFilter==0 || r.valueType==valueTypeFilter) &&
        (r.timestamp >= tsFrom && (tsTo==0 || r.timestamp < tsTo))) {
          return r;
      }
      slot++;
    }

    // must advance to next device
    if(singleModule)
      return InvalidReading;  // only read from one device
    
    // find the next device
    deviceOrdinal++;
    while(manager->readings[deviceOrdinal].reading.sensorType != SubModule && manager->readings[deviceOrdinal].reading.module!=nullptr)
      deviceOrdinal++;
    device = manager->readings[deviceOrdinal].reading.module;

    slot = 0;
  }
  return InvalidReading;  // end of readings
}

ModuleSet::ReadingIterator ModuleSet::forEach()
{
  return ReadingIterator(this);
}

ModuleSet::ReadingIterator ModuleSet::forEach(short deviceId)
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == SubModule && reading.module!=nullptr && reading.module->id == deviceId) {
      ReadingIterator itr = ReadingIterator(this);
      itr.deviceOrdinal = i;
      itr.device = reading.module;
      itr.singleModule = true;
      return itr;
    }
  }
  return ReadingIterator(NULL);
}

ModuleSet::ReadingIterator ModuleSet::forEach(SensorType st)
{
  ReadingIterator itr = ReadingIterator(this);
  itr.sensorTypeFilter = st;
  return itr;  
}

void ModuleSet::handleUpdate()
{
  unsigned long long _now = millis();
  update_iterator = (update_iterator+1) % slots;  // rolling iterator
  while(readings[update_iterator].reading.sensorType != SubModule && readings[update_iterator].reading.module!=nullptr)
    update_iterator = (update_iterator+1) % slots;  // rolling iterator
  Module* device = readings[update_iterator].reading.module;

  if(device!=NULL && device->isStale(_now)) {
      device->nextUpdate = _now + device->updateInterval;
      device->handleUpdate();
      return; // only handle one update at a time
  }

}

} // ns:Nimble
