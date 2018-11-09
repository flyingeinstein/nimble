#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <cassert>
#include <functional>


#include "pool.h"
#include "binbag.h"

#include "Token.h"
#include "Argument.h"

// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

namespace Rest {

#define URL_MATCHED                         0
#define URL_FAIL_NO_ENDPOINT                (-1)
#define URL_FAIL_NO_HANDLER                 (-2)
#define URL_FAIL_DUPLICATE                  (-3)
#define URL_FAIL_PARAMETER_TYPE             (-4)
#define URL_FAIL_MISSING_PARAMETER          (-5)
#define URL_FAIL_AMBIGUOUS_PARAMETER        (-6)
#define URL_FAIL_EXPECTED_PATH_SEPARATOR    (-7)
#define URL_FAIL_EXPECTED_EOF               (-8)
#define URL_FAIL_INVALID_TYPE               (-9)
#define URL_FAIL_SYNTAX                     (-10)
#define URL_FAIL_INTERNAL                   (-15)
#define URL_FAIL_INTERNAL_BAD_STRING        (-16)

/// \brief Convert a return value to a string.
/// Typically use this to get a human readable string for an error result.
const char* uri_result_to_string(short result);

// indicates a default Rest handler that matches any http verb request
// this enum belongs with the web servers HTTP_GET, HTTP_POST, HTTP_xxx constants
typedef enum {
    HttpGet=1,
    HttpPost,
    HttpPut,
    HttpPatch,
    HttpDelete,
    HttpOptions,   
    HttpMethodAny = 128
} HttpMethod;

/// \brief Convert a http method enum value to a string.
const char* uri_method_to_string(uint32_t method);


//typedef class RestRequest;
//typedef std::function<short, RestRequest*> RestMethodHandler;
//typedef std::function<void, void> RestVoidMethodHandler;

typedef enum {
  HttpHandler,
  RestHandler
} HandlerPrototype;


/// \brief Contains details of a resolved Uri request and the UriExpression that matched it.
/// Like the UriEndpoint above, but used when an actual http request is matched against an
/// Endpoint Uri expression. This structure contains information on both the http request
/// and the matched expression.
/*typedef struct _ResolvedUriEndpoint {
    uint32_t method;        // GET, POST, PUT, PATCH, DELETE, etc
    const char* name;
    const char* requestUri;
    UriEndpointMethodHandler handler;
    std::map<String, argdddp> arguments;
    uint32_t flags;         // UEF flags
} ResolvedUriEndpoint;*/


typedef uint16_t url_opcode_t;

/// \defgroup MethodHandlers Associates an http method verb to a handler function
/// \@{
/// \brief Class used to associate a http method verb with a handler function
/// This is used when adding a handler to an endpoint for specific http verbs such as GET, PUT, POST, PATCH, DELETE, etc.
/// Do not use this class, but instead use the template functions  GET(handler), PUT(handler), POST(handler), etc.
template<class H>
class MethodHandler {
public:
    HttpMethod method;
    H& handler;

    MethodHandler(HttpMethod _method, H& _handler) : method(_method), handler(_handler) {}
};

template<class H> MethodHandler<H> GET(H& handler) { return MethodHandler<H>(HttpGet, handler); }
template<class H> MethodHandler<H> PUT(H& handler) { return MethodHandler<H>(HttpPut, handler); }
template<class H> MethodHandler<H> POST(H& handler) { return MethodHandler<H>(HttpPost, handler); }
template<class H> MethodHandler<H> PATCH(H& handler) { return MethodHandler<H>(HttpPatch, handler); }
template<class H> MethodHandler<H> DELETE(H& handler) { return MethodHandler<H>(HttpDelete, handler); }
template<class H> MethodHandler<H> OPTIONS(H& handler) { return MethodHandler<H>(HttpOptions, handler); }
template<class H> MethodHandler<H> ANY(H& handler) { return MethodHandler<H>(HttpMethodAny, handler); }
/// \@}


/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
class Endpoints {
public:
    class Handler;
    typedef ::Rest::Argument Argument;

protected:
    class Node;
    class Literal;
    typedef ::Rest::Token Token;

    class ArgumentType : public ::Rest::Type {
        public:
            inline ArgumentType() : next(nullptr) {}
            ArgumentType(const char* _name, unsigned short _typemask)
                : ::Rest::Type(_name, _typemask), next(nullptr)
            {}

            // if this is not an endpoint, then we point to the next term in the endpoint
            Node* next;
    };

private:
    // stores the expression as a chain of endpoint nodes
    Node *ep_head, *ep_tail, *ep_end;

    // allocated text strings
    binbag* text;

    // some statistics on the endpoints
    size_t maxUriArgs;       // maximum number of embedded arguments on any one endpoint expression

public:
    /// \brief Links a http method verb with a Rest callback handler
/// Associates a handler callback with a http method request. Many of these associations can
/// be attached to a single Endpoint Uri.
    class Handler {
    public:
        std::string _name;

        Handler() {}
        Handler(const char* __name) : _name(__name) {}

        inline bool operator==(const Handler& rhs) const { return _name==rhs._name; }

//    uint32_t method;
//    HandlerPrototype prototype;
//    union {
//      RestVoidMethodHandler    voidHandler;  // non-rest handler doesnt prepare json request/response
//      RestMethodHandler        restHandler;  // rest handler using Json request/response
//    }
    };

protected:


public:
    class Endpoint {
    public:
        int status;
        std::string name;
        HttpMethod method;
        Handler handler;

        inline Endpoint() :  status(0), method(HttpMethodAny), args(nullptr), nargs(0) {}
        inline Endpoint(HttpMethod _method, const Handler& _handler, int _status) :  status(_status), method(_method), handler(_handler), args(nullptr), nargs(0) {}
        inline Endpoint(const Endpoint& copy) : status(copy.status), name(copy.name), handler(copy.handler), method(copy.method),
                    args(nullptr), nargs(copy.nargs)
        {
            if(nargs>0) {
                args = new Argument[nargs];
                for (int i = 0; i < nargs; i++)
                    args[i] = copy.args[i];
            }
        }

        virtual ~Endpoint() { delete [] args; }

        inline Endpoint& operator=(const Endpoint& copy) {
            status = copy.status;
            name = copy.name;
            method = copy.method;
            handler = copy.handler;
            nargs = copy.nargs;
            if(nargs>0) {
                args = new Argument[nargs];
                for (int i = 0; i < nargs; i++)
                    args[i] = copy.args[i];
            }
            return *this;
        }

        inline explicit operator bool() const { return status==URL_MATCHED; }

        const Argument& operator[](int idx) const {
            return (idx>=0 && idx<nargs)
                ? args[idx]
                : Argument::null;
        }

        const Argument& operator[](const char* _name) const {
            for(int i=0; i<nargs; i++) {
                if (strcmp(_name, args[i].name()) == 0)
                    return args[i];
            }
            return Argument::null;
        }

    protected:
        Argument* args;
        size_t nargs;

        friend Endpoints;
    };

public:
    Handler defaultHandler; // like a 404 handler

public:
    /// \brief Initialize an empty UriExpression with a maximum number of code size.
    explicit Endpoints(int elements);

    /// \brief Destroys the RestUriExpression and releases memory
    virtual ~Endpoints() {
        ::free(ep_head);
        binbag_free(text);
        delete exception;
    }

    /// \brief Parse and add single Uri Endpoint expressions to our list of Endpoints
    Endpoints& add(const char *endpoint_expression, MethodHandler<Handler> methodHandler );

#if __cplusplus < 201103L || (defined(_MSC_VER) && _MSC_VER < 1900)
    // for pre-c++11 support we have to specify a number of add handler methods
    inline Endpoints& add(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2 ) {
        add(endpoint_expression, h1);
        return add(endpoint_expression, h2);
    }
    inline Endpoints& add(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3 ) {
        add(endpoint_expression, h1, h2);
        return add(endpoint_expression, h3);
    }
#else
    // c++11 using parameter pack expressions to recursively call add()
    template<class T, class... Targs>
    Endpoints& add(const char *endpoint_expression, T h1, Targs... rest ) {
        add(endpoint_expression, h1);   // add first argument
        return add(endpoint_expression, rest...);   // add the rest (recursively)
    }
#endif

    inline Endpoints& katch(const std::function<void(Endpoint)>& endpoint_exception_handler) {
        if(exception != nullptr) {
            endpoint_exception_handler(*exception);
            delete exception;
        }
        return *this;
    }

    /// \brief Match a Uri against the compiled list of Uri expressions.
    /// If a match is found with an associated http method handler, the resolved UriEndpoint object is filled in.
    Endpoint resolve(HttpMethod method, const char *uri);

    /// \brief Print the state machine code for the URL processor.
    //yarn *debugPrint(UriExpression *expr, yarn *out);

    /*
     * Internal Members
     */

public:
    inline Node* newNode()
    {
        return new (ep_end++) Node();
    }

    inline ArgumentType* newArgument(const char* name, unsigned short typemask)
    {
        // todo: make this part of paged memory
        size_t nameid = binbag_insert_distinct(text, name);
        ArgumentType* arg = new ArgumentType(binbag_get(text, nameid), typemask);  // todo: use our binbag here
        return arg;
    }

    Literal* newLiteral(Node* ep, Literal* literal);
    Literal* addLiteralString(Node* ep, const char* literal_value);
    Literal* addLiteralNumber(Node* ep, ssize_t literal_value);

protected:
    /// \brief if an error occurs during an add() this member will be set
    /// all further add() calls will instantly return without adding Endpoints. Use katch() member to handle this
    /// exception at some point after one or more add() calls.
    Endpoint* exception;

    class Node
    {
    public:
        // if there is more input in parse stream
        Literal *literals;    // first try to match one of these literals

        // if no literal matches, then try to match based on token type
        ArgumentType *string, *numeric, *boolean;

        // if we are at the end of the URI then we can pass to one of the http verb handlers
        Handler *GET, *POST, *PUT, *PATCH, *DELETE, *OPTIONS;
    };

    class Literal
    {
    public:
        // if this argument is matched, the value is added to the request object under this field name
        // this id usually indicates an index into an array of text terms (binbag)
        ssize_t id;

        // true if the id should be take as a numeric value and not a string index ID
        bool isNumeric;

        // if this is not an endpoint, then we point to the next term in the url path
        Node* next;

        inline bool isValid() { return isNumeric || (id>=0); }
    };




    typedef enum {
        mode_add = 1,           // indicates adding a new endpoint/handler
        mode_resolve = 2        // indicates we are resolving a URL to a defined handler
    } parse_mode_e;

    class ParseData {
    public:
        parse_mode_e mode;   // indicates if we are parsing to resolve or to add an endpoint

        // parser data
        const char *uri;    // parser input string (gets eaten as parsing occurs)
        Token t, peek;      // current token 't' and look-ahead token 'peek'

        int state;          // parser state machine current state
        int level;          // level of evaluation, typically 1 level per path separation

        Node* ep;      // current endpoint evaluated

        // holds the current method name
        // the name is generated as we are parsing the URL
        char methodName[2048];
        char *pmethodName;

        ParseData(Endpoints* expr, const char** _uri);

        // will contain the arguments embedded in the URL
        // when adding, will contain argument type info
        // when resolving, will contain argument values
        ArgumentType** argtypes;
        Argument* args;
        size_t nargs;
        size_t szargs;
    };

protected:
    short parse(ParseData* ev);
};

} // ns:Rest