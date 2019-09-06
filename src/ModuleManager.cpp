#include "ModuleManager.h"
#include "Module.h"

#include <stdlib.h>
#include <stdint.h>
#include <FS.h>

namespace Nimble {

// the main device manager
ModuleManager ModuleManager::Default;


ModuleManager::ModuleManager()
    : ntp(NULL), httpServer(NULL), restHandler(NULL)
{
}

void ModuleManager::begin(WebServer& _http, NTPClient& _ntp)
{
  httpServer = &_http;
  ntp = &_ntp;

  if(restHandler == NULL) {
    restHandler = new RestRequestHandler();
    setupRestHandler();
    httpServer->addHandler(restHandler);
  }

  _modules.begin();
}

void ModuleManager::handleUpdate()
{
    _modules.handleUpdate();
}

void ModuleManager::setupRestHandler()
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
    Module& dev = _modules[ req_dev ];

    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  std::function<const Module*(Rest::UriRequest&)> const_device_resolver = [this](Rest::UriRequest& request) -> const Module* {
    Rest::Argument req_dev = request["xxx"];
    Module& dev = _modules[ req_dev ];
         
    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  auto device_api_resolver = [this](Rest::ParserState& lhs_request) -> Endpoints::Handler {
    Rest::Argument req_dev = lhs_request.request.args["id"];
    #if 0
    Module& dev = (req_dev.isInteger())
          ? _modules[ (long)req_dev ]
          : (req_dev.isString())
            ? _modules[ (String)req_dev ]
            : NullModule;
    #else
    Module& dev = _modules[ req_dev ];
    #endif

    if (&dev != &NullModule) {
      // device found, see of the device has an API extension
      typename Endpoints::Node rhs_node;

      if(dev.hasEndpoints()) {
        // try to resolve the endpoint using the device's API extension
        rhs_node = dev.endpoints()->getRoot();
      } else {
        // device has no API extension, so use the default one
        // todo: setup the device default API controller
      }

      if(rhs_node) {
        Rest::ParserState rhs_request(lhs_request);
        typename Endpoints::Handler handler = rhs_node.resolve(rhs_request);
        if (handler!=nullptr) {
            lhs_request = rhs_request;      // we resolved a handler, so we modify the lhs request with rhs (which may have extra args)
            return handler;   
        }
      }
    }

    // device or handler not found
    return Endpoints::Handler();
  };

  // Playground
  on("/api/echo/:msg(string|integer)").GET( func );
  
  // ModuleSet API
  on("/api/devices")
    .GET([this](RestRequest& request) { jsonGetModules(_modules, request.response); return 200; });
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


String ModuleManager::getAliasesFile()
{
  String out;
  ModuleSet::ReadingIterator itr = _modules.forEach();
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

int ModuleManager::parseAliasesFile(const char* aliases)
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
      Module& dev = _modules[devid];
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

int ModuleManager::restoreAliasesFile() {
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

bool ModuleManager::saveAliasesFile(const char* aliases)
{
  File f = SPIFFS.open("/aliases.txt", "w");
  if(f) {
      f.print(aliases);
      f.close();
      return true;
  }
  return false;
}

void ModuleManager::jsonGetModules(ModuleSet& modset, JsonObject &root)
{
  // list all devices
  JsonArray devs = root.createNestedArray("devices");
  for(short i=0; i < modset.slots; i++) {
    if(modset.readings[i].reading.module != nullptr) {
      // get the slot
      Module* device = modset.readings[i].reading.module;
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

void ModuleManager::jsonForEachBySensorType(JsonObject& root, ModuleSet::ReadingIterator& itr, bool detailedValues)
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


} // ns:Nimble
