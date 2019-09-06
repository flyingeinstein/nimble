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
class ModuleManager;
class SensorReading;

typedef bool (*SlotCallback)(Module::Slot& slot, void* pUserData);
typedef bool (*ReadingCallback)(SensorReading& reading, void* pUserData);
#if 0
typedef bool (*ModuleCallback)(Module& module, void* pUserData);
#else
using ModuleCallback = std::function< bool(Module&, void*) >;
#endif


/**
 * @brief Manages a collection of sensor or other devices
 * This class is the root device manager but can also hold a collection of sub devices.
 * 
 */
class ModuleSet : public Module {
  friend class ModuleManager;   // @todo we are all friends, but golly we shouldn't just sleep with everbody.

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

    void reset();

    // clear all devices
    void clear();

    short add(Module& dev);
    
    void remove(short deviceId);
    void remove(Module& dev);

#if 0
    // find a device by id
    const Module& find(short deviceId) const;
    Module& find(short deviceId);

    // find a device by its alias
    const Module& find(String deviceAlias) const;
    Module& find(String deviceAlias);

    // find a device by its alias
    const Module& find(const Rest::Argument& deviceAliasOrId) const;
    Module& find(const Rest::Argument& deviceAliasOrId);
#else
    /// find a device by id
    const Module& operator[](short moduleID) const;
    Module& operator[](short moduleID);

    /// find a device by its alias
    const Module& operator[](String alias) const;
    Module& operator[](String alias);

    /// @brief find a device from a Rest argument
    /// This operator will determine if the argument is integer or string, and call the 
    /// appropriate operator to find the module.
    const Module& operator[](const Rest::Argument& aliasOrId) const;
    Module& operator[](const Rest::Argument& aliasOrId);
#endif

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

    void forEach(SlotCallback cb, void* pUserData, SensorType st = AnySensorType);
    void forEach(ReadingCallback cb, void* pUserData, SensorType st = AnySensorType);
    void forEach(ModuleCallback cb, void* pUserData = nullptr);

protected:
    short update_iterator;  // ordinal of next device update
    
    void alloc(short n);

    // do not allow copying
    ModuleSet(const ModuleSet& copy) = delete;
    ModuleSet& operator=(const ModuleSet& copy) = delete;
};

void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json);

} // ns:Nimble
