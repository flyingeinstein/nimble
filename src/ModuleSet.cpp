
// include core Wiring API
//#include <stdlib.h>
//#include <stdint.h>
//#include <HardwareSerial.h>


#include "ModuleSet.h"
#include "Module.h"

namespace Nimble {

ModuleSet::ModuleSet(short maxModules)
  : slots(maxModules), devices(NULL), update_iterator(0) {
    devices = (Module**)calloc(slots, sizeof(Module*));
}

ModuleSet::~ModuleSet() {
  if(devices) free(devices);
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

void ModuleSet::begin()
{
}


short ModuleSet::add(Module& dev)
{
  for(short i=0; i<slots; i++) 
  {
    // look for a free slot
    if(devices[i]==NULL) {
      devices[i] = &dev;
      dev.owner = this;
      dev.begin();
      return i;
    }
  }
  return -1;
}

void ModuleSet::remove(short deviceId) {
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i]->id == deviceId) {
      devices[i] = NULL;
    }
  }
}

void ModuleSet::remove(Module& dev) {
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i] == &dev) {
      devices[i] = NULL;
    }
  }
}

const Module& ModuleSet::find(short deviceId) const
{
  for(short i=0; i<slots; i++) 
    if(devices[i] && devices[i]->id == deviceId)
      return *devices[i];
  return NullModule;
}

Module& ModuleSet::find(short deviceId)
{
  for(short i=0; i<slots; i++) 
    if(devices[i] && devices[i]->id == deviceId)
      return *devices[i];
  return NullModule;
}

const Module& ModuleSet::find(String deviceAlias) const
{
  if(deviceAlias.length()!=0) {
    for(short i=0; i<slots; i++) 
      if(devices[i] && devices[i]->alias == deviceAlias)
        return *devices[i];
  }
  return NullModule;
}

Module& ModuleSet::find(String deviceAlias)
{
  if(deviceAlias.length()!=0) {
    for(short i=0; i<slots; i++) 
      if(devices[i] && devices[i]->alias == deviceAlias)
        return *devices[i];
  }
  return NullModule;
}

SensorReading ModuleSet::getReading(const SensorAddress& sa) const
{ 
  return getReading(sa.device, sa.slot); 
}

SensorReading ModuleSet::getReading(short deviceId, unsigned short slotId) const
{
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i]->id == deviceId) {
      // get the slot
      Module* device = devices[i];
      return (slotId < device->slotCount())
        ? (*device)[slotId]
        : SensorReading(); 
    }
  }
  return SensorReading();
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
    device = manager->devices[0];
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
    device = manager->devices[++deviceOrdinal];
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
    if(devices[i] && devices[i]->id == deviceId) {
      ReadingIterator itr = ReadingIterator(this);
      itr.deviceOrdinal = i;
      itr.device = devices[i];
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

void ModuleSet::clearAll()
{
  for(short i=0; i<slots; i++)
    if(devices[i]!=NULL)
      devices[i]->clear();
}

void ModuleSet::handleUpdate()
{
  unsigned long long _now = millis();
  for(short n = slots; n>0; n--) {
    Module* device = devices[update_iterator];
    update_iterator = (update_iterator+1) % slots;  // rolling iterator

    if(device!=NULL && device->isStale(_now)) {
        device->nextUpdate = _now + device->updateInterval;
        device->handleUpdate();
        return; // only handle one update at a time
    }
  }
}

} // ns:Nimble
