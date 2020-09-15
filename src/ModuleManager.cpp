#include "ModuleManager.h"
#include "Module.h"
#include "DefaultModuleRestHandler.h"

#include <stdlib.h>
#include <stdint.h>
#include <FS.h>

namespace Nimble {

// the main device manager
ModuleManager ModuleManager::Default;


const char* hostname = SSID_HOSTNAME;


ModuleManager::ModuleManager()
    : httpServer(NULL), restHandler(NULL)//, _moduleEndpoints( new DefaultModuleRestHandler())
{
}

void ModuleManager::begin(WebServer& _http)
{
  httpServer = &_http;

  if(restHandler == NULL) {
    restHandler = new WebServerRequestHandler();
    restHandler->endpoints = this;    // todo: handlers should take a collection of delegates
    httpServer->addHandler(restHandler);
  }

  _modules.begin();
}

void ModuleManager::handleUpdate()
{
    _modules.handleUpdate();
}

Rest::Endpoint ModuleManager::delegate(Rest::Endpoint &p)
{  
  if(auto api = p / "api") {
    // accept any Uri that starts with api
    if(api.request().accept())
      return api;

    String s;
    auto echo = api / "echo" / &s / Rest::GET([&s](RestRequest& req) {
      req.response["reply"] = s;
      return 200;
    });

    // module by ID
    if(auto device = api / "device") {
      int devid = -1;
      String devname;
      Module* mod;
      Endpoint module;
      if(module = device / &devid) {
        if(devid >= 0) {
          // get module by ID
          mod = &_modules[ devid ];
          if (mod == &NullModule)
            mod = nullptr;
        } else
          module.accept(Rest::NotFound);
      } else if(module = device / &devname) {
        if(!devname.isEmpty()) {
          // get module by name
          mod = &_modules[ devname ];
          if (mod == &NullModule)
            mod = nullptr;
        } else
          module.accept(Rest::NotFound);
      }

      if(mod) {
        // delegate to the module, or if unhandles we can return some driver info
        auto mod_result = module / *mod;
        if (mod_result.status <= Rest::NoHandler) {
          module / Rest::GET( [&mod](RestRequest& req) {
            req.response["driver"] = mod->getDriverName();
            req.response["alias"] = mod->getAlias();
            return 200;
          });
        }
      }
    } else if(auto devices = api / "devices") {
      // todo: handle API calls for all devices, like config, export, etc
      devices / Rest::GET([this](RestRequest& req) {
        auto mods = req.response.createNestedArray("modules");
        _modules.forEach([this, &req, &mods](const Module& mod, void*) {
          auto jmod = mods.createNestedObject();
          jmod["id"] = mod.id;
          mod.toJson(jmod, Module::JsonMinimum);
          return true;
        });
        return 200;
      });
    }



    // config API calls
    if(auto config = api / "config" ) {
      config / "aliases"
        / Rest::GET([this](RestRequest& request) {
            // retrieve the alias file
            String aliases = getAliasesFile();
            request.server.send(200, "text/plain", aliases);
            return HTTP_RESPONSE_SENT;
          })
        / Rest::POST([this](RestRequest& request) {
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

    return echo;
  }
  return {};
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


} // ns:Nimble
