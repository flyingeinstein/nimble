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

    /// @brief Add a module to this module set.
    /// This will also set the new owner in the sub-module.
    short add(Module& dev);
    
    /// @brief Remove a sub-module from the collection by device ID.
    void remove(short deviceId);

    /// @brief Remove a sub-module from the collection.
    void remove(Module& dev);

    /// @brief find a device by id
    const Module& operator[](short moduleID) const;
    Module& operator[](short moduleID);

    /// @brief find a device by its alias
    const Module& operator[](String alias) const;
    Module& operator[](String alias);

    /// @brief find a reading by device:slot address.
    /// for convenience, but if you are reading multiple values you should get the device ptr then read the slots (readings)
    SensorReading getReading(short deviceId, unsigned short slotId) const;
    SensorReading getReading(const SensorAddress& sa) const;

    /// @brief Update sub-modules
    /// Internally this method determines which modules are scheduled to run now. For devices the schedule is typically synced
    /// to when the driver needs to interact with hardware.
    /// Internally the update only updates one module at a time, so this method should be called often. The default Nimble runtime
    /// calls this method every loop() iteration but since it only updates 1 device max there is plenty of time left for WiFi
    /// and other core OS services to run.
    void handleUpdate();

    // iterate every reading available
    ReadingIterator forEach();

    // iterator readings from a deviceId
    ReadingIterator forEach(short deviceId);

    // get an iterator over a type of reading
    ReadingIterator forEach(SensorType st);
    
    // also allow any calls to the Module::forEach methods
    using Module::forEach;

protected:
    short update_iterator;  // ordinal of next device update
    
    // do not allow copying
    ModuleSet(const ModuleSet& copy) = delete;
    ModuleSet& operator=(const ModuleSet& copy) = delete;
};

void httpSend(ESP8266WebServer& server, short responseCode, const JsonObject& json);

} // ns:Nimble
