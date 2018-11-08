
#include <stdio.h>
#include <stdlib.h>
#include "Rest.h"


int main(int argc, const char* argv[])
{
	Endpoints endpoints(100);
	Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), bus("i2c-bus");

    // add some endpoints
	Endpoints::Endpoint a = endpoints.add(HttpGet, "/api/devices", devices);
	Endpoints::Endpoint b = endpoints.add(HttpGet, "/api/devices/:dev(integer|string)/slots", slots);
    Endpoints::Endpoint c = endpoints.add(HttpPut, "/api/devices/:dev(integer|string)/slot/:slot(integer)/meta", slot);
    endpoints.add("/api/bus/i2c/:bus(integer)/devices",
                  GET<Endpoints::Handler>(bus),
                  PUT<Endpoints::Handler>(bus)
    );

    // resolve some endpoints
    Endpoints::Endpoint rb1 = endpoints.resolve(HttpGet, "/api/devices/5/slots");
    Endpoints::Endpoint rb2 = endpoints.resolve(HttpGet, "/api/devices/i2c/slots");
    Endpoints::Endpoint rc1 = endpoints.resolve(HttpPut, "/api/devices/i2c/slot/96/meta");

    Endpoints::Endpoint r_1 = endpoints.resolve(HttpPut, "/api/bus/i2c/3/devices");

    return 0;
}
