
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "../../Rest.h"


class RestRequest
{
public:
    Rest::HttpMethod method;
    std::string uri;
    std::string request;
    std::string response;
    Rest::Arguments& args;

    RestRequest(Rest::Arguments& _args) : args(_args) {}
};


using Rest::HttpMethod;
using Rest::HttpGet;
using Rest::HttpPut;
using Rest::HttpPost;
using Rest::HttpPatch;
using Rest::HttpDelete;
using Rest::HttpOptions;

template<class TRestRequest>
class RestRequestHandler
{
public:
    // types
    typedef TRestRequest RequestType;

    typedef Rest::Handler< TRestRequest& > RequestHandler;
    typedef Rest::Endpoints<RequestHandler> Endpoints;

    // the collection of Rest handlers
    Endpoints endpoints;

    virtual bool handle(HttpMethod requestMethod, std::string requestUri) {
        Rest::HttpMethod method = (Rest::HttpMethod)requestMethod;
        typename Endpoints::Endpoint ep = endpoints.resolve(method, requestUri.c_str());
        if (ep) {
            RequestType request(ep);
            request.method = method;
            request.uri = requestUri;
            ep.handler(request);
            return true;
        } else
            return false;
    }

#if 0
    RestRequestHandler& on(const char *endpoint_expression, Rest::Handler< TRestRequest& > methodHandler ) {
        //Rest::Handler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, methodHandler);
        return *this;
    }

    RestRequestHandler& on(const char *endpoint_expression, std::function< int(TRestRequest&) > methodHandler ) {
        //Rest::Handler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, methodHandler);
        return *this;
    }
#else
    template<class... Targs>
    RestRequestHandler& on(const char *endpoint_expression, Targs... rest ) {
        endpoints.on(endpoint_expression, rest...);   // add the rest (recursively)
        return *this;
    }
#endif

#if 0
    // c++11 using parameter pack expressions to recursively call add()
    template<class T, class... Targs>
    RestRequestHandler& on(const char *endpoint_expression, T h1, Targs... rest ) {
      on(endpoint_expression, h1);   // add first argument
      return on(endpoint_expression, rest...);   // add the rest (recursively)
    }
#endif
};

DEFINE_HTTP_METHOD_HANDLERS(RestRequest)

using Rest::uri_result_to_string;

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
