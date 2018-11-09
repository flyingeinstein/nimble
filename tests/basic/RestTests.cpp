
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <tests.h>
#include "../../Rest.h"

using namespace Rest;

TEST(endpoints_simple)
{
    Endpoints endpoints(100);
    Endpoints::Handler devices("devices");

    // add some endpoints
    endpoints.on("/api/devices", GET(devices));
    Endpoints::Endpoint res = endpoints.resolve(HttpGet, "/api/devices");
    return res.method==HttpGet && res.handler==devices && res.status==URL_MATCHED;
}

TEST(endpoints_partial_match_returns_no_handler) {
    Endpoints endpoints(100);
    Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    Endpoints::Endpoint r = endpoints.resolve(HttpGet, "/api/bus/i2c");
    return r.status==URL_FAIL_NO_HANDLER;
}

TEST(endpoints_int_argument)
{
    Endpoints endpoints(100);
    Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"])
       ? OK
       : FAIL;
}

TEST(endpoints_real_argument)
{
    Endpoints endpoints(100);
    Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(real)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3.14/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isNumber() && 3.14==(double)r_1["bus"])
        ? OK
        : FAIL;
}

TEST(endpoints_string_argument)
{
    Endpoints endpoints(100);
    Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(string)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/default/devices");
    return (r_1.status==URL_MATCHED && r_1["bus"].isString() && strcmp("default", r_1["bus"])==0)
        ? OK
        : FAIL;
}

TEST(endpoints_many)
{
	Endpoints endpoints(100);
	Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");

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
        .katch([](Endpoints::Endpoint p) {
            std::cout << "exception occured adding endpoints: "
                << uri_result_to_string(p.status) << ": "
                << p.name;
            return FAIL;
        });

    // resolve some endpoints
    Endpoints::Endpoint rb1 = endpoints.resolve(HttpGet, "/api/devices/5/slots");
    if(rb1.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Endpoint rb2 = endpoints.resolve(HttpGet, "/api/devices/i2c/slots");
    if(rb2.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Endpoint rc1 = endpoints.resolve(HttpPut, "/api/devices/i2c/slot/96/meta");
    if(rc1.status!=URL_MATCHED)
        return FAIL;
    const char* devid = rc1["dev"];
    if(!rc1["dev"].isString() || strcmp(rc1["dev"], "i2c")!=0)
        return FAIL;
    unsigned long slotid = rc1["slot"];
    if(!rc1["slot"].isInteger() || 96!=(long)rc1["slot"])
        return FAIL;

    Endpoints::Endpoint r_1 = endpoints.resolve(HttpPut, "/api/bus/i2c/3/devices");
    if(r_1.status!=URL_MATCHED || !r_1["bus"].isInteger() || 3!=(long)r_1["bus"])
        return FAIL;

    return OK;
}
