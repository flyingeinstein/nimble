
#include "Device.h"

// a do-nothing device, returned whenever find fails
Device NullDevice(-1, 0);


Device::Device(short _id, short _slots, unsigned long _updateInterval, unsigned long _flags)
  : id(_id), owner(NULL), slots(_slots), readings(NULL), flags(_flags), updateInterval(_updateInterval), nextUpdate(0), state(Offline)
{
  if(_slots>0)
    readings = (Slot*)calloc(slots, sizeof(Slot));
}

Device::Device(const Device& copy)
  : id(copy.id), owner(copy.owner), slots(copy.slots), readings(NULL), flags(copy.flags), updateInterval(copy.updateInterval), nextUpdate(0), state(copy.state)
{
  if(slots>0) {
    readings = (Slot*)calloc(slots, sizeof(Slot));
    memcpy(readings, copy.readings, slots*sizeof(Slot));  
  }
}

Device::~Device()
{
  // todo: notify our owner we are dying
  if(owner)
    owner->remove(*this);
  if(readings)
    free(readings);
}

Device& Device::operator=(const Device& copy)
{
  owner=copy.owner;
  id=copy.id;
  slots=copy.slots;
  readings=NULL;
  flags=copy.flags;
  updateInterval=copy.updateInterval;
  nextUpdate=copy.nextUpdate;
  state=copy.state;
  readings = (Slot*)calloc(slots, sizeof(Slot));
  memcpy(readings, copy.readings, slots*sizeof(Slot));
  return *this;
}

void Device::setOwner(Devices* _owner)
{
  owner = _owner;

  std::function<int(RestRequest&)> func = [](RestRequest& request) {
    request.response["error"] = "no device specific endpoint";
    return 404;
  };  
  //endpoints.onDefault( func );
  // endpoints.defaultHandler->setOwner( _owner );
}

void Device::alloc(unsigned short _slots)
{
  if(_slots >=MAX_SLOTS) {
    // essentially crash, an unrealistic number of slots requested
    Serial.print("internal error: ");
    Serial.print(_slots);
    Serial.print(" slots requested, limit is ");
    Serial.println(MAX_SLOTS);
    while(1) delay(10);
  }
  
  if(_slots != slots || !readings) {
    readings = readings
      ? (Slot*)realloc(readings, _slots*sizeof(Slot))
      : (Slot*)calloc(_slots, sizeof(Slot));
    slots = _slots;
  }
}

const char* Device::getDriverName() const
{
  return NULL; // indicates no name
}

Device::operator bool() const { 
  return this!=&NullDevice && id>=0; 
}

void Device::begin()
{
}

void Device::reset()
{
}

void Device::clear()
{
  for(int i=0; i<slots; i++)
    (*this)[i].clear();
}

void Device::delay(unsigned long _delay)
{
  nextUpdate = millis() + _delay;
}

String Device::prefixUri(const String& uri, short slot) const
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

void Device::onHttp(const String &uri, ESP8266WebServer::THandlerFunction handler)
{
    http().on( prefixUri(uri), handler);
}

void Device::onHttp(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn)
{
    http().on( prefixUri(uri), method, fn);
}

void Device::onHttp(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn, ESP8266WebServer::THandlerFunction ufn)
{
    http().on( prefixUri(uri), method, fn, ufn);
}

void Device::jsonGetReading(JsonObject& node, short slot) const
{
  if(slot >=0 && slot < slots) {
    SensorReading r = (*this)[slot];
    r.toJson(node);
  }
}

void Device::jsonGetReadings(JsonObject& node) const
{
  JsonArray jslots = node.createNestedArray("slots");
  for(short i=0, _i=slotCount(); i<_i; i++) {
    JsonObject jr = jslots.createNestedObject();
    SensorReading r = (*this)[i];
    r.toJson(jr);
  }
}

#if 0
void Device::httpGetReading(short slot) {
  ESP8266WebServer* http = getWebServer();
  if(http!=NULL && slot >=0 && slot < slots) {
    String value;
    SensorReading r = (*this)[slot];
    value += "{ \"address\": \"";
    value += id;
    value += ':';
    value += slot;
    value += "\", ";
    value += " \"type\": \"";
    value += SensorTypeName(r.sensorType);
    
    value += "\", \"valueType\": \"";
    value += r.valueType;

    value += "\", \"value\": ";
    value += r.toString();
    value += " }";

    http->send(200, "application/json", value);
  } else if(http!=NULL)
    http->send(400, "text/plain", "invalid slot");
}

void Device::httpPostValue(short slot) {
  ESP8266WebServer* http = getWebServer();
  if(http!=NULL && slot >=0 && slot < slots) {
    String value = http->arg("plain");
    SensorReading r = readings[slot];
    switch(r.valueType) {
      case 'i': 
      case 'l': r.l = value.toInt(); break;
      case 'f': r.l = atof(value.c_str()); break;
      case 'b': r.b = (value=="true"); break;      
    }
    readings[slot] = r;

    // echo back the value
    httpGetReading(slot);
  } else if(http!=NULL)
    http->send(400, "text/plain", "invalid slot");
}

void Device::enableDirect(short slot, bool _get, bool _post)
{
  ESP8266WebServer* http = getWebServer();
  if(http) {
    String prefix = prefixUri(String(), slot);

    // attach handler that can get the value
    if(_get) {
      Serial.print("GET (direct) ");
      Serial.println(prefix);
      http->on(
        prefix, 
        HTTP_GET, 
        std::bind(&Device::httpGetReading, this, slot)   // bind instance method to callable THanderFunction (std::function)
      );
    }

    // attach a handler that can set a value
    if(_post)
      http->on(
        prefix+"/value", 
        HTTP_POST, 
        std::bind(&Device::httpPostValue, this, slot)   // bind instance method to callable THanderFunction (std::function)
      );  
  }
}
#endif  // old Rest functions

String Device::getSlotAlias(short slotIndex) const
{
  return (slotIndex>=0 && slotIndex < slots)
    ? readings[slotIndex].alias
    : "";
}

void Device::setSlotAlias(short slotIndex, String alias)
{
  if (slotIndex>=0 && slotIndex < slots)
    readings[slotIndex].alias = alias;
}

short Device::findSlotByAlias(String slotAlias) const
{
  if(slotAlias.length()>0) {
    for(short i=0, _i=slotCount(); i<_i; i++) {
      if(readings[i].alias == slotAlias)
        return i;
    }
  }
  return -1;
}

void Device::handleUpdate()
{
}

bool Device::isStale(unsigned long _now) const
{
  if(_now==0)
    _now = millis();
  return _now > nextUpdate;
}

DeviceState Device::getState() const
{
  return state;
}

SensorReading& Device::operator[](unsigned short slotIndex)
{
  if(slotIndex >= slots)
    alloc( slotIndex+1 );
  return readings[slotIndex].reading;
}

const SensorReading& Device::operator[](unsigned short slotIndex) const
{
  if(slotIndex >= slots)
    return InvalidReading;
  return readings[slotIndex].reading;
}

int Device::toJson(JsonObject& target, JsonFlags displayFlags) const
{
  unsigned long long now = millis();
  const char* driver = getDriverName();
  const char* statename = DeviceStateName(getState());
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
    statistics.toJson(target);
    
  if(displayFlags & JsonSlots)
    jsonGetReadings(target);
  return 200;
}

void Device::Statistics::toJson(JsonObject& target) const
{
  target["updates"] = updates;
  JsonObject _errors = target.createNestedObject("errors");
  _errors["bus"] = errors.bus;
  _errors["sensing"] = errors.sensing;
}
