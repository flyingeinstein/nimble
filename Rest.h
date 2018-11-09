#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <cassert>
#include <functional>


#include "pool.h"
#include "binbag.h"


// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

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

// Bitmask values for different types
#define ARG_MASK_INTEGER       ((unsigned short)1)
#define ARG_MASK_REAL          ((unsigned short)2)
#define ARG_MASK_BOOLEAN       ((unsigned short)4)
#define ARG_MASK_STRING        ((unsigned short)8)
#define ARG_MASK_UNSIGNED      ((unsigned short)16)
#define ARG_MASK_UINTEGER      (ARG_MASK_UNSIGNED|ARG_MASK_INTEGER)
#define ARG_MASK_NUMBER        (ARG_MASK_REAL|ARG_MASK_INTEGER)
#define ARG_MASK_ANY           (ARG_MASK_NUMBER|ARG_MASK_BOOLEAN|ARG_MASK_STRING)

/// \brief Convert a http method enum value to a string.
const char* uri_method_to_string(uint32_t method);

// simple token IDs
#define TID_EOF               300
#define TID_INTEGER           303
#define TID_FLOAT             305
#define TID_BOOL              307

// String token IDs
// the following token types allocate memory for string tokens and so must be freed
#define TID_ERROR                500
#define TID_STRING               501
#define TID_IDENTIFIER           502

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

protected:
    class Node;
    class Literal;
    class ArgumentType;
    class Token;

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

//    uint32_t method;
//    HandlerPrototype prototype;
//    union {
//      RestVoidMethodHandler    voidHandler;  // non-rest handler doesnt prepare json request/response
//      RestMethodHandler        restHandler;  // rest handler using Json request/response
//    }
    };

protected:
    class ArgumentType
    {
    public:
        inline ArgumentType() : name(nullptr), typemask(0), next(nullptr) {}
        ArgumentType(const char* _name, unsigned short _typemask) : name(_name), typemask(_typemask), next(nullptr) {}

    public:
        // if this argument is matched, the value is added to the request object under this field name
        const char* name;

        // collection of ARG_MASK_xxxx bits represent what types are supported for this argument
        unsigned short typemask;

        // if this is not an endpoint, then we point to the next term in the endpoint
        Node* next;
    };

public:
    class Argument : protected ArgumentType {
    public:
        Argument() : type(0), ul(0) {}
        Argument(const Argument& copy) : ArgumentType(copy), type(copy.type), ul(copy.ul) {
            if(type == ARG_MASK_STRING) {
                s = strdup(copy.s);
            }
        }

        Argument(const ArgumentType& arg) : ArgumentType(arg), type(0), ul(0) {}

        Argument(const ArgumentType& arg, long _l) : ArgumentType(arg), type(ARG_MASK_INTEGER), l(_l) {}
        Argument(const ArgumentType& arg, unsigned long _ul) : ArgumentType(arg), type(ARG_MASK_UINTEGER), ul(_ul) {}
        Argument(const ArgumentType& arg, double _d) : ArgumentType(arg), type(ARG_MASK_NUMBER), d(_d) {}
        Argument(const ArgumentType& arg, bool _b) : ArgumentType(arg), type(ARG_MASK_BOOLEAN), b(_b) {}
        Argument(const ArgumentType& arg, const char* _s) : ArgumentType(arg), type(ARG_MASK_STRING), s(strdup(_s)) {}

        ~Argument() { if(s && type == ARG_MASK_STRING) ::free(s); }

        Argument& operator=(const Argument& copy) {
            ArgumentType::operator=(copy);
            type = copy.type;
            if(type == ARG_MASK_STRING)
                s = strdup(copy.s);
            else
                ul = copy.ul;
            return *this;
        }

        inline const char* name() const { return ArgumentType::name; }

        int isOneOf(std::initializer_list<const char*> enum_values, bool case_insensitive=true) {
            typeof(strcmp) *cmpfunc = case_insensitive
                                      ? &strcasecmp
                                      : &strcmp;
            if((type&ARG_MASK_STRING)!=ARG_MASK_STRING)
                return -2;
            int j=0;
            for(std::initializer_list<const char*>::const_iterator x=enum_values.begin(), _x=enum_values.end(); x!=_x; x++,j++) {
                if (cmpfunc(s, *x) == 0)
                    return j;
            }
            return -1;
        }


        inline bool isInteger() const { return (type&ARG_MASK_INTEGER)==ARG_MASK_INTEGER; }
        inline bool isSignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_INTEGER; }
        inline bool isUnsignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_UINTEGER; }
        inline bool isNumber() const { return (type&ARG_MASK_NUMBER)==ARG_MASK_NUMBER; }
        inline bool isBoolean() const { return (type&ARG_MASK_BOOLEAN)==ARG_MASK_BOOLEAN; }
        inline bool isString() const { return (type&ARG_MASK_STRING)==ARG_MASK_STRING; }

        // only supported in C++11
        inline operator long() const { assert(type&ARG_MASK_INTEGER); return l; }
        inline operator unsigned long() const { assert(type&ARG_MASK_INTEGER); return ul; }
        inline operator double() const { assert(type&ARG_MASK_NUMBER); return d; }
        inline operator bool() const { return (type == ARG_MASK_BOOLEAN) ? b : (ul>0); }
        inline operator const char*() const { assert(type&ARG_MASK_STRING); return s; }

#if 0   // ArgumentValues should never change, so use constructor only (and assignment op if needed)
        void set(long _l) { type = ARG_MASK_INTEGER; l = _l; }
        void set(unsigned long _ul) { type = ARG_MASK_UINTEGER; l = _ul; }
        void set(bool _b) { type = ARG_MASK_BOOLEAN; b = _b; }
        void set(const char* _s) { type = ARG_MASK_STRING; s = strdup(_s); }
#endif
        unsigned short type;

        union {
            long l;
            unsigned long ul;
            double d;
            bool b;
            char* s;
        };

    public:
        static const Argument null;
    };

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


    class Token {
    private:
        // seems strange to not allow copy or assignment but we dont need it
        // we use swap() instead to pass the peek token to the current token (its more efficient)
        Token(const Token& copy);
        Token& operator=(const Token& copy);


    public:
        short id;
        char *s;        // todo: we can probably make this use binbag, and store string ID in 'i'
        int64_t i;
        double d;

        inline Token() : id(0), s(nullptr), i(0), d(0) {}

        ~Token() { clear(); }

        /// \brief clears the token and frees memory if token was a string
        void clear()
        {
            if(s && id >=500)
                free(s);
            id = 0;
            s = nullptr;
            i = 0;
            d = 0.0;
        }

        /// \brief swap the values of two tokens
        /// If using a current and peek token during parsing, this can be more efficient than copying the token
        /// across by saving a possible string copy and memory allocation.
        void swap(Token& rhs) {
            short _id = rhs.id;
            char *_s = rhs.s;
            int64_t _i = rhs.i;
            double _d = rhs.d;
            rhs.id = id;
            rhs.s = s;
            rhs.i = i;
            rhs.d = d;
            id = _id;
            s = _s;
            i = _i;
            d = _d;
        }

        void set(short _id, const char* _begin, const char* _end)
        {
            assert(_id >= 500);  // only IDs above 500 can store a string
            id = _id;
            // todo: use static buffer here, we reuse tokens anyway
            if(_end == nullptr) {
                s = strdup(_begin);
            } else {
                s = (char *) calloc(1, _end - _begin + 1);
                memcpy(s, _begin, _end - _begin);
            }
        }

        inline bool is(short _id) const {
            return id == _id;
        }

        template<typename... Targs>
        inline bool is(short _id, Targs... args) const {
            return id == _id || is(args...);
        }

        int isOneOf(std::initializer_list<const char*> enum_values, bool case_insensitive=true) {
            if(id<=500)
                return -2;
            typeof(strcmp) *cmpfunc = case_insensitive
                                      ? &strcasecmp
                                      : &strcmp;
            int j=0;
            for(std::initializer_list<const char*>::const_iterator x=enum_values.begin(), _x=enum_values.end(); x!=_x; x++,j++) {
                if (cmpfunc(s, *x) == 0)
                    return j;
            }
            return -1;
        }

        int toEnum(std::initializer_list<const char*> enum_values, bool case_insensitive=true) {
            if(id<=500)
                return -2;
            typeof(strcmp) *cmpfunc = case_insensitive
                                      ? &strcasecmp
                                      : &strcmp;
            int j=0;
            for(std::initializer_list<const char*>::const_iterator x=enum_values.begin(), _x=enum_values.end(); x!=_x; x++,j++) {
                if (cmpfunc(s, *x) == 0) {
                    clear();
                    id = TID_INTEGER;
                    i = j;
                    return j;
                }
            }
            return -1;
        }

        /// \brief scans the next token from the URL line
        int scan(const char** pinput, short allow_parameters)
        {
            const char* input = *pinput;
            char error[512];

            clear();

            if(*input==0) {
                id = TID_EOF;
                *pinput = input;
                return 0;
            }

            // check for single character token
            // note: if we find a single char token we break and then return, otherwise (default) we jump over
            // to check for longer token types like keywords and attributes
            if(strchr("/", *input)!=nullptr) {
                s = nullptr;
                id = *input++;
                goto done;
            } else if(allow_parameters && strchr("=:?(|)", *input)!=nullptr) {
                // these symbols are allowed when we are scanning a Rest URL match expression
                // but are not valid in normal URLs, or at least considered part of normal URL matching below
                s = nullptr;
                id = *input++;
                goto done;
            }


            // check for literal float
            if(input[0]=='.') {
                if(isdigit(input[1])) {
                    // decimal number
                    char *p;
                    id = TID_FLOAT;
                    d = strtod(input, &p);
                    i = (int64_t) d;
                    input = p;
                    goto done;
                } else {
                    // plain dot symbol
                    id = '.';
                    s = nullptr;
                    input++;
                    goto done;
                }
            } else if(input[0]=='0' && input[1]=='x') {
                // hex constant
                char* p;
                id = TID_INTEGER;
                i = (int64_t)strtoll(input, &p, 16);
                input = p;
                goto done;
            } else if(isdigit(*input)) {
//scan_number:
                // integer or float constant
                char *p;
                id = TID_INTEGER;
                i = (int64_t)strtoll(input, &p, 0);
                if (*p == '.') {
                    id = TID_FLOAT;
                    d = strtod(input, &p);
                    input = p;
                } else
                    input = p;
                goto done;
            }
                // check for boolean value
            else if(strncasecmp(input, "false", 5) ==0 && !isalnum(input[5])) {
                input += 5;
                id = TID_BOOL;
                i = 0;
                goto done;
            }
            else if(strncasecmp(input, "true", 4) ==0 && !isalnum(input[4])) {
                input += 4;
                id = TID_BOOL;
                i = 1;
                goto done;
            }
                // check for identifier
            else if(isalpha(*input) || *input=='_') {
                // pull out an identifier match
                short ident = TID_IDENTIFIER;
                const char* p = input;
                while(*input && (*input=='_' || *input=='-' || isalnum(*input))) {
                    input++;
                }
                set(ident, p, input);
                goto done;
            }

            sprintf(error, "syntax error, unexpected '%c' in input", *input);
            input++;
            set(TID_ERROR, error, nullptr);

            done:
            *pinput = input;
            return 1;
        }
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

