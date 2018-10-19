
// include core Wiring API
#include <stdlib.h>
#include <stdint.h>
#include <HardwareSerial.h>


#include "DeviceManager.h"

const char* SensorTypeName(SensorType st)
{
  switch(st) {
    case Humidity: return "humidity";
    case Hygrometer: return "hygrometer";
    case Temperature: return "temperature";
    case Motion: return "motion";
    default: return "n/a";
  }
}

const char* DeviceStateName(DeviceState dt)
{
  switch(dt) {
    case Offline: return "offline";
    case Degraded: return "degraded";
    case Nominal: return "nominal";
    default:
      return "unknown";
  }
}

// the main device manager
Devices DeviceManager;

// a do-nothing device, returned whenever find fails
Device NullDevice(-1, 0);

SensorReading NullReading(Invalid, VT_NULL, 0);
SensorReading InvalidReading(Invalid, VT_INVALID, 0);

const DeviceDriverInfo** Devices::drivers = NULL;
short Devices::driversSize = 0;
short Devices::driversCount = 0;

void Devices::registerDriver(const DeviceDriverInfo* driver)
{
  if(drivers == NULL) {
    // first init
    drivers = (const DeviceDriverInfo**)calloc(driversSize=16, sizeof(DeviceDriverInfo*));
  }
  drivers[driversCount++] = driver;
}

const DeviceDriverInfo* Devices::findDriver(const char* name)
{
  // todo: split name into category if exists
  for(short i=0; i<driversCount; i++) {
    if(strcmp(name, drivers[i]->name)==0)
      return drivers[i];
  }
  return NULL;
}

Devices::Devices(short _maxDevices)
  : slots(_maxDevices), devices(NULL), update_iterator(0), ntp(NULL) {
    devices = (Device**)calloc(slots, sizeof(Device*));
}

Devices::~Devices() {
  if(devices) free(devices);
}

#if 0
Devices& Devices::operator=(const Devices& copy) {
    size=copy.size;
    count=copy.count;
    writePos=copy.writePos;
    Serial.print("Devices.alloc assignment:"); Serial.println(size);
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
    memcpy(values, copy.values, copy.count*sizeof(SensorReading));
    return *this;
}
#endif

void Devices::begin(NTPClient& client)
{
  ntp = &client;
}

short Devices::add(Device* dev)
{
  for(short i=0; i<slots; i++) 
  {
    // look for a free slot
    if(devices[i]==NULL) {
      devices[i] = dev;
      return i;
    }
  }
  return -1;
}

const Device& Devices::find(short deviceId) const
{
  for(short i=0; i<slots; i++) 
    if(devices[i] && devices[i]->id == deviceId)
      return *devices[i];
  return NullDevice;
}

Device& Devices::find(short deviceId)
{
  for(short i=0; i<slots; i++) 
    if(devices[i] && devices[i]->id == deviceId)
      return *devices[i];
  return NullDevice;
}

SensorReading Devices::getReading(short deviceId, unsigned short slotId)
{
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i]->id == deviceId) {
      // get the slot
      Device* device = devices[i];
      return (slotId < device->slotCount())
        ? (*device)[slotId]
        : SensorReading(); 
    }
  }
  return SensorReading();
}

Devices::ReadingIterator::ReadingIterator(Devices* _manager)
  : sensorTypeFilter(Invalid), valueTypeFilter(0), tsFrom(0), tsTo(0), 
    device(NULL), slot(0), manager(_manager), singleDevice(false), deviceOrdinal(0)
{
}

Devices::ReadingIterator& Devices::ReadingIterator::OfType(SensorType st)
{
  sensorTypeFilter = st;
  return *this;
}

Devices::ReadingIterator& Devices::ReadingIterator::TimeBetween(unsigned long from, unsigned long to)
{
  tsFrom = from;
  tsTo = to;
  return *this;
}

Devices::ReadingIterator& Devices::ReadingIterator::Before(unsigned long ts)
{
  tsTo = ts;
  return *this;
}

Devices::ReadingIterator& Devices::ReadingIterator::After(unsigned long ts)
{
  tsFrom = (ts==0) ? 0 : ts-1;
  return *this;
}

SensorReading Devices::ReadingIterator::next()
{
  if(manager==NULL)
    return InvalidReading;  // nothing to iterate
  if(device ==NULL) {
    // get first device
    deviceOrdinal=0; slot=0;
    device = manager->devices[0];
    if(device==NULL) {
      Serial.println("NoDevices");
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
    if(singleDevice)
      return InvalidReading;  // only read from one device
    device = manager->devices[++deviceOrdinal];
    slot = 0;
  }
  return InvalidReading;  // end of readings
}

Devices::ReadingIterator Devices::forEach()
{
  return ReadingIterator(this);
}

Devices::ReadingIterator Devices::forEach(short deviceId)
{
    for(short i=0; i<slots; i++) 
    if(devices[i] && devices[i]->id == deviceId) {
      ReadingIterator itr = ReadingIterator(this);
      itr.deviceOrdinal = i;
      itr.device = devices[i];
      itr.singleDevice = true;
      return itr;
    }
    return ReadingIterator(NULL);
}

Devices::ReadingIterator Devices::forEach(SensorType st)
{
  ReadingIterator itr = ReadingIterator(this);
  itr.sensorTypeFilter = st;
  return itr;  
}

void Devices::clearAll()
{
  for(short i=0; i<slots; i++)
    if(devices[i]!=NULL)
      devices[i]->clear();
}

void Devices::handleUpdate()
{
  unsigned long long _now = millis();
  for(short n = slots; n>0; n--) {
    Device* device = devices[update_iterator];
    update_iterator = (update_iterator+1) % slots;  // rolling iterator

    if(device!=NULL && device->isStale(_now)) {
        device->nextUpdate = _now + device->updateInterval;
        device->handleUpdate();
        return; // only handle one update at a time
    }
  }
}

#if 0
void Devices::write(const SensorReading& value) { 
  if(writePos>=size)
    alloc(size*1.25 + 1);
  if(writePos < count && values[writePos].sensor==value.sensor) {
    // update of reading
    values[writePos++] = value; 
  } else if(writePos < count) {
    // sensor is writing more values this time, insert the value
    if(count>=size)
      alloc(size*1.25 + 1);
    memmove(&values[writePos+1], &values[writePos], sizeof(values[0])*(count-writePos));  // make room for new reading
    count++;  // we expanded by 1
    values[writePos++] = value; // insert new reading
  } else {
    // new reading to append
    values[writePos] = value; 
    writePos++;
    count++;
  }
}

void Devices::alloc(short n) {
  Serial.print("Devices.alloc realloc:"); Serial.println(n);
  if(values) {
    values = (SensorReading*)realloc(values, n*sizeof(SensorReading));
    size = n;
  } else {
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
  }
}
#endif


Device::Device(short _id, short _slots, unsigned long _updateInterval)
  : id(_id), slots(_slots), readings(NULL), updateInterval(_updateInterval), nextUpdate(0), state(Offline)
{
  if(_slots>0)
    readings = (SensorReading*)calloc(slots, sizeof(SensorReading));
}

Device::Device(const Device& copy)
  : id(copy.id), slots(copy.slots), readings(NULL), updateInterval(copy.updateInterval), nextUpdate(0), state(copy.state)
{
  if(slots>0) {
    readings = (SensorReading*)calloc(slots, sizeof(SensorReading));
    memcpy(readings, copy.readings, slots*sizeof(SensorReading));  
  }
}

Device::~Device()
{
  if(readings)
    free(readings);
}

Device& Device::operator=(const Device& copy)
{
  id=copy.id;
  slots=copy.slots;
  readings=NULL;
  updateInterval=copy.updateInterval;
  nextUpdate=0;
  state=copy.state;
  readings = (SensorReading*)calloc(slots, sizeof(SensorReading));
  memcpy(readings, copy.readings, slots*sizeof(SensorReading));
  return *this;
}

void Device::alloc(unsigned short _slots)
{
  if(_slots != slots || !readings) {
    readings = readings
      ? (SensorReading*)realloc(readings, _slots*sizeof(SensorReading))
      : (SensorReading*)calloc(_slots, sizeof(SensorReading));
    slots = _slots;
  }
}

void Device::reset()
{
}

void Device::clear()
{
  for(int i=0; i<slots; i++)
    readings[i].clear();
}

void Device::handleUpdate()
{
  
}

bool Device::isStale(unsigned long long _now) const
{
  if(_now==0)
    _now = millis();
  return _now > nextUpdate;
}

DeviceState Device::getState() const
{
  return state;
}

SensorReading Device::operator[](unsigned short slotIndex) const
{
  return (slotIndex < slots)
    ? readings[slotIndex]
    : InvalidReading;
}


void SensorReading::clear()
{
  valueType = VT_CLEAR;
  timestamp = millis();
  l = 0;
}

String SensorReading::toString() const {
    switch(valueType) {
      default:
      case 'n': return "null"; break;
      case 'i':
      case 'l': return String(l); break;
      case 'f': return String(f); break;
      case 'b': return String(b ? "true":"false"); break;
    }  
}
