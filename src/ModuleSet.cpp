
// include core Wiring API
#include <stdlib.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <FS.h>


#include "ModuleSet.h"
#include "Module.h"


// the main device manager
ModuleSet ModuleManager;

ModuleSet::ModuleSet(short maxModules)
  : slots(maxModules), devices(NULL), update_iterator(0), ntp(NULL), httpServer(NULL), restHandler(NULL) {
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

void ModuleSet::begin(WebServer& _http, NTPClient& _ntp)
{
  httpServer = &_http;
  ntp = &_ntp;

  if(restHandler == NULL) {
    restHandler = new RestRequestHandler();
    setupRestHandler();
    httpServer->addHandler(restHandler);
  }
}

void ModuleSet::setupRestHandler()
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

  // resolves a device number to a Module object
  std::function<Module*(Rest::UriRequest&)> device_resolver = [this](Rest::UriRequest& request) -> Module* {
    Rest::Argument req_dev = request["xxx"];
    Module& dev = (req_dev.isInteger())
          ? find( (long)req_dev )
          : (req_dev.isString())
            ? find( (const char*)req_dev )
            : NullModule;

    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  std::function<const Module*(Rest::UriRequest&)> const_device_resolver = [this](Rest::UriRequest& request) -> const Module* {
    Rest::Argument req_dev = request["xxx"];
    Module& dev = (req_dev.isInteger())
          ? find( (long)req_dev )
          : (req_dev.isString())
            ? find( (const char*)req_dev )
            : NullModule;

    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  auto device_api_resolver = [this](Rest::ParserState& lhs_request) -> Endpoints::Handler {
    Rest::Argument req_dev = lhs_request.request.args["id"];
    Module& dev = (req_dev.isInteger())
          ? find( (long)req_dev )
          : (req_dev.isString())
            ? find( (const char*)req_dev )
            : NullModule;
    
    if (&dev != &NullModule) {
      // device found, see of the device has an API extension
      typename Endpoints::Node rhs_node;

      if(dev._endpoints != nullptr) {
        // try to resolve the endpoint using the device's API extension
        rhs_node = dev._endpoints->getRoot();
      } else {
        // device has no API extension, so use the default one
        // todo: setup the device default API controller
      }

      if(rhs_node) {
        Rest::ParserState rhs_request(lhs_request);
        typename Endpoints::Handler handler = rhs_node.resolve(rhs_request);
        if (handler!=nullptr)
          return handler;   
      }
    }

    // device or handler not found
    return Endpoints::Handler();
  };

  // Playground
  on("/api/echo/:msg(string|integer)").GET( func );
  
  // ModuleSet API
  on("/api/devices")
    .GET([this](RestRequest& request) { jsonGetModules(request.response); return 200; });
  on("/api/dev/:xxx(string|integer)")
    .with(const_device_resolver)
    .GET(&Module::restDetail)
    .GET("status", &Module::restStatus)
    .GET("slots", &Module::restSlots)
    .GET("statistics", &Module::restStatistics);
  
  // delegate device API requests to the Module or the default device API controller
  on("/api/device/:id(string|integer)")
    .otherwise(device_api_resolver);

  // Config API
  on("/api/config/aliases")
    .GET([this](RestRequest& request) {
      // retrieve the alias file
      String aliases = getAliasesFile();
      request.server.send(200, "text/plain", aliases);
      return HTTP_RESPONSE_SENT;
    })
    .POST([this](RestRequest& request) {
      // write an aliases file
      String aliases = request.server.arg("plain");
      if(parseAliasesFile(aliases.c_str()) >0)
        saveAliasesFile(aliases.c_str());

      // re-read the aliase file back
      aliases = getAliasesFile();
      request.server.send(200, "text/plain", aliases);
      return HTTP_RESPONSE_SENT;
    });
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

String ModuleSet::getAliasesFile()
{
  String out;
  ReadingIterator itr = forEach();
  SensorReading r;
  Module* lastDev = NULL;
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

int ModuleSet::parseAliasesFile(const char* aliases)
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
      Module& dev = find(devid);
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

int ModuleSet::restoreAliasesFile() {
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

bool ModuleSet::saveAliasesFile(const char* aliases)
{
  File f = SPIFFS.open("/aliases.txt", "w");
  if(f) {
      f.print(aliases);
      f.close();
      return true;
  }
  return false;
}

void ModuleSet::jsonGetModules(JsonObject &root)
{
  // list all devices
  JsonArray devs = root.createNestedArray("devices");
  for(short i=0; i < slots; i++) {
    if(devices[i]) {
      // get the slot
      Module* device = devices[i];
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

void ModuleSet::jsonForEachBySensorType(JsonObject& root, ReadingIterator& itr, bool detailedValues)
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


