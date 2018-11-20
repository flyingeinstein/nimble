#pragma once

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "Rest.h"

class Devices;

class RestError {
  public:
    short code;
    String message;
};

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


/// \brief Main argument to Rest Callbacks
/// This structure is created and passed to Rest Method callbacks after the request Uri
/// and Endpoint has been resolved. Any arguments in the Uri Endpoint expression will be
/// added to the request json object and merged with POST data or other query parameters.
template<class TWebServer, class TRequest, class TResponse>
class RestRequest : public TRequest, public TResponse {
  public:
    typedef TWebServer WebServerType;
    typedef TRequest RequestType;
    typedef TResponse ResponseType;

    Devices& devices;
    TWebServer& server;
    Rest::Arguments& args;

    Rest::HttpMethod method;        // GET, POST, PUT, PATCH, DELETE, etc

    /// content type of the incoming request (should always be application/json)
    const char* contentType;

    unsigned long long timestamp;   // timestamp request was received, set by framework

    short httpStatus;               // return status sent in response

    inline const Rest::Argument& operator[](int idx) const { return args[idx]; }
    inline const Rest::Argument& operator[](const char* _name) const { return args[_name]; }


    RestRequest(Devices& _devices, TWebServer& _server, Rest::Arguments& _args) : devices(_devices), server(_server), args(_args) {}

  protected:
    DynamicJsonDocument requestDoc;
};


template<class TRestRequest>
class RestRequestHandler : public ::RequestHandler
{
  public:
    // types
    typedef TRestRequest RequestType;
    typedef typename RequestType::WebServerType WebServerType;

    typedef Handler< TRestRequest& > RequestHandler;
    typedef Rest::Endpoints<RequestHandler> Endpoints;

    // the collection of Rest handlers
    Endpoints endpoints;

    RestRequestHandler(Devices* _owner) : owner(_owner) {}

    virtual bool canHandle(HTTPMethod method, String uri) {
      return true;
    }

    virtual bool handle(WebServerType& server, HTTPMethod requestMethod, String requestUri) {
      Rest::HttpMethod method = (Rest::HttpMethod)requestMethod;
      typename Endpoints::Endpoint ep = endpoints.resolve(method, requestUri.c_str());
      if (ep) {
        RequestType request(*owner, server, ep);
        //request.endpoint = ep;
        request.method = method;
        request.contentType = "application/json";
        request.timestamp = millis();
        request.httpStatus = 0;

        ep.handler(request);

        // make a << operator to send output to server response
        String content;
        serializeJson(request.response, content);
        server.send(request.httpStatus, "application/json", content);
        return true;
      }
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

  protected:
    Devices* owner;
};

namespace ArduinoJson {
class Request {
  public:
    Request() {
      request = requestDoc.to<JsonObject>();
    }

    /// The parsed POST as Json if the content-type was application/json
    DynamicJsonDocument requestDoc;
    JsonObject request;
};

class Response {
  public:
    Response() {
      response = responseDoc.to<JsonObject>();
      // todo: errors array?
    }

    /// output response object built by Rest method handler.
    /// This object is empty when the method handler is called and is up to the method handler to populate with the
    /// exception of standard errors, warnings and status output which is added when the method handler returns.
    DynamicJsonDocument responseDoc;
    JsonObject response;

    /// Array of JsonError objects.
    /// The rest method handler can insert exception objects into this array. This object is an empty json array
    /// when the method handler is called. If it has at least one error upon exit then it will be added to the
    /// response object and the method return status will also reflect that an error occured.
    /// It is recommended that any associated objectids such as stream ID be added to each error element.
    std::vector<RestError> errors;

    // todo: we should have a << operator here to output to server Stream
};
}

typedef RestRequest<ESP8266WebServer, ArduinoJson::Request, ArduinoJson::Response> Esp8266RestRequest;
typedef RestRequestHandler< Esp8266RestRequest > Esp8266RestRequestHandler;
