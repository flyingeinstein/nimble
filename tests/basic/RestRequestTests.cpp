
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "../../Rest.h"


template<class... TArgs>
class Handler {
public:
    typedef std::function< int(TArgs... args) > F0;

    Handler() {}
    Handler(F0 _f) : f0(std::move(_f)) {
    }
    Handler(int _f(TArgs... args)) : f0(_f) {}

    int operator()(TArgs... args) {
        return f0(args...);
    }

    F0 f0;
};

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

using Rest::GET;
using Rest::PUT;
using Rest::POST;
using Rest::PATCH;
using Rest::DELETE;
using Rest::OPTIONS;

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

    typedef Handler< TRestRequest& > RequestHandler;
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

    RestRequestHandler& on(const char *endpoint_expression, Rest::MethodHandler< int(TRestRequest&) > methodHandler ) {
        Rest::MethodHandler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, h);
        return *this;
    }

    RestRequestHandler& on(const char *endpoint_expression, Rest::MethodHandler< std::function< int(TRestRequest&) > > methodHandler ) {
        Rest::MethodHandler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, h);
        return *this;
    }

#if 0
    // c++11 using parameter pack expressions to recursively call add()
    template<class T, class... Targs>
    RestRequestHandler& on(const char *endpoint_expression, T h1, Targs... rest ) {
      on(endpoint_expression, h1);   // add first argument
      return on(endpoint_expression, rest...);   // add the rest (recursively)
    }
#endif
};


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
