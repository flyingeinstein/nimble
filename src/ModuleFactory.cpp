
#include "ModuleFactory.h"

namespace Nimble {

ModuleFactory::ModuleFactory()
    : drivers(nullptr), driversSize(0), driversCount(0)
{
}

void ModuleFactory::registerDriver(const ModuleInfo* driver)
{
    if(drivers == NULL) {
        // first init
        drivers = (const ModuleInfo**)calloc(driversSize=16, sizeof(ModuleInfo*));
    }
    drivers[driversCount++] = driver;
}

const ModuleInfo* ModuleFactory::findDriver(const char* name)
{
    // todo: split name into category if exists
    for(short i=0; i<driversCount; i++) {
        if(strcmp(name, drivers[i]->name)==0)
            return drivers[i];
    }
    return NULL;
}

} // ns:Nimble
