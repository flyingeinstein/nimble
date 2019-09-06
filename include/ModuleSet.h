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
#include "Module.h"

namespace Nimble {

class Module;
class SensorReading;

/**
 * @brief Manages a collection of sensor or other devices
 * This class is the root device manager but can also hold a collection of sub devices.
 * 
 */
class ModuleSet : public Module {
  public:
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
    ModuleSet(short id=0, short maxModules=32);
    virtual ~ModuleSet();

    void begin();

    // clear all devices
    void clearAll();

    short add(Module& dev);
    
    void remove(short deviceId);
    void remove(Module& dev);

    // find a device by id
    const Module& operator[](short moduleId) const;
    Module& operator[](short moduleId);

    // find a device by its alias
    const Module& operator[](String alias) const;
    Module& operator[](String alias);

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

protected:
    short update_iterator;  // ordinal of next device update
    
    void alloc(short n);

    // do not allow copying
    ModuleSet(const ModuleSet& copy) = delete;
    ModuleSet& operator=(const ModuleSet& copy) = delete;
};

void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json);

} // ns:Nimble
