
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "../../Rest.h"

/// \brief Links a http method verb with a Rest callback handler
/// Associates a handler callback with a http method request. Many of these associations can
/// be attached to a single Endpoint Uri.
class NamedHandler {
public:
    std::string _name;

    NamedHandler() {}
    NamedHandler(const char* __name) : _name(__name) {}

    inline bool operator==(const NamedHandler& rhs) const { return _name==rhs._name; }
};

typedef Rest::Endpoints<NamedHandler> NamedEndpoints;

using Rest::GET;
using Rest::POST;
using Rest::PUT;
using Rest::PATCH;
using Rest::DELETE;
using Rest::OPTIONS;
using Rest::ANY;

using Rest::uri_result_to_string;


TEST(endpoints_simple)
{
    NamedEndpoints endpoints;
    NamedHandler devices("devices");

    // add some endpoints
    endpoints.on("/api/devices", Rest::GET(devices));
    NamedEndpoints::Endpoint res = endpoints.resolve(HttpGet, "/api/devices");
    return res.method==HttpGet && res.handler==devices && res.status==URL_MATCHED;
}

TEST(endpoints_partial_match_returns_no_handler) {
    NamedEndpoints endpoints;
    NamedHandler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    NamedEndpoints::Endpoint r = endpoints.resolve(HttpGet, "/api/bus/i2c");
    return (r.status==URL_FAIL_NO_HANDLER)
        ? OK
        : FAIL;
}

TEST(endpoints_int_argument)
{
    NamedEndpoints endpoints;
    NamedEndpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    NamedEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"])
       ? OK
       : FAIL;
}

TEST(endpoints_real_argument)
{
    NamedEndpoints endpoints;
    NamedEndpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(real)/devices", GET(getbus));
    NamedEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3.14/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isNumber() && 3.14==(double)r_1["bus"])
        ? OK
        : FAIL;
}

TEST(endpoints_string_argument)
{
    NamedEndpoints endpoints;
    NamedEndpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(string)/devices", GET(getbus));
    NamedEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/default/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isString() && strcmp("default", (const char*)r_1["bus"])==0)
        ? OK
        : FAIL;
}

TEST(endpoints_many)
{
	NamedEndpoints endpoints;
	NamedEndpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");

    // add some endpoints
	endpoints
	    .on("/api/devices", GET(devices))
	    .on("/api/devices/:dev(integer|string)/slots", GET(slots))
        .on("/api/devices/:dev(integer|string)/slot/:slot(integer)/meta", PUT(slot));
	endpoints
        .on("/api/bus/i2c/:bus(integer)/devices",
                  GET(getbus),
                  PUT(putbus),
                  PATCH(putbus),
                  POST(putbus),
                  DELETE(putbus),
                  //GET(putbus),         // will cause a duplicate endpoint error
                  OPTIONS(putbus)
                  )
        // any errors produced in the above sentences will get caught here
        .katch([](NamedEndpoints::Endpoint p) {
            std::cout << "exception occured adding endpoints: "
                << uri_result_to_string(p.status) << ": "
                << p.name;
            return FAIL;
        });

    // resolve some endpoints
    NamedEndpoints::Endpoint rb1 = endpoints.resolve(Rest::HttpGet, "/api/devices/5/slots");
    if(rb1.status!=URL_MATCHED)
        return FAIL;

    NamedEndpoints::Endpoint rb2 = endpoints.resolve(Rest::HttpGet, "/api/devices/i2c/slots");
    if(rb2.status!=URL_MATCHED)
        return FAIL;

    NamedEndpoints::Endpoint rc1 = endpoints.resolve(Rest::HttpPut, "/api/devices/i2c/slot/96/meta");
    if(rc1.status!=URL_MATCHED)
        return FAIL;
    const char* devid = rc1["dev"];
    if(!rc1["dev"].isString() || strcmp(rc1["dev"], "i2c")!=0)
        return FAIL;
    unsigned long slotid = rc1["slot"];
    if(!rc1["slot"].isInteger() || 96!=(long)rc1["slot"])
        return FAIL;

    NamedEndpoints::Endpoint r_1 = endpoints.resolve(Rest::HttpPut, "/api/bus/i2c/3/devices");
    if(r_1.status!=URL_MATCHED || !r_1["bus"].isInteger() || 3!=(long)r_1["bus"])
        return FAIL;

    return OK;
}
