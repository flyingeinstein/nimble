
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


class Functional {
public:
    typedef std::function<void()> F0;
    typedef std::function<void(Rest::Endpoints<Functional>::Endpoint&)> F1;

    Functional() : args(0), f0(NULL) {}
    Functional(const F0& _f) : args(0), f0(new F0(_f)) {
    }
    Functional(const F1& _f) : args(1), f1(new F1(_f)) {
    }
    Functional(const Functional& copy) : args(copy.args) {
        switch(args) {
            case 0: f0=new F0(*copy.f0); break;
            case 1: f1=new F1(*copy.f1); break;
            default: assert(false);
        }
    }
    virtual ~Functional() {
        free();
    }

    Functional& operator=(const Functional& copy) {
        free();
        args = copy.args;
        switch(args) {
            case 0: f0=new F0(*copy.f0); break;
            case 1: f1=new F1(*copy.f1); break;
            default: assert(false);
        }
        return *this;
    }

    void operator()(Rest::Endpoints<Functional>::Endpoint& ep) {
        switch(ep.handler.args) {
            case 0: (*f0)(); break;
            case 1: (*f1)(ep); break;
            default: assert(false);
        }
    }

    void free() {
        switch(args) {
            case 0: delete f0; break;
            case 1: delete f1; break;
            default: assert(false);
        }
    }
    //inline bool operator==(const FunctionalHandler& rhs) const { return _name==rhs._name; }

    int args;
    union {
        void* f;
        F0* f0;  // non-rest handler doesnt prepare json request/response
        F1* f1;  // rest handler using Json request/response
    };
};

typedef Rest::Endpoints<Functional> FunctionalEndpoints;


using Rest::GET;
using Rest::PUT;
using Rest::POST;
using Rest::PATCH;
using Rest::DELETE;
using Rest::OPTIONS;

using Rest::HttpGet;
using Rest::HttpPut;
using Rest::HttpPost;
using Rest::HttpPatch;
using Rest::HttpDelete;
using Rest::HttpOptions;

using Rest::uri_result_to_string;


TEST(endpoints_simple)
{
    NamedEndpoints endpoints;
    NamedHandler devices("devices");

    // add some endpoints
    endpoints.on("/api/devices", GET(devices));
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
    return (r_1.status==URL_MATCHED && r_1["bus"].isString() && strcmp("default", r_1["bus"])==0)
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
        .on("/api/devices/:dev(integer|string)/slot/:slot(integer)/meta", PUT(slot))
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
    NamedEndpoints::Endpoint rb1 = endpoints.resolve(HttpGet, "/api/devices/5/slots");
    if(rb1.status!=URL_MATCHED)
        return FAIL;

    NamedEndpoints::Endpoint rb2 = endpoints.resolve(HttpGet, "/api/devices/i2c/slots");
    if(rb2.status!=URL_MATCHED)
        return FAIL;

    NamedEndpoints::Endpoint rc1 = endpoints.resolve(HttpPut, "/api/devices/i2c/slot/96/meta");
    if(rc1.status!=URL_MATCHED)
        return FAIL;
    const char* devid = rc1["dev"];
    if(!rc1["dev"].isString() || strcmp(rc1["dev"], "i2c")!=0)
        return FAIL;
    unsigned long slotid = rc1["slot"];
    if(!rc1["slot"].isInteger() || 96!=(long)rc1["slot"])
        return FAIL;

    NamedEndpoints::Endpoint r_1 = endpoints.resolve(HttpPut, "/api/bus/i2c/3/devices");
    if(r_1.status!=URL_MATCHED || !r_1["bus"].isInteger() || 3!=(long)r_1["bus"])
        return FAIL;

    return OK;
}

TEST(endpoints_int_argument_func)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num]() { func_num = 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    if (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"]) {
        r_1.handler(r_1);
        return (func_num==1) ? OK : FAIL;
    } else
        return FAIL;
}

TEST(endpoints_int_argument_func_w_1arg)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num](Rest::Endpoints<Functional>::Endpoint& ep) { func_num = (ep.name=="api/bus/i2c/<int>/devices") ? 2 : 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    if (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"]) {
        r_1.handler(r_1);
        return (func_num==2) ? OK : FAIL;
    } else
        return FAIL;
}

TEST(endpoints_default_handler_called)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num]() { func_num = 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/something/random");
    r_1.handler(r_1);
    return (func_num == 2)
            ? OK
            : FAIL;
}
