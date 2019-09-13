
#pragma once

#include "NimbleConfig.h"

namespace Nimble {

class EndpointsInterface {
public:
    virtual bool hasEndpoints() const=0;
    virtual const Endpoints* endpoints() const=0;
    virtual Endpoints* endpoints()=0;
};

class DefaultEndpointsInterface : public EndpointsInterface {
public:
    inline DefaultEndpointsInterface() : _endpoints(nullptr) {}

    virtual bool hasEndpoints() const;
    virtual const Endpoints* endpoints() const;
    virtual Endpoints* endpoints();

protected:
    /// @brief Contains endpoints for this device only.
    /// By default this member will be null and is only created when a derived device requests 
    /// creation of API endpoints using the on(...) method.
    Endpoints* _endpoints;
};

} // ns:Nimble
