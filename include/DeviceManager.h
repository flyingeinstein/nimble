/**
 * @file DeviceManager.h
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

#include <NTPClient.h>

#include <WiFiUdp.h>
#include <NTPClient.h>

#include "NimbleConfig.h"
#include "SensorReading.h"


class Devices;
class Device;
class SensorReading;


class Devices {
  public:
    using WebServer = ESP8266WebServer;
    
    //typedef Esp8266RestRequestHandler RestRequestHandler;
    //typedef Esp8266RestRequest RestRequest;  // RestRequestHandler::RequestType RestRequest;
    //typedef typename RestRequestHandler::HandlerType HandlerType;
    using Endpoints = typename RestRequestHandler::Endpoints;
    
    short slots;
    Device** devices;
    
    class ReadingIterator
    {
      public:
        SensorType sensorTypeFilter;
        char valueTypeFilter;
        unsigned long tsFrom, tsTo;
    
        Device* device;
        unsigned short slot;

        ReadingIterator& OfType(SensorType st);
        
        // returns an iterator that matches only specific time ranges
        ReadingIterator& TimeBetween(unsigned long from, unsigned long to);
        ReadingIterator& Before(unsigned long ts);
        ReadingIterator& After(unsigned long ts);
    
        SensorReading next();
        
      protected:
        Devices* manager;
        bool singleDevice;
        short deviceOrdinal;

        ReadingIterator(Devices* manager);

        // web handlers
        void httpGetDevices();
        
      friend class Devices;
      friend class DevicesRequestHandler;
    };

  protected:  
    /*class RequestHandler : public ::RequestHandler {
      public:
        RequestHandler(Devices* _owner) : owner(_owner) {}
        virtual bool canHandle(HTTPMethod method, String uri);
        virtual bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri);

        bool expectDevice(ESP8266WebServer& server, const char*& p, Device*& dev);
      private:
        Devices* owner;
    };*/
    
public:
    Devices(short maxDevices=32);
    ~Devices();

    void begin(WebServer& _http, NTPClient& client);

    // clear all devices
    void clearAll();

    short add(Device& dev);
    
    void remove(short deviceId);
    void remove(Device& dev);

    // find a device by id
    const Device& find(short deviceId) const;
    Device& find(short deviceId);

    // find a device by its alias
    const Device& find(String deviceAlias) const;
    Device& find(String deviceAlias);

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
    void jsonGetDevices(JsonObject& root);
    void jsonForEachBySensorType(JsonObject& root, ReadingIterator& itr, bool detailedValues=true);

    static void registerDriver(const DeviceDriverInfo* driver);
    
  protected:
    short update_iterator;  // ordinal of next device update
    NTPClient* ntp;
    WebServer* httpServer;
    RestRequestHandler* restHandler;
    
    void alloc(short n);

    // do not allow copying
    Devices(const Devices& copy) = delete;
    Devices& operator=(const Devices& copy) = delete;

  protected:
    static const DeviceDriverInfo** drivers;
    static short driversSize;
    static short driversCount;

    void setupRestHandler();
    static const DeviceDriverInfo* findDriver(const char* name);
};

extern Devices DeviceManager;

void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json);
