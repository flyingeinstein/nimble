
#include "Module.h"
#include "ModuleManager.h"


namespace Nimble {

// a do-nothing device, returned whenever find fails
Module NullModule(-1, 0);

Module::Slot Module::Slot::makeError(long errorCode, String errstr) {
  Slot s;
  s.alias = errstr;
  s.reading = SensorReading(ErrorCode, errorCode);
  return s;
}

Module::Module(short _id, short _slots, unsigned long _updateInterval, unsigned long _flags)
  : id(_id), owner(NULL), slots(_slots), readings(NULL), flags(_flags), updateInterval(_updateInterval), nextUpdate(0), state(Offline)
{
  if(_slots > MAX_SLOTS) 
    _slots = MAX_SLOTS;
  if(_slots>0)
    readings = (Slot*)calloc(slots, sizeof(Slot));
}

Module::Module(const Module& copy)
  : id(copy.id), owner(copy.owner), slots(copy.slots), readings(NULL), flags(copy.flags), updateInterval(copy.updateInterval), nextUpdate(0), state(copy.state)
{
  if(slots>0) {
    readings = (Slot*)calloc(slots, sizeof(Slot));
    memcpy(readings, copy.readings, slots*sizeof(Slot));  
  }
}

Module::~Module()
{
  // todo: notify our owner we are dying
  if(owner)
    owner->remove(*this);
  if(readings)
    free(readings);
}

Module& Module::operator=(const Module& copy)
{
  owner=copy.owner;
  id=copy.id;
  slots=copy.slots;
  readings=NULL;
  flags=copy.flags;
  #if 0
  // todo: Endpoints class needs a copy constructor/assignment
  if(copy._endpoints) {
    // copy the rhs endpoints
    if(_endpoints)
      *_endpoints = *copy._endpoints;
    else
      *_endpoints = new Endpoints(*copy._endpoints);
  } else if(_endpoints) {
    // clear out our endpoints
    delete _endpoints;
    _endpoints = nullptr;
  }
  #endif
  updateInterval=copy.updateInterval;
  nextUpdate=copy.nextUpdate;
  state=copy.state;
  readings = (Slot*)calloc(slots, sizeof(Slot));
  memcpy(readings, copy.readings, slots*sizeof(Slot));
  return *this;
}

void Module::setOwner(ModuleSet* _owner)
{
  owner = _owner;
}

void Module::alloc(unsigned short _slots)
{
  if(_slots >=MAX_SLOTS) {
    // essentially crash, an unrealistic number of slots requested
    Serial.print("internal error: ");
    Serial.print(_slots);
    Serial.print(" slots requested, limit is ");
    Serial.println(MAX_SLOTS);
    while(1) ::delay(10);
  }
  
  if(_slots != slots || !readings) {
    readings = readings
      ? (Slot*)realloc(readings, _slots*sizeof(Slot))
      : (Slot*)calloc(_slots, sizeof(Slot));
    slots = _slots;
  }
}

const char* Module::getDriverName() const
{
  return NULL; // indicates no name
}

Module::operator bool() const {
  return this!=&NullModule && id>=0;
}

void Module::begin()
{
  forEach( [](Module& mod, void* pUserData) -> bool {
    mod.begin();
    return true;
  });
}

void Module::reset()
{
  forEach( [](Module& module, void* pUserData) -> bool {
    module.reset();
    return true;
  });
}

void Module::clear()
{
  forEach( [](SensorReading& reading, void* pUserData) -> bool {
    // if slot is a module, then call it's clear method...dont clear out the module itself!
    if( reading.sensorType == SubModule && reading.valueType==VT_PTR && reading.module!=nullptr)
      reading.module->clear();
    else
      reading.clear();
    return true;
  });
}

void Module::handleUpdate()
{
  forEach( [](Module& module, void* pUserData) -> bool {
    module.handleUpdate();
    return true;
  });
}

void Module::delay(unsigned long _delay)
{
  nextUpdate = millis() + _delay;
}

String Module::prefixUri(const String& uri, short slot) const
{
  String u;
  u += "/dev/";
  u += id;
  if(slot>=0) {
    u += "/slot/";
    u += slot;
  }
  if(uri.length() >0) {
    u += '/';
    u += uri;
  }
  return u;
}

WebServer& Module::http()
{
  return ModuleManager::Default.http();
}

void Module::onHttp(const String &uri, ESP8266WebServer::THandlerFunction handler)
{
    ModuleManager::Default
      .http()
      .on( prefixUri(uri), handler);
}

void Module::onHttp(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn)
{
    ModuleManager::Default
      .http()
      .on( prefixUri(uri), method, fn);
}

void Module::onHttp(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn, ESP8266WebServer::THandlerFunction ufn)
{
    ModuleManager::Default
      .http()
      .on( prefixUri(uri), method, fn, ufn);
}

String Module::getSlotAlias(short slotIndex) const
{
  return (slotIndex>=0 && slotIndex < slots)
    ? readings[slotIndex].alias
    : "";
}

void Module::setSlotAlias(short slotIndex, String alias)
{
  if (slotIndex>=0 && slotIndex < slots)
    readings[slotIndex].alias = alias;
}

short Module::findSlotByAlias(String slotAlias) const
{
  if(slotAlias.length()>0) {
    for(short i=0, _i=slotCount(); i<_i; i++) {
      if(readings[i].alias == slotAlias)
        return i;
    }
  }
  return -1;
}

SensorReading& Module::find(String alias, SensorType stype)
{
  if(alias.length() >0) {
    for(short i=0; i<slots; i++) {
      if(((stype == AnySensorType) || (readings[i].reading.sensorType == stype))    // match sensor type (if set)
            && readings[i].alias == alias                                                 // match alias name
        ) {
          return readings[i].reading;
        }
    }
  }
  return NullReading;
}

const SensorReading& Module::find(String alias, SensorType stype) const
{
  if(alias.length() >0) {
    for(short i=0; i<slots; i++) {
      if(((stype == AnySensorType) || (readings[i].reading.sensorType == stype))    // match sensor type (if set)
            && readings[i].alias == alias                                                 // match alias name
        ) {
          return readings[i].reading;
        }
    }
  }
  return NullReading;
}

Module::Slot Module::slotByAddress(const char* slotId, SensorType requiredType)
{
  const char* b = slotId;
  bool isNumeric = true;
  short id = 0;
  while( *slotId && *slotId !=':') {
    if(isNumeric && isdigit(*slotId)) {
      // accumulate ID value
      id = id*10 + (*slotId - '0');
    } else
      isNumeric = false;
  }

  Slot* slot= nullptr;
  if( slotId > b) {
    // we processed at least 1 character, is it an ID or alias?
    if(isNumeric) {
      // find the slot by ID
      slot = &readings[id];
    } else {
      // find the slot by alias
      short len = slotId - b;
      for(short i=0; i<slots; i++) {
        if(strncmp(readings[i].alias.c_str(), b, len)==0) {
          slot = &readings[i];
          break;
        }
      }
    }
  }

  if(slot != nullptr) {
    // if there is more to the address, then recurse
    if(*slotId) {
      assert(*slotId == ':'); // must be since the while() above exits on only : or \0

      // slot must then be a module
      if(slot->reading.sensorType != SubModule)
        return Slot::makeError(-2, "one or more address prefixes were not of module type");  // expected module
      // recurse into module
      return slot->reading.module->slotByAddress(slotId, requiredType);
    } else {
      // check that slot type matches
      return (requiredType == AnySensorType || slot->reading.sensorType == requiredType)
        ? *slot
        : Slot::makeError(-3, "type mismatch");   // required type mismatch
    }
  }
  return Slot::makeError(-1, "bad module or slot address");  // bad slot address
}

void Module::forEach(SlotCallback cb, void* pUserData, SensorType st)
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == st) {
      if(!cb(readings[i], pUserData))
        return;
    }
  }
}

void Module::forEach(ReadingCallback cb, void* pUserData, SensorType st)
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == st) {
      if(!cb(reading, pUserData))
        return;
    }
  }
}

void Module::forEach(ModuleCallback cb, void* pUserData)
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == SubModule && reading.valueType==VT_PTR && reading.module!=nullptr) {
      if(!cb(*reading.module, pUserData))
        return;
    }
  }
}

void Module::forEach(ConstSlotCallback cb, void* pUserData, SensorType st) const
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == st) {
      if(!cb(readings[i], pUserData))
        return;
    }
  }
}

void Module::forEach(ConstReadingCallback cb, void* pUserData, SensorType st) const
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == st) {
      if(!cb(reading, pUserData))
        return;
    }
  }
}

void Module::forEach(ConstModuleCallback cb, void* pUserData) const
{
  for(short i=0; i<slots; i++) {
    SensorReading reading = readings[i].reading;
    if(reading.sensorType == SubModule && reading.valueType==VT_PTR && reading.module!=nullptr) {
      if(!cb(*reading.module, pUserData))
        return;
    }
  }
}

bool Module::isStale(unsigned long _now) const
{
  if(_now==0)
    _now = millis();
  return _now > nextUpdate;
}

ModuleState Module::getState() const
{
  return state;
}

SensorReading& Module::operator[](unsigned short slotIndex)
{
  if(slotIndex >= slots)
    alloc( slotIndex+1 );
  return readings[slotIndex].reading;
}

const SensorReading& Module::operator[](unsigned short slotIndex) const
{
  if(slotIndex >= slots)
    return InvalidReading;
  return readings[slotIndex].reading;
}

SensorReading& Module::operator[](const Rest::Argument& arg) {
  return arg.isInteger()
      ? operator[]( (long)arg )
      : arg.isString()
        ? operator[]( (String)arg )
        : NullReading;
}

void Module::jsonGetReading(JsonObject& node, short slot) const
{
  auto reading = find(slot);
  reading.toJson(node);
}

void Module::jsonGetReadings(JsonObject& node) const
{
  JsonArray jslots = node.createNestedArray("slots");
  forEach( [&jslots,&node](const SensorReading& reading, void* userData) -> bool {
    JsonObject jr = jslots.createNestedObject();
    reading.toJson(jr);
    return true;
  }, nullptr);
}

int Module::toJson(JsonObject& target, JsonFlags displayFlags) const
{
  unsigned long long now = millis();
  unsigned long nextUpdate = getNextUpdate();
  const char* driver = getDriverName();
  const char* statename = ModuleStateName(getState());
  String alias = getAlias();
  if(alias.length()>0)
    target["alias"] = alias;
  if(driver)
    target["driver"] = driver;
  target["flags"] = getFlags();
  target["state"] = statename;

  if(now < nextUpdate)
    target["nextUpdate"] = nextUpdate - now;

  if(displayFlags & JsonStatistics)
    getStatistics().toJson(target);
    
  if(displayFlags & JsonSlots)
    jsonGetReadings(target);
  return 200;
}

void Module::Statistics::toJson(JsonObject& target) const
{
  target["updates"] = updates;
  JsonObject _errors = target.createNestedObject("errors");
  _errors["bus"] = errors.bus;
  _errors["sensing"] = errors.sensing;
}

} // ns:Nimble
