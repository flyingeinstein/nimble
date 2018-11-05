#pragma once

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <stdint.h>


// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

#define URL_MATCHED                         1
#define URL_PARTIAL                         (0)
#define URL_FAIL_NO_ENDPOINT                (-1)
#define URL_FAIL_NO_HANDLER                 (-2)
#define URL_FAIL_PARAMETER_TYPE             (-3)
#define URL_FAIL_MISSING_PARAMETER          (-4)
#define URL_FAIL_AMBIGUOUS_PARAMETER        (-5)
#define URL_FAIL_EXPECTED_PATH_SEPARATOR    (-6)
#define URL_FAIL_EXPECTED_EOF               (-7)
#define URL_FAIL_INVALID_TYPE               (-9)
#define URL_FAIL_SYNTAX                     (-10)
#define URL_FAIL_INTERNAL                   (-15)
#define URL_FAIL_INTERNAL_BAD_STRING        (-16)

/// \brief Convert a return value to a string.
/// Typically use this to get a human readable string for an error result.
const char* uri_result_to_string(short result);

// indicates a default Rest handler that matches any http verb request
// this enum belongs with the web servers HTTP_GET, HTTP_POST, HTTP_xxx constants
enum {
    HTTP_ANY = 1000
};

/// \brief Convert a http method enum value to a string.
const char* uri_method_to_string(uint32_t method);


typedef class RestRequest;
typedef std::function<short, RestRequest*> RestMethodHandler;

/// \group Rest Method Declarations
/// a block of Rest Method declarations must begin and end with these macros
#define BEGIN_ENDPOINTS(name)  const UriEndpoint name[] = {
#define END_ENDPOINTS  };

/// in between the BEGIN/END ENDPOINTS you can add Rest Method endpoints with this macro
/// you only need to call this once for each distict Uri expression, but you can add multiple
/// handlers for GET, POST, PUT, PATCH, and DELETE using the related macros in place of the ...
#define REST_ENDPOINT(uri, ...)  { uri, 0, { __VA_ARGS__ } }
/// \endgroup

/// \group Http Method Verb Handlers
/// specify one or more of these as extra arguments to the REST_ENDPOINT macro. The ANY handler
/// will be called when no other specific method matches. The ANY handler can be placed in any
/// order and the handler resolution will always try to match a specific handlers first.
#define ANY(handler) { HTTP_ANY, handler }
#define GET(handler) { HTTP_GET, handler }
#define POST(handler) { HTTP_POST, handler }
#define PUT(handler) { HTTP_PUT, handler }
#define PATCH(handler) { HTTP_PATCH, handler }
#define DELETE(handler) { HTTP_DELETE, handler }
#define OPTIONS(handler) { HTTP_OPTIONS, handler }
/// \endgroup

typedef enum {
  HttpHandler,
  RestHandler
} HandlerPrototype;

/// \brief Links a http method verb with a Rest callback handler
/// Associates a handler callback with a http method request. Many of these associations can
/// be attached to a single Endpoint Uri.
typedef struct _UriEndpointMethodHandler {
    uint32_t method;
    HandlerPrototype prototype;
    union {
      THandler          httpHandler;  // non-rest handler doesnt prepare json request/response
      RestMethodHandler restHandler;  // rest handler using Json request/response
    }
} UriEndpointMethodHandler;

/// \brief Specification for Rest Uri Endpoint with inline arguments
/// Specifies a Endpoint Uri expresssion for a Rest method that may contain named arguments
/// that will be extracted and placed in an arguments json object during evaluation.
typedef struct _UriEndpoint {
    const char* uri;
    uint32_t flags;         // UEF flags
    UriEndpointMethodHandler handlers[6];
} UriEndpoint;

class RestError {
  public:
    short code;
    String message;
};

/// \brief Contains details of a resolved Uri request and the UriExpression that matched it.
/// Like the UriEndpoint above, but used when an actual http request is matched against an
/// Endpoint Uri expression. This structure contains information on both the http request
/// and the matched expression.
typedef struct _ResolvedUriEndpoint {
    uint32_t method;        // GET, POST, PUT, PATCH, DELETE, etc
    const char* name;
    const char* requestUri;
    UriEndpointMethodHandler handler;
    std::map<String, argdddp> arguments;
    uint32_t flags;         // UEF flags
} ResolvedUriEndpoint;


/// \brief Main argument to Rest Callbacks
/// This structure is created and passed to Rest Method callbacks after the request Uri
/// and Endpoint has been resolved. Any arguments in the Uri Endpoint expression will be
/// added to the request json object and merged with POST data or other query parameters.
class RestRequest {
public:
    ResolvedUriEndpoint endpoint;   // information about the endpoint (URI) the request was made
    short httpStatus;               // return status sent in response

    std::map<String, ff> args;

    /// combined list of parameters by priority: embedded-uri, post, query-string.
    /// use functions such as query_param_u32, query_param_i64, query_param_bool or query_param_string to retrieve
    /// parameter values.
    JsonObject request;

    /// output response object built by Rest method handler.
    /// This object is empty when the method handler is called and is up to the method handler to populate with the
    /// exception of standard errors, warnings and status output which is added when the method handler returns.
    DynamicJsonDocument doc;
    JsonObject response;

    /// Array of JsonError objects.
    /// The rest method handler can insert exception objects into this array. This object is an empty json array
    /// when the method handler is called. If it has at least one error upon exit then it will be added to the
    /// response object and the method return status will also reflect that an error occured.
    /// It is recommended that any associated objectids such as stream ID be added to each error element.
    std::vector<RestError> errors;

    /// content type of the incoming request
    const char* contentType;

    /// the following Json objects get combined into the 'request' Json object but some Rest handlers may want to get
    /// parameters specifically from POST or URL query-string.
    JsonObject* post;              // parsed POST Json object
    JsonObject* query;             // query string parameters as a Json object

    unsigned long long timestamp;   // timestamp request was received, set by framework

    Esp8266WebServer& server;     // deprecated, may or may not be valid, do not use unless necessary

  protected:
    DynamicJsonDocument requestDoc;
};

struct _Endpoint;
typedef struct _Endpoint Endpoint;

struct _EndpointLiteral;
typedef struct _EndpointLiteral EndpointLiteral;

struct _EndpointArgument;
typedef struct _EndpointArgument EndpointArgument;

typedef uint16_t url_opcode_t;

/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
typedef struct _UriExpression {
    // stores the expression as a chain of endpoint nodes
    Endpoint *ep_head, *ep_tail, *ep_end;

    // allocated text strings
    binbag* text;

} UriExpression;

/// \brief Initialize an empty UriExpression with a maximum number of code size.
UriExpression *uri_endpoint_init(int elements);

/// \brief Parse one or more Uri Endpoint expressions, typically contained in a BEGIN/END ENDPOINTS macro block.
short rest_add_endpoints(UriExpression *expr, const UriEndpoint *endpoints, int count);

/// \brief Parse a single Uri Endpoint expressions and merge into the given UriExpression.
/// The supplied UriExpression does not need to be blank, it can contain previously compiled expressions.
short rest_add_endpoint(UriExpression *expr, const UriEndpoint *endpoint);

/// \brief Match a Uri against the compiled list of Uri expressions.
/// If a match is found with an associated http method handler, the resolved UriEndpoint object is filled in.
short rest_uri_resolve_endpoint(UriExpression *expr, uint32_t method, const char *uri,
                                ResolvedUriEndpoint *endpoint_out);

/// \brief Print the state machine code for the URL processor.
yarn *rest_uri_debug_print(UriExpression *expr, yarn *out);

const char* request_param_string(RestRequest* request, const char* name);
const char* query_param_string(RestRequest* request, const char* name);
BOOL query_param_u32(RestRequest* request, const char* name, UINT32* value_out);
BOOL query_param_i64(RestRequest* request, const char* name, INT64* value_out);
BOOL query_param_bool(RestRequest* request, const char* name, BOOL default_value);
//const char* post_param_string(RestRequest* request, const char* name);
//const char* post_param_numeric(RestRequest* request, const char* name);
