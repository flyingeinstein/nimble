
#include <stdio.h>
#include <stdlib.h>
#include "Rest.h"


int main(int argc, const char* argv[])
{
	Endpoints endpoints(100);
	Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");

    // add some endpoints
	endpoints
	    .add("/api/devices", GET(devices))
	    .add("/api/devices/:dev(integer|string)/slots", GET(slots))
        .add("/api/devices/:dev(integer|string)/slot/:slot(integer)/meta", PUT(slot));
    endpoints
        .add("/api/bus/i2c/:bus(integer)/devices",
                  GET(getbus),
                  PUT(putbus),
                  PATCH(putbus),
                  POST(putbus),
                  DELETE(putbus),
                  OPTIONS(putbus),
                  GET(putbus)
    );

    // resolve some endpoints
    Endpoints::Endpoint rb1 = endpoints.resolve(HttpGet, "/api/devices/5/slots");
    Endpoints::Endpoint rb2 = endpoints.resolve(HttpGet, "/api/devices/i2c/slots");
    Endpoints::Endpoint rc1 = endpoints.resolve(HttpPut, "/api/devices/i2c/slot/96/meta");

    Endpoints::Endpoint r_1 = endpoints.resolve(HttpPut, "/api/bus/i2c/3/devices");

    return 0;
}
