
#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"

/// @todo Create influx module
typedef struct _InfluxTarget {
    String database;
    String measurement;
} InfluxTarget;

/// @todo I think SensorInfo can be deprecated now that modules are more generic, possibly just need Module generic config stuff (and maybe keep as json)
typedef struct _SensorInfo {
    String channelName;
    String driver;
    // influx target?
    uint8_t pin;  //todo: should be pinmap
    unsigned long updateFrequency;
} SensorInfo;


typedef Module* (*ModuleFactoryFunction)(SensorInfo* info);

typedef struct _ModuleInfo {
    const char* name;
    const char* category;
    ModuleFactoryFunction factory;
} ModuleInfo;



class ModuleFactory
{
public:
    ModuleFactory();

    void registerDriver(const ModuleInfo* driver);
    const ModuleInfo* findDriver(const char* name);

protected:
    /// @todo use paged set here?
    const ModuleInfo** drivers;
    short driversSize;
    short driversCount;
};

/// @brief This should be the one and only module factory
extern ModuleFactory DefaultModuleFactory;