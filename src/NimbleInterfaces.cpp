
#include "NimbleInterfaces.h"

namespace Nimble {

bool DefaultEndpointsInterface::hasEndpoints() const {
    return _endpoints != nullptr; 
}

const Endpoints* DefaultEndpointsInterface::endpoints() const 
{
   return _endpoints; 
}

Endpoints* DefaultEndpointsInterface::endpoints() 
{
   return (_endpoints == nullptr)
    ? _endpoints = new Endpoints()
    : _endpoints;
}

} // ns: Nimble