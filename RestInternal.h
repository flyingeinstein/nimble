#pragma once

#include <sys/types.h>

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

// Bitmask values for different types
#define JSON_BITMASK_INTEGER       (1)
#define JSON_BITMASK_REAL          (2)
#define JSON_BITMASK_BOOLEAN       (4)
#define JSON_BITMASK_STRING        (8)
#define JSON_BITMASK_NUMBER        (JSON_BITMASK_REAL|JSON_BITMASK_INTEGER)
#define JSON_BITMASK_ANY           (JSON_BITMASK_NUMBER|JSON_BITMASK_BOOLEAN|JSON_BITMASK_STRING)

struct _EndpointLiteral;
struct _EndpointArgument;


struct _Endpoint {
    // if there is more input in parse stream
    struct _EndpointLiteral *literals;    // first try to match one of these literals

    // if no literal matches, then try to match based on token type
    struct _EndpointArgument *string, *numeric, *boolean;

    // if we are at the end of the URI then we can pass to one of the http verb handlers
    RestMethodHandler GET, POST, PUT, PATCH, DELETE, OPTIONS;
};


struct _EndpointLiteral {
    // if this argument is matched, the value is added to the request object under this field name
    // this id usually indicates an index into an array of text terms (binbag)
    ssize_t id;

    // true if the id should be take as a numeric value and not a string index ID
    BOOL isNumeric;

    // if this is not an endpoint, then we point to the next term in the endpoint
    struct _Endpoint* next;
};


struct _EndpointArgument {
    // if this argument is matched, the value is added to the request object under this field name
    const char* name;

    // if this is not an endpoint, then we point to the next term in the endpoint
    struct _Endpoint* next;
};


typedef enum {
    nop = 0x1000, // lets store opcodes in the upper bit so we can do some assert testing during debugging
    pushString,
    scan,
    pop,
    jmp,
    isType,
    isNotType,
    matchSep,
    matchEof,
    matchWord,
    writeParam,     // [name:stringid, :0]      write a parameter to output json
    setName,        // set the rest method name
    callEndpoint,     // set the handler ID for the rest method
    success,
    fail
} url_opcode_e;

typedef enum {
    mode_add = 1,           // indicates adding a new endpoint/handler
    mode_resolve = 2        // indicates we are resolving a URL to a defined handler
} eval_mode_e;

typedef struct _token {
    short id;
    char *s;
    int64_t i;
    double d;
} token;

typedef struct {
    eval_mode_e mode;   // indicates if we are parsing to resolve or to add an endpoint

    // parser data
    const char *uri;    // parser input string (gets eaten as parsing occurs)
    token t, peek;      // current token 't' and look-ahead token 'peek'

    int state;          // parser state machine current state
    int level;          // level of evaluation, typically 1 level per path separation

    Endpoint* ep;      // current endpoint evaluated
    const UriEndpoint *endpoint;    // static endpoint declaration using macros (only set when adding endpoints)

    // holds the current method name
    // the name is generated as we are parsing the URL
    char methodName[2048];
    char *pmethodName;

    // will contain the arguments embedded in the URL
    // when adding, will contain argument type info
    // when resolving, will contain argument values
    json_object* arguments;
} rest_uri_eval_data;


// initialize the parsing and evaluation data
short rest_uri_init_eval_data(UriExpression *expr, rest_uri_eval_data *ev, const char **uri);

// lexer functions
void token_clear(token *token);
void token_set_string(token *t, short id, const char *_begin, const char *_end);
int rest_uri_scanner(token *t, const char **pinput, short allow_parameters);

const UriEndpointMethodHandler *uri_find_handler(const UriEndpoint *ep, uint32_t method);

// internal functions
short rest_uri_resolve_internal(UriExpression *expr, rest_uri_eval_data *ev, ResolvedUriEndpoint *endpoint_out);
yarn *rest_uri_debug_print_internal(UriExpression *expr, rest_uri_eval_data *ev, yarn *out, int level);


Endpoint* rest_ep_new(UriExpression* exp);

BOOL rest_ep_literal_isValid(EndpointLiteral* lit);

int rest_ep_literal_count(EndpointLiteral* lit);

// add a literal (string or number) to an endpoint
EndpointLiteral* rest_ep_add_literal(Endpoint* ep, EndpointLiteral* literal);
EndpointLiteral* rest_ep_add_literal_string(Endpoint* ep, binbag* bb, const char* literal_value);
EndpointLiteral* rest_ep_add_literal_number(Endpoint* ep, binbag* bb, ssize_t literal_value);
