#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"
#include "ModuleFactory.h"
#include "ModuleSet.h"
#include "Logger.h"

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


namespace Nimble {

class EndpointsInterface;


class ModuleManager : public ModuleFactory, public Rest::Endpoint::Delegate
{
public:
    using WebServerRequestHandler = Rest::Platforms::Default::WebServerRequestHandler;

    ModuleManager();

    void begin(WebServer& _http);

    void handleUpdate();

    // generate a file of all device and slot aliases
    String getAliasesFile();

    // parse a file of device and/or slot aliases and set where possible
    int parseAliasesFile(const char* aliases);

    // save alias file to SPIFFS fs
    bool saveAliasesFile(const char* aliases);

    // load aliases file from SPIFFS fs
    int restoreAliasesFile();

    // send a message to the logger
    inline Logger& logger() { return _logger; }

    ModuleSet& modules() { return _modules; }
    const ModuleSet& modules() const { return _modules; }

    // Web interface
    inline const WebServer& http() const { return *httpServer; }
    inline WebServer& http() { return *httpServer; }
    
    // Rest interface
    inline const WebServerRequestHandler& restServer() const { return *restHandler; }
    inline WebServerRequestHandler& restServer() { return *restHandler; }

    /// @brief Handles Rest API requests on this module
    Rest::Endpoint delegate(Rest::Endpoint &p) override;

protected:
    ModuleSet _modules;
    Logger _logger;
    WebServer* httpServer;
    WebServerRequestHandler* restHandler;

public:
    /// @brief Should be the one and only module manager
    static ModuleManager Default;
};


} // ns:Nimble