
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "requests.h"


#if 0
template<class H> Rest::MethodHandler<std::function<H> > PUTR(const H& handler) {
    auto f = std::function<H>(handler);
    return Rest::MethodHandler<std::function<H> >(HttpPut, f);
}
#endif

TEST(endpoints_std_function)
{
    RestRequestHandler<RestRequest> rest;
    std::function<int(RestRequest & )> func = [](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    };
    rest.on("/api/echo/:msg(string|integer)", GET(func));
    return OK;
}

int handler_func(RestRequest& r) { r.response = "hello world"; return 2; }

TEST(endpoints_function)
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)", GET(handler_func));
    return OK;
}

//template<class H> Rest::Handler<H&> GETT(std::function< int(H&) > handler) { return Rest::Handler<H&>(HttpGet, handler); }
//Rest::Handler<RestRequest&> GETT(std::function< int(RestRequest&) > handler) { return Rest::Handler<RestRequest&>(HttpGet, handler); }

TEST(endpoints_lambda)
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)", GET([](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    }));
    return OK;
}
