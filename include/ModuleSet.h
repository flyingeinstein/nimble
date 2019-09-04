/**
 * @file ModuleSet.h
 * @author Colin F. MacKenzie (nospam2@colinmackenzie.net)
 * @brief Manages a hierarchy of attached devices
 * @version 0.1
 * @date 2019-08-27
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"

#include <NTPClient.h>

#include <WiFiUdp.h>
#include <NTPClient.h>

#include "NimbleConfig.h"
#include "SensorReading.h"


class ModuleSet;
class Module;
class SensorReading;

/**
 * @brief Manages a collection of sensor or other devices
 * This class is the root device manager but can also hold a collection of sub devices.
 * 
 */
class ModuleSet {
  public:
    using WebServer = ESP8266WebServer;
    
    //typedef Esp8266RestRequestHandler RestRequestHandler;
    //typedef Esp8266RestRequest RestRequest;  // RestRequestHandler::RequestType RestRequest;
    //typedef typename RestRequestHandler::HandlerType HandlerType;
    using Endpoints = typename RestRequestHandler::Endpoints;
    
    short slots;
    Module** devices;
    
    class ReadingIterator
    {
      public:
        SensorType sensorTypeFilter;
        char valueTypeFilter;
        unsigned long tsFrom, tsTo;
    
        Module* device;
        unsigned short slot;

        ReadingIterator& OfType(SensorType st);
        
        // returns an iterator that matches only specific time ranges
        ReadingIterator& TimeBetween(unsigned long from, unsigned long to);
        ReadingIterator& Before(unsigned long ts);
        ReadingIterator& After(unsigned long ts);
    
        SensorReading next();
        
      protected:
        ModuleSet* manager;
        bool singleModule;
        short deviceOrdinal;

        ReadingIterator(ModuleSet* manager);

        // web handlers
        void httpGetModules();
        
      friend class ModuleSet;
    };

public:
    ModuleSet(short maxModules=32);
    ~ModuleSet();

    void begin(WebServer& _http, NTPClient& client);

    // clear all devices
    void clearAll();

    short add(Module& dev);
    
    void remove(short deviceId);
    void remove(Module& dev);

    // find a device by id
    const Module& find(short deviceId) const;
    Module& find(short deviceId);

    // find a device by its alias
    const Module& find(String deviceAlias) const;
    Module& find(String deviceAlias);

    // find a reading by device:slot
    // for convenience, but if you are reading multiple values you should get the device ptr then read the slots (readings)
    SensorReading getReading(short deviceId, unsigned short slotId) const;
    SensorReading getReading(const SensorAddress& sa) const;

    // determine which devices need to interact with their hardware
    void handleUpdate();

    // iterate every reading available
    ReadingIterator forEach();

    // iterator readings from a deviceId
    ReadingIterator forEach(short deviceId);

    // get an iterator over a type of reading
    ReadingIterator forEach(SensorType st);

    // generate a file of all device and slot aliases
    String getAliasesFile();

    // parse a file of device and/or slot aliases and set where possible
    int parseAliasesFile(const char* aliases);

    // save alias file to SPIFFS fs
    bool saveAliasesFile(const char* aliases);

    // load aliases file from SPIFFS fs
    int restoreAliasesFile();

    // Web interface
    inline const WebServer& http() const { return *httpServer; }
    inline WebServer& http() { return *httpServer; }
    
    // Rest interface
    inline const RestRequestHandler& rest() const { return *restHandler; }
    inline RestRequestHandler& rest() { return *restHandler; }

    Endpoints::Node on(const char *endpoint_expression ) {
      return restHandler->on(endpoint_expression);   // add the rest (recursively)
    }

    // json interface
    void jsonGetModules(JsonObject &root);
    void jsonForEachBySensorType(JsonObject& root, ReadingIterator& itr, bool detailedValues=true);

protected:
    short update_iterator;  // ordinal of next device update
    NTPClient* ntp;
    WebServer* httpServer;
    RestRequestHandler* restHandler;
    
    void alloc(short n);

    // do not allow copying
    ModuleSet(const ModuleSet& copy) = delete;
    ModuleSet& operator=(const ModuleSet& copy) = delete;

  protected:

    void setupRestHandler();
};

extern ModuleSet ModuleManager;

void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json);
