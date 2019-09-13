
#pragma once

#include "ModuleManager.h"

namespace Nimble {

class DefaultModuleRestHandler : public EndpointsInterface {
public:
    // maybe constructor should take a module collection and Endpoint attachment point?
    DefaultModuleRestHandler();

    virtual bool hasEndpoints() const;
    virtual const Endpoints* endpoints() const;
    virtual Endpoints* endpoints();

    inline Endpoints::Node on(const char *endpoint_expression ) {
      return _endpoints.on(endpoint_expression);
    }

    // json interface
    void jsonGetModules(ModuleSet& modset, JsonObject &root);
    void jsonForEachBySensorType(JsonObject& root, ModuleSet::ReadingIterator& itr, bool detailedValues=true);

protected:
    // created only when requested, either by const or non-const
    Endpoints _endpoints, _defaultModuleEndpoints;
};


} // ns:Nimble
