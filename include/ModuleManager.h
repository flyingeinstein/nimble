#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"
#include "ModuleFactory.h"
#include "ModuleSet.h"

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


namespace Nimble {

class ModuleManager : public ModuleFactory
{
public:
    ModuleManager();

    void begin(WebServer& _http, NTPClient& client);

    void handleUpdate();

    // generate a file of all device and slot aliases
    String getAliasesFile();

    // parse a file of device and/or slot aliases and set where possible
    int parseAliasesFile(const char* aliases);

    // save alias file to SPIFFS fs
    bool saveAliasesFile(const char* aliases);

    // load aliases file from SPIFFS fs
    int restoreAliasesFile();

    // add our default rest handler to the http web server
    void setupRestHandler();

    // json interface
    void jsonGetModules(ModuleSet& modset, JsonObject &root);
    void jsonForEachBySensorType(JsonObject& root, ModuleSet::ReadingIterator& itr, bool detailedValues=true);

    ModuleSet& modules() { return _modules; }
    const ModuleSet& modules() const { return _modules; }

    // Web interface
    inline const WebServer& http() const { return *httpServer; }
    inline WebServer& http() { return *httpServer; }
    
    // Rest interface
    inline const RestRequestHandler& rest() const { return *restHandler; }
    inline RestRequestHandler& rest() { return *restHandler; }

    Endpoints::Node on(const char *endpoint_expression ) {
      return restHandler->on(endpoint_expression);   // add the rest (recursively)
    }

protected:
    ModuleSet _modules;
    NTPClient* ntp;
    WebServer* httpServer;
    RestRequestHandler* restHandler;

public:
    /// @brief Should be the one and only module manager
    static ModuleManager Default;
};


} // ns:Nimble