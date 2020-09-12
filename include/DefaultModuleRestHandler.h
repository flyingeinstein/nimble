
#pragma once

#include "ModuleManager.h"

namespace Nimble {

class DefaultModuleRestHandler : public Endpoint::Delegate {
public:
    // maybe constructor should take a module collection and Endpoint attachment point?
    DefaultModuleRestHandler();

    Endpoint delegate(Endpoint& parent) override;
  
    // json interface
    void jsonGetModules(ModuleSet& modset, JsonObject &root);
    void jsonForEachBySensorType(JsonObject& root, ModuleSet::ReadingIterator& itr, bool detailedValues=true);
};


} // ns:Nimble
