
// include core Wiring API
#include <stdlib.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <FS.h>


#include "DeviceManager.h"


String SensorAddress::toString() const
{
  String s;
  s += device;
  s += ':';
  s += slot;
  return s;
}


const char* SensorTypeName(SensorType st)
{
  switch(st) {
    case Invalid: return "invalid";
    case Numeric: return "numeric";
    case Timestamp: return "timestamp";
    case Milliseconds: return "milliseconds";
    case Humidity: return "humidity";
    case Hygrometer: return "hygrometer";
    case Temperature: return "temperature";
    case HeatIndex: return "heatindex";
    case Illuminance: return "illuminance";
    case pH: return "pH";
    case ORP: return "ORP";
    case DissolvedOxygen: return "dissolved oxygen";
    case Conductivity: return "conductivity";
    case CO2: return "CO2";
    case Pressure: return "pressure";
    case Flow: return "flow";
    case Altitude: return "altitude";
    case AirPressure: return "air pressure";
    case AirQuality: return "air quality";
    case Voltage: return "voltage";
    case Current: return "current";
    case Watts: return "watts";
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
  : slots(_maxDevices), devices(NULL), restHandler(NULL), update_iterator(0), ntp(NULL) {
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

void Devices::begin(WebServer& _http, NTPClient& _ntp)
{
  httpServer = &_http;
  ntp = &_ntp;

  if(restHandler == NULL) {
    restHandler = new RestRequestHandler();
    setupRestHandler();
    httpServer->addHandler(restHandler);
  }
}

/*int Devices::deviceRestHandler(RestRequest& request)
{
  const char* _url = request["_url"];
  Rest::Argument req_dev = request["device"];

  Device* dev = (req_dev.isNumber())
                ? &find( (long)req_dev )
                : (req_dev.isString())
                  ? &find( (const char*)req_dev )
                  : nullptr;

  if(dev != nullptr) {
    Endpoints::Endpoint ep = dev->endpoints.resolve(request.method, _url);
    if(ep.status == URL_MATCHED) {
      request.args = request.args + ep;
      ep.handler(request);
    }
  }
}*/

void Devices::setupRestHandler()
{  
  std::function<int(RestRequest&)> func = [](RestRequest& request) {
    String s("Hello ");
    auto msg = request["msg"];
    if(msg.isString())
      s += msg.toString();
    else {
      s += '#';
      s += (long)msg;
    }
    request.response["reply"] = s;
    return 200;
  };
  //on("/api/dev/:device(string|integer)/*", ANY(std::bind(&Devices::deviceRestHandler, this, std::placeholders::_1)));
  on("/api/echo/:msg(string|integer)").GET( func );
  /*restHandler.on("/api/echo/:msg(string|integer)", PUT([](RestRequest& request) {
    request.response["reply"] = "Smello World!";
    return 200;
  ));*/
}

short Devices::add(Device& dev)
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

void Devices::remove(short deviceId) {
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i]->id == deviceId) {
      devices[i] = NULL;
    }
  }
}

void Devices::remove(Device& dev) {
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i] == &dev) {
      devices[i] = NULL;
    }
  }
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

const Device& Devices::find(String deviceAlias) const
{
  if(deviceAlias.length()!=0) {
    for(short i=0; i<slots; i++) 
      if(devices[i] && devices[i]->alias == deviceAlias)
        return *devices[i];
  }
  return NullDevice;
}

Device& Devices::find(String deviceAlias)
{
  if(deviceAlias.length()!=0) {
    for(short i=0; i<slots; i++) 
      if(devices[i] && devices[i]->alias == deviceAlias)
        return *devices[i];
  }
  return NullDevice;
}

SensorReading Devices::getReading(const SensorAddress& sa) const 
{ 
  return getReading(sa.device, sa.slot); 
}

SensorReading Devices::getReading(short deviceId, unsigned short slotId) const
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
  for(short i=0; i<slots; i++) {
    if(devices[i] && devices[i]->id == deviceId) {
      ReadingIterator itr = ReadingIterator(this);
      itr.deviceOrdinal = i;
      itr.device = devices[i];
      itr.singleDevice = true;
      return itr;
    }
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

String Devices::getAliasesFile()
{
  String out;
  ReadingIterator itr = forEach();
  SensorReading r;
  Device* lastDev = NULL;
  while( (r = itr.next()) ) {
    if(lastDev != itr.device) {
      // check for device alias
      if(itr.device->alias.length()) {
        out += itr.device->id;
        out += '=';
        out += itr.device->alias;
        out += '\n';
      }
      lastDev = itr.device;
    }
    
    String alias = itr.device->getSlotAlias(itr.slot);
    if(alias.length()) {
        out += itr.device->id;
        out += ':';
        out += itr.slot;
        out += '=';
        out += alias;
        out += '\n';
    }
  }
  return out;
}

int Devices::parseAliasesFile(const char* aliases)
{
  int parsed = 0;
  
  while(*aliases) {
    short devid=-1, slotid=-1;

    // trim white space
    while(*aliases && isspace(*aliases))
      aliases++;

    // ignore extra lines or comments (#)
    if(*aliases =='\n') {
      aliases++;
      continue;
    } else if(*aliases == '#') {
      while(*aliases && *aliases++!='\n')
        ;
      continue;
    }
      
    
    // read device ID
    if(isdigit(*aliases)) {
      devid = (short)atoi(aliases);
      while(isdigit(*aliases))
        aliases++;
    } else {
      // bad line, skip
      while(*aliases && *aliases++!='\n')
        ;
      continue;
    }
      
    // optionally read slot
    if(*aliases == ':') {
      aliases++;
      
      // read slot
      if(isdigit(*aliases)) {
        slotid = (short)atoi(aliases);
        while(isdigit(*aliases))
          aliases++;
      } else {
        // bad line, skip
        while(*aliases && *aliases++!='\n')
          ;
        continue;
      }
    }

    // expect =, then read alias
    if(*aliases == '=') {
      aliases++;

      // record where we are, then skip to end of line
      String alias;
      while(*aliases && *aliases!='\r' && *aliases!='\n') {
        alias += *aliases;
        aliases++;
      }
      if(*aliases == '\r')
        aliases++;
      if(*aliases == '\n')
        aliases++;

      // now set the alias
      Device& dev = find(devid);
      if(dev) {
        if(slotid>=0) {
          // set slot alias
          dev.setSlotAlias(slotid, alias);
          parsed++;
        } else {
          // set device alias
          dev.alias = alias;
          parsed++;
        }
      }
    }
  }
  return parsed;
}

int Devices::restoreAliasesFile() {
  File f = SPIFFS.open("/aliases.txt", "r");
  if(f) {
    String aliases = f.readString();
    int parsed = parseAliasesFile(aliases.c_str());
    f.close();
    Serial.println("loaded aliases file");
    return parsed;
  }
  return 0;
}

bool Devices::saveAliasesFile(const char* aliases)
{
  File f = SPIFFS.open("/aliases.txt", "w");
  if(f) {
      f.print(aliases);
      f.close();
      return true;
  }
  return false;
}

void Devices::jsonGetDevices(JsonObject& root)
{
  // list all devices
  JsonArray devs = root.createNestedArray("devices");
  for(short i=0; i < slots; i++) {
    if(devices[i]) {
      // get the slot
      Device* device = devices[i];
      const char* driverName = device->getDriverName();
      JsonObject jdev = devs.createNestedObject();

      // id
      jdev["id"] = device->id;

      // driver name
      if(driverName!=NULL)
        jdev["driver"] = driverName;

      // alias
      if(device->alias.length())
        jdev["alias"] = device->alias;

      // device slot metadata
      JsonArray jslots = jdev.createNestedArray("slots");
      for(int j=0, _j = device->slotCount(); j<_j; j++) {
        SensorReading r = (*device)[j];
        if(r) {
          JsonObject jslot = jslots.createNestedObject();

          // slot alias
          String alias = device->getSlotAlias(j);
          if(alias.length())
            jslot["alias"] = alias;

          // slot sensor type
          jslot["type"] = SensorTypeName(r.sensorType);
        }
      }
    }
  }  
}

void Devices::jsonForEachBySensorType(JsonObject& root, ReadingIterator& itr, bool detailedValues)
{
  SensorReading r;
  JsonArray groups[LastSensorType];
  //memset(groups, 0, sizeof(groups));
  
  while( (r = itr.next()) ) {
    if(r.sensorType < FirstSensorType || r.sensorType >= LastSensorType)
      continue; // guard array bounds
    
    // get the group for this sensor
    JsonArray jgroup = groups[r.sensorType];
    if(jgroup.isNull())
      jgroup = groups[r.sensorType] = root.createNestedArray(SensorTypeName(r.sensorType));
    
    // add this sensor value
    if(detailedValues) {
      JsonObject jr = jgroup.createNestedObject();
      jr["address"] = SensorAddress(itr.device->id, itr.slot).toString();
      r.toJson(jr, false);
    } else {
      r.addTo(jgroup);
    }
  }
}

#if 0
void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json)
{
  String content;
  serializeJson(json, content);
  server.send(responseCode, "application/json", content);
}


template<class T>
bool expectNumeric(ESP8266WebServer& server, const char*& p, T& n) {
  auto limits = std::numeric_limits<T>();
  return expectNumeric<T>(server, p, limits.min, limits.max, n);
}

template<class T>
bool expectNumeric(ESP8266WebServer& server, const char*& p, T _min, T _max, T& n) {
  if(!isdigit(*p)) {
    String e;
    e += "expected numeric value near ";
    e += p;
    server.send(400, "text/plain", e);
    return false;
  }

  // read the number
  int v = atoi(p);
  while(*p && isdigit(*p))
    p++;

  // check limits
  if(v < _min || v > _max) {
    String e;
    e += "value ";
    e += v;
    e += " outslide limits, expected numeric between ";
    e += _min;
    e += " and ";
    e += _max;
    server.send(400, "text/plain", e);
    return false;
  } else {
    // passed limit checks, return numeric
    n = (T)v;
    return true;
  }

  return false;
}

bool Devices::RequestHandler::expectDevice(ESP8266WebServer& server, const char*& p, Device*& dev) {
  short id;
  Device* d = &NullDevice;
  if(isdigit(*p) && expectNumeric(server, p, (short)0, owner->slots, id)) {
    // got an ID
    d = &owner->find(id);
  } else if(isalpha(*p)) {
    // possibly a device alias
    String alias;
    while(*p && *p!='/')
      alias += *p++;

    // search for alias
    d = &owner->find(alias);
  } else
    return false;

  // see if we found a valid device
  if(d != &NullDevice) {
    dev = d;
    return true;
  } else
    return false;
}



bool Devices::RequestHandler::canHandle(HTTPMethod method, String uri) { 
  return owner!=NULL && (uri.startsWith("/aliases") || uri.startsWith("/sensors") || uri.startsWith("/devices") || uri.startsWith("/device/"));
}


bool Devices::RequestHandler::handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) {
  Device* dev;
  const char* p = requestUri.c_str();
  DynamicJsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  if(strncmp(p, "/sensors", 8)==0) {
    p += 8;
    if(*p==0 || strcmp(p, "/values")==0) {
      Devices::ReadingIterator itr = DeviceManager.forEach();
      owner->jsonForEachBySensorType(root, itr, *p==0);
      goto output_doc;
    }
  } else if(strcmp(p, "/devices")==0) {
    owner->jsonGetDevices(root);
    goto output_doc;
  } else if(strncmp(p, "/device/", 8)==0) {
    p += 8;
    if(!expectDevice(server, p, dev))
      return true;  // because we sent a response

    if(*p==0) {
      dev->jsonGetReadings(root);
      goto output_doc;
    } else if(*p=='/') {
      // parse device sub-command
      p++;
      
      if(strncmp(p, "slot/", 5)==0) {
        p += 5;

        short slot;
        if(isalpha(*p)) {
          // test if alias
          String alias;
          while(*p && *p!='/')
            alias += *p++;

          // search for alias
          slot = dev->findSlotByAlias(alias);
          if(slot<0)
            return false;
        } else if(!expectNumeric<short>(server, p, 0, dev->slotCount()-1, slot))
          return false;
          
        
        if(*p==0) {
          // get dev:slot value
          dev->jsonGetReading(root, slot);
          goto output_doc;
        } else if(*p=='/') {
          // get dev:slot sub-command
          p++;

          if(strcmp(p, "alias")==0) {
            if(requestMethod == HTTP_POST) {
              // set the slot alias
              String alias = server.arg("plain");
              dev->setSlotAlias(slot, alias);
            }

            // get slot alias
            root["alias"] = dev->getSlotAlias(slot);
            goto output_doc;
          }
        }
      } else if(strcmp(p, "alias")==0) {
            if(requestMethod == HTTP_POST) {
              // set the slot alias
              String alias = server.arg("plain");
              dev->alias = alias;
            }

            // get slot alias
            root["alias"] = dev->alias;
            goto output_doc;
          }
    }
  } else if(strcmp(p, "/aliases")==0) {
    p += 8;

    if(requestMethod == HTTP_POST) {
      // read an aliases file
      String aliases = server.arg("plain");
      if(owner->parseAliasesFile(aliases.c_str()) >0)
        owner->saveAliasesFile(aliases.c_str());
    }

    // output the aliases
    String aliases = owner->getAliasesFile();
    server.send(200, "text/plain", aliases);
    return true;
  }
  return false;
output_doc:
  httpSend(server, 200, root);
  return true;  
}
#endif  // disabled old handler code


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
  nextUpdate=0;
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

void Device::jsonGetReading(JsonObject& node, short slot)
{
  if(slot >=0 && slot < slots) {
    SensorReading r = (*this)[slot];
    r.toJson(node);
  }
}

void Device::jsonGetReadings(JsonObject& node)
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

SensorReading& Device::operator[](unsigned short slotIndex)
{
  if(slotIndex >= slots)
    alloc( slotIndex+1 );
  return readings[slotIndex].reading;
}


void SensorReading::clear()
{
  valueType = VT_CLEAR;
  timestamp = millis();
  l = 0;
}

String SensorReading::toString() const {
    switch(valueType) {
      case 'i':
      case 'l': return String(l); break;
      case 'f': return String(f); break;
      case 'b': return String(b ? "true":"false"); break;
      case 'n':
      default:
        return "null"; break;
    }  
}

void SensorReading::addTo(JsonArray& arr) const
{
  switch(valueType) {
    case 'i':
    case 'l': arr.add(l); break;
    case 'f': arr.add(f); break;
    case 'b': arr.add(b); break;
    case 'n':
    default:
      arr.add((char*)NULL); break;
  }    
}

void SensorReading::toJson(JsonObject& root, bool showType) const {
  if(showType)
    root["type"] = SensorTypeName(sensorType);
  switch(valueType) {
    case 'i':
    case 'l': root["value"] = l; break;
    case 'f': root["value"] = f; break;
    case 'b': root["value"] = b; break;
    case 'n':
    default:
      root["value"] = (char*)NULL; break;
  }
}
