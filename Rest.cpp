//
// Created by ineoquest on 2/2/18.
//

#include "Rest.h"


#include <string.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if 0   // I think this is old and was always unused
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
#endif

Endpoints::rest_uri_eval_data::rest_uri_eval_data(Endpoints* _expr, const char** _uri)
    : mode(mode_resolve), uri(nullptr), state(0), level(0), ep( _expr->ep_head ),
      pmethodName(methodName), argtypes(nullptr), args(nullptr), nargs(0), szargs(0)
{
    methodName[0]=0;
    if(_uri != nullptr) {
        // scan first token
        if (!t.scan(_uri, 1))
            goto bad_eval;
        if (!peek.scan(_uri, 1))
            goto bad_eval;
    }
    uri = *_uri;
    return;
bad_eval:
    state = -1;
}

const char* uri_method_to_string(uint32_t method) {
    switch(method) {
        case HttpGet: return "GET";
        case HttpPost: return "POST";
        case HttpPut: return "PUT";
        case HttpPatch: return "PATCH";
        case HttpDelete: return "DELETE";
        case HttpOptions: return "OPTIONS";
        case HttpMethodAny: return "ANY";
        default: return "GET";
    }
}

const char* uri_result_to_string(short result) {
    switch(result) {
        case URL_MATCHED: return "matched";
        case URL_PARTIAL: return "partial match";
        case URL_FAIL_NO_ENDPOINT: return "no matching endpoint";
        case URL_FAIL_NO_HANDLER: return "endpoint doesnt support requests for given http verb";
        case URL_FAIL_PARAMETER_TYPE: return "parameter type mismatch";
        case URL_FAIL_MISSING_PARAMETER: return "missing expected parameter";
        case URL_FAIL_AMBIGUOUS_PARAMETER: return "ambiguous parameter type in endpoint declaration";
        case URL_FAIL_EXPECTED_PATH_SEPARATOR: return "expected path separator";
        case URL_FAIL_EXPECTED_EOF: return "expected end of input";
        case URL_FAIL_INVALID_TYPE: return "invalid type";
        case URL_FAIL_SYNTAX: return "syntax error";
        case URL_FAIL_INTERNAL: return "internal error";
        case URL_FAIL_INTERNAL_BAD_STRING: return "internal error: bad string reference";
        default: return "unspecified error";
    }
}

#if 0
const UriEndpointMethodHandler* uri_find_handler(const UriEndpoint* ep, uint32_t method)
{
    // try to find an exact match, otherwise accept the ANY handler if supplied
    int i=0;
    const UriEndpointMethodHandler *any=NULL, *handler=NULL;
    for(; i<sizeof(ep->handlers)/sizeof(UriEndpointMethodHandler); i++) {
        if(MG_ANY==ep->handlers[i].method)
            any = &ep->handlers[i];
        else if(ep->handlers[i].handler!=NULL && ep->handlers[i].method == method)
            return &ep->handlers[i];
    }
    return (handler!=NULL) ? handler : any;
}
#endif





Endpoints::Endpoints(int elements)
    : text( binbag_create(1000, 1.5) ), maxUriArgs(0)
{
    ep_head = ep_tail = ep_end =  (Node*)calloc(elements, sizeof(Node));
    Node* root = newNode(); // create root endpoint
    //root->
}

#define GOTO_STATE(st) { ev->state = st; goto rescan; }
#define NEXT_STATE(st) { ev->state = st; }
#define SCAN { ev->t.clear(); ev->t.swap( ev->peek ); ev->peek.scan(&ev->uri, 1); }


// list of possible states
enum {
    expectPathPartOrSep,        // first token only
    expectPathSep,
    expectPathPart,
    expectPathPartOrParam,
    expectParam,
    expectParameterValue,
    endParam,
    errorExpectedIdentifier,
    errorExpectedIdentifierOrString,
    expectHtmlSuffix,
    expectEof
} url_state_e;

short Endpoints::parse(rest_uri_eval_data* ev)
{
    short rv;
    uint64_t ptr;
    long wid;
    Node* epc = ev->ep;
    Literal* lit;
    Argument* arg;

    // read datatype or decl type
    while(ev->t.id!=TID_EOF) {
    rescan:
        epc = ev->ep;

        switch(ev->state) {
            case expectPathPartOrSep:
                // check if we have a sep, and jump immediately to other state
                GOTO_STATE( (ev->t.id=='/')
                            ? expectPathSep
                            : expectPathPart
                );
                break;
            case expectPathSep: {
                // expect a slash or end of URL
                if(ev->t.id=='/') {
                    NEXT_STATE( (ev->mode==mode_resolve) ? expectPathPart : expectPathPartOrParam);

                    // add seperator to method name
                    if(ev->pmethodName > ev->methodName && *(ev->pmethodName-1)!='/')
                        *ev->pmethodName++ = '/';

                    // recursively call parse so we can add more code at the end of this match
#if 0 // another case of not having to recurse
                    SCAN;
                    if((rv=parse(ev))!=0)
                        return rv;

                    // we can post-evaluate the branch we just parsed here
                    return 0;
#endif
                } else if(ev->t.id == TID_EOF) {
                    // safe place to end
                    goto done;
                }
            } break;
            case expectHtmlSuffix: {
                if(ev->t.id==TID_STRING || ev->t.id==TID_IDENTIFIER) {
                    if(strcasecmp(ev->t.s, "html") !=0) {
                        return URL_FAIL_NO_ENDPOINT;    // only supports no suffix, or html suffix
                    } else
                        NEXT_STATE(expectEof);  // always expect eof after suffix
                }
            } break;
            case expectPathPart: {
                if(ev->t.id==TID_STRING || ev->t.id==TID_IDENTIFIER) {
                    // we must see if we already have a literal with this name
                    lit = NULL;
                    wid = binbag_find_nocase(text, ev->t.s);
                    if(wid>=0 && epc->literals) {
                        // word exists in dictionary, see if it is a literal of current endpoint
                        lit = epc->literals;
                        while(lit->isValid() && lit->id!=wid)
                            lit++;
                        if(lit->id!=wid)
                            lit=NULL;
                    }

                    // add the new literal if we didnt find an existing one
                    if(lit==NULL) {
                        if(ev->mode == mode_add) {
                            // regular URI word, add to lexicon and generate code
                            lit = addLiteralString(ev->ep, ev->t.s);
                            ev->ep = lit->next = newNode();
                        } else if(ev->mode == mode_resolve && epc->string!=NULL) {
                            GOTO_STATE(expectParameterValue);
                        } else
                            return URL_FAIL_NO_ENDPOINT;
                    } else
                        ev->ep = lit->next;

                    NEXT_STATE( expectPathSep );

                    // add component to method name
                    strcpy(ev->pmethodName, ev->t.s);
                    ev->pmethodName += strlen(ev->t.s);

#if 0   // todo: validate: we recursed in iq, but we didt do a scan first (either add SCAN, or dont recurse because we are no longer doing anything after)
                    // recursively call parse so we can add more code at the end of this match
                    if((rv=parse(ev))!=0)
                        return rv;
                    return 0;   // inner recursive call would have completed call, so we are done too
#endif
                } else if(ev->mode==mode_resolve) {
                    GOTO_STATE(expectParameterValue);
                } else
                    NEXT_STATE( errorExpectedIdentifierOrString );
            } break;
            case expectParameterValue: {
                const char* _typename=NULL;
                assert(ev->args);   // must have collection of args
                assert(ev->nargs < ev->szargs);

                // try to match a parameter by type
                if((ev->t.id==TID_STRING || ev->t.id==TID_IDENTIFIER) && epc->string!=NULL) {
                    // we can match by string argument type (parameter match)
                    assert(ev->args);
                    ev->args[ev->nargs++] = ArgumentValue(*epc->string, ev->t.s);
                    //json_object_object_add(ev->arguments, epc->string->name, json_object_new_string(ev->t.s));
                    ev->ep = epc->string->next;
                    _typename = "string";
                } else if(ev->t.id==TID_INTEGER && epc->numeric!=NULL) {
                    // numeric argument
                    ev->args[ev->nargs++] = ArgumentValue(*epc->numeric, (long)ev->t.i);
                    //json_object_object_add(ev->arguments, epc->numeric->name, json_object_new_int64(ev->t.i));
                    ev->ep = epc->numeric->next;
                    _typename = "int";
                } else if(ev->t.id==TID_FLOAT && epc->numeric!=NULL) {
                    // numeric argument
                    ev->args[ev->nargs++] = ArgumentValue(*epc->numeric, ev->t.d);
                    //json_object_object_add(ev->arguments, epc->numeric->name, json_object_new_double(ev->t.d));
                    ev->ep = epc->numeric->next;
                    _typename = "float";
                } else if(ev->t.id==TID_BOOL && epc->boolean!=NULL) {
                    // numeric argument
                    ev->args[ev->nargs++] = ArgumentValue(*epc->boolean, ev->t.i>0);
                    //json_object_object_add(ev->arguments, epc->boolean->name, json_object_new_boolean(ev->t.i>0));
                    ev->ep = epc->boolean->next;
                    _typename = "boolean";
                } else
                    NEXT_STATE( errorExpectedIdentifierOrString ); // no match by type

                // add component to method name
                *ev->pmethodName++ = '<';
                strcpy(ev->pmethodName, _typename);
                ev->pmethodName += strlen(_typename);
                *ev->pmethodName++ = '>';
                *ev->pmethodName = 0;

                // successful match, jump to next endpoint node
                NEXT_STATE( expectPathSep );
                SCAN;

                // recursively call parse so we can add more code at the end of this match
                if((rv=parse(ev))!=0)
                    return rv;
                return 0;   // inner recursive call would have completed call, so we are done too

            } break;
            case expectPathPartOrParam: {
                if(ev->t.id==':')
                    NEXT_STATE(expectParam)
                else
                    GOTO_STATE(expectPathPart);
            } break;
            case expectParam: {
                if(ev->t.id==TID_IDENTIFIER) {
                    uint16_t typemask = 0;
                    char name[32];
                    memset(&name, 0, sizeof(name));
                    strcpy(name, ev->t.s);

                    // read parameter spec
                    if(ev->peek.id=='(') {
                        SCAN;

                        // read parameter types
                        do {
                            SCAN;
                            if(ev->t.id!=TID_IDENTIFIER)
                                return URL_FAIL_SYNTAX;

                            // scan the parameter typename
                            if(strcmp(ev->t.s, "integer")==0 || strcmp(ev->t.s, "int")==0)
                                typemask |= ARG_MASK_INTEGER;
                            else if(strcmp(ev->t.s, "real")==0)
                                typemask |= ARG_MASK_REAL;
                            else if(strcmp(ev->t.s, "number")==0)
                                typemask |= ARG_MASK_NUMBER;
                            else if(strcmp(ev->t.s, "string")==0)
                                typemask |= ARG_MASK_STRING;
                            else if(strcmp(ev->t.s, "boolean")==0 || strcmp(ev->t.s, "bool")==0)
                                typemask |= ARG_MASK_BOOLEAN;
                            else if(strcmp(ev->t.s, "any")==0)
                                typemask |= ARG_MASK_ANY;
                            else
                                return URL_FAIL_INVALID_TYPE;

                            SCAN;
                        } while(ev->t.id=='|');

                        // expect closing tag
                        if(ev->t.id !=')')
                            return URL_FAIL_SYNTAX;
                    } else typemask = ARG_MASK_ANY;

                    // determine the typemask of any already set handlers at this endpoint.
                    // we cannot have two different handlers that handle the same type, but if the typemask
                    // exactly matches we can just consider a match and jump to that endpoint.
                    uint16_t tm_values[3] = { ARG_MASK_NUMBER, ARG_MASK_STRING, ARG_MASK_BOOLEAN };
                    Argument* tm_handlers[3] = { epc->numeric, epc->string, epc->boolean };

                    // we loop through our list of handlers, we save the first non-NULL handler encountered and
                    // then find more instances of that handler and build a typemask. We set the handlers in our
                    // list of handlers to NULL for each matching one so eventually we will have all NULL handlers
                    // and each distinct handler will have been checked.
                    arg = NULL;
                    while(arg==NULL && (tm_handlers[0]!=NULL || tm_handlers[1]!=NULL || tm_handlers[2]!=NULL)) {
                        int i;
                        uint16_t tm=0;
                        Argument *x=NULL, *y=NULL;
                        for(i=0; i<sizeof(tm_handlers)/sizeof(tm_handlers[0]); i++) {
                            if(tm_handlers[i]!=NULL) {
                                if(x==NULL) {
                                    // captured the first non-NULL element
                                    x = tm_handlers[i];
                                    tm |= tm_values[i];
                                    tm_handlers[i]=0;
                                } else if(x==tm_handlers[i]){
                                    // already encountered the first element, found a second match
                                    tm |= tm_values[i];
                                    tm_handlers[i]=0;
                                }
                            }
                        }

                        if(x!=NULL) {
                            uint16_t _typemask = (uint16_t)(((typemask & ARG_MASK_NUMBER)>0) ? typemask | ARG_MASK_NUMBER : typemask);
                            assert(tm>0); // must have gotten at least some typemask then
                            if(tm == _typemask) {
                                // exact match, we can jump to the endpoint
                                ev->ep = x->next;
                                arg = x;
                                if(ev->argtypes)
                                    ev->argtypes[ev->nargs++] = arg;    // add to list of args we encountered
                            } else if((tm & _typemask) >0) {
                                // uh-oh, user specified a rest endpoint that handles the same type but has differing
                                // endpoint targets. The actual target will be ambiquous.
                                return URL_FAIL_AMBIGUOUS_PARAMETER;
                            }
                            // otherwise no cross-over type match so this type handler can be ignored
                        }
                    }


                    if(arg==NULL) {
                        // add the argument to the Endpoint
                        arg = newArgument(name, typemask);
                        if(ev->argtypes) {
                            assert(ev->nargs < ev->szargs);
                            ev->argtypes[ev->nargs++] = arg;    // add to list of args we encountered
                        }
                        ev->ep = arg->next = newNode();

                        if ((typemask & ARG_MASK_NUMBER) > 0) {
                            // int or real
                            if (epc->numeric == NULL)
                                epc->numeric = arg;
                        }
                        if ((typemask & ARG_MASK_BOOLEAN) > 0) {
                            // boolean
                            if (epc->boolean == NULL)
                                epc->boolean = arg;
                        }
                        if ((typemask & ARG_MASK_STRING) > 0) {
                            // string
                            if (epc->string == NULL)
                                epc->string = arg;
                        }
                    }

#if 0   // i dont think it makes sense to accept defaults...how would that really work in URLs
                    // read a default if given
                    if(peek.id=='=') {
                        // read default value
                    }
#endif

                    NEXT_STATE( expectPathSep );

                    // recursively call parse so we can add more code at the end of this match
                    if((rv=parse(ev))!=0)
                        return rv;
                    return 0;   // inner recursive call would have completed call, so we are done too

                }
            } break;

            case expectEof:
                return (ev->t.id !=TID_EOF)
                        ? URL_FAIL_NO_ENDPOINT
                        : 0;
            case errorExpectedIdentifier:
                return -2;
                break;
            case errorExpectedIdentifierOrString:
                return -3;
                break;
        }

        // next token
        SCAN;
    }

    {
        // set the method name
        if (*(ev->pmethodName - 1) == '/')
            ev->pmethodName--;
        *ev->pmethodName = 0;
    }


done:
    return 0;
}

#if 0
short rest_uri_token_is_type(token t, uint32_t typemask)
{
    if(t.id==TID_INTEGER && (typemask & ARG_MASK_INTEGER)>0)
        return 1;
    else if(t.id==TID_FLOAT && (typemask & ARG_MASK_REAL)>0)
        return 1;
    else if(t.id==TID_BOOL && (typemask & ARG_MASK_BOOLEAN)>0)
        return 1;
    else if((t.id==TID_STRING || t.id==TID_IDENTIFIER) && (typemask & ARG_MASK_STRING)>0)
        return 1;
    else
        return 0;
}

json_object* json_object_from_token(token t)
{
    if(t.id==TID_INTEGER)
        return json_object_new_int64(t.i);
    if(t.id==TID_FLOAT)
        return json_object_new_double(t.d);
    if(t.id==TID_BOOL)
        return json_object_new_boolean(t.i>0);
    if(t.id==TID_STRING || t.id==TID_IDENTIFIER)
        return json_object_new_string(t.s);
    return NULL;
}
#endif

Endpoints::Endpoint Endpoints::add(HttpMethod method, const char *endpoint_expression, Handler handler)
//short rest_add_endpoint(UriExpression *expr, const UriEndpoint *endpoint)
{
    short rs;
    const char* uri = endpoint_expression;

    rest_uri_eval_data ev(this, &endpoint_expression);
    if(ev.state<0)
        return Endpoint(defaultHandler, URL_FAIL_INTERNAL);

    //ev.endpoint = endpoint;     // the endpoint constant we are adding
    ev.argtypes = (Argument**)calloc(ev.szargs = 20, sizeof(Argument));
    ev.mode = mode_add;         // tell the parser we are adding this endpoint

    if((rs = parse(&ev)) !=0) {
        printf("parse-eval-error %d   %s\n", rs, ev.uri);
        return Endpoint(defaultHandler, rs);
    } else {
        // if we encountered more args than we did before, then save the new value
        if(ev.nargs > maxUriArgs)
            maxUriArgs = ev.nargs;

        // attach the handler to this endpoint
        Node* epc = ev.ep;
        Endpoint endpoint;
        endpoint.name = ev.methodName;

        if(method == HttpMethodAny) {
            // attach to all remaining method handlers
            Handler* h = new Handler(handler);
            short matched = 0;
            if(!epc->GET) { epc->GET = h; matched++; }
            if(!epc->POST) { epc->POST = h; matched++; }
            if(!epc->PUT) { epc->PUT = h; matched++; }
            if(!epc->PATCH) { epc->PATCH = h; matched++; }
            if(!epc->DELETE) { epc->DELETE = h; matched++; }
            if(!epc->GET) { epc->GET = h; matched++; }
            endpoint.handler = *h;
            endpoint.status = matched ? URL_MATCHED : URL_FAIL_NO_HANDLER;
            return endpoint; // successfully added
        } else {
            Handler** target = nullptr;

            // get a pointer to the Handler member variable from the node
            switch(method) {
                case HttpGet: target = &epc->GET; break;
                case HttpPost: target = &epc->POST; break;
                case HttpPut: target = &epc->PUT; break;
                case HttpPatch: target = &epc->PATCH; break;
                case HttpDelete: target = &epc->DELETE; break;
                case HttpOptions: target = &epc->OPTIONS; break;
                default:
                    return Endpoint(defaultHandler, URL_FAIL_INTERNAL); // unknown method type
            }

            if(target !=nullptr) {
                // set the target method handler but error if it was already set by a previous endpoint declaration
                if(*target !=nullptr ) {
                    fprintf(stderr, "fatal: endpoint %s %s was previously set to a different handler\n",
                            uri_method_to_string(method), endpoint_expression);
                    return Endpoint(defaultHandler, URL_FAIL_DUPLICATE);
                } else {
                    *target = new Handler(handler);
                    endpoint.handler = **target;
                    endpoint.status = URL_MATCHED;
                    return endpoint; // successfully added
                }
            } else
                return Endpoint(defaultHandler, URL_FAIL_NO_HANDLER);
        }
    }
}

Endpoints::Endpoint Endpoints::resolve(uint32_t method, const char *uri)
{
    short rs;
    rest_uri_eval_data ev(this, &uri);
    if(ev.state<0)
        return Endpoint(defaultHandler, URL_FAIL_INTERNAL);
    ev.args = new ArgumentValue[ev.szargs = maxUriArgs];

    // parse the input
    if((rs=parse( &ev )) ==0) {
        // successfully resolved the endpoint
        Endpoint endpoint;
        Handler* handler;
        switch(method) {
            case HttpGet: handler = ev.ep->GET; break;
            case HttpPost: handler = ev.ep->POST; break;
            case HttpPut: handler = ev.ep->PUT; break;
            case HttpPatch: handler = ev.ep->PATCH; break;
            case HttpDelete: handler = ev.ep->DELETE; break;
            case HttpOptions: handler = ev.ep->OPTIONS; break;
            default: handler = &defaultHandler;
        }

        if(handler !=nullptr) {
            endpoint.status = URL_MATCHED;
            endpoint.handler = *handler;
            endpoint.name = ev.methodName;
            endpoint.args = ev.args;
            endpoint.nargs = ev.nargs;
        } else {
            endpoint.handler = defaultHandler;
            endpoint.status = URL_FAIL_NO_HANDLER;
        }
        return endpoint;
    } else {
        // todo: we shouldnt print the error
        printf("parse-eval-error %d   %s\n", rs, ev.uri);
        return Endpoint(defaultHandler, URL_FAIL_NO_ENDPOINT);
    }
    // todo: we need to release memory from EV struct
}

#if 0
yarn* rest_uri_debug_print_internal(UriExpression* expr, rest_uri_eval_data* ev, yarn* out, int level)
{
    Endpoint* ep = ev->ep;
    Argument* arg;
    {
        // print what method handlers are attached to this endpoint, if any
        if(ep->GET!=NULL | ep->POST!=NULL | ep->PUT!=NULL | ep->PATCH!=NULL | ep->DELETE!=NULL | ep->OPTIONS!=NULL) {
            yarn_printf(out, "[");
            yarn_join(out, ',', (const char* []){
                    ep->GET?"GET":NULL,
                    ep->POST?"POST":NULL,
                    ep->PUT?"PUT":NULL,
                    ep->PATCH?"PATCH":NULL,
                    ep->DELETE?"DELETE":NULL,
                    ep->OPTIONS?"OPTIONS":NULL
            }, 6);
            yarn_printf(out, "]");
        }

        // loop through all literals that are acceptable from this endpoint
        Literal* lit = ep->literals;
        if(lit) {
            // if we have more than 1 literal, then put on a second line
            BOOL multiple = rest_ep_literal_count(lit) >1;

            while (rest_ep_literal_isValid(lit)) {
                if(multiple)
                    yarn_printf(out, "\n%*s ·", level*2, "");

                if (lit->isNumeric)
                    yarn_printf(out, "/%d", lit->id);
                else
                    yarn_printf(out, "/%s", binbag_get(expr->text, lit->id));

                // recurse if we have more endpoints to travel
                if (lit->next) {
                    ev->ep = lit->next;
                    rest_uri_debug_print_internal(expr, ev, out, level+1);
                    ev->ep = ep;
                }
                lit++;
            }
        }

        // figure out what types are grouped together
        // determine the typemask of any already set handlers at this endpoint.
        // we cannot have two different handlers that handle the same type, but if the typemask
        // exactly matches we can just consider a match and jump to that endpoint.
        uint16_t tm_values[3] = { ARG_MASK_NUMBER, ARG_MASK_STRING, ARG_MASK_BOOLEAN };
        Argument* tm_handlers[3] = { ep->numeric, ep->string, ep->boolean };

        // we loop through our list of handlers, we save the first non-NULL handler encountered and
        // then find more instances of that handler and build a typemask. We set the handlers in our
        // list of handlers to NULL for each matching one so eventually we will have all NULL handlers
        // and each distinct handler will have been checked.
        arg = NULL;
        while(arg==NULL && (tm_handlers[0]!=NULL || tm_handlers[1]!=NULL || tm_handlers[2]!=NULL)) {
            int i;
            uint16_t tm=0;
            Argument *x=NULL, *y=NULL;
            for(i=0; i<sizeof(tm_handlers)/sizeof(tm_handlers[0]); i++) {
                if(tm_handlers[i]!=NULL) {
                    if(x==NULL) {
                        // captured the first non-NULL element
                        x = tm_handlers[i];
                        tm |= tm_values[i];
                        tm_handlers[i]=0;
                    } else if(x==tm_handlers[i]){
                        // already encountered the first element, found a second match
                        tm |= tm_values[i];
                        tm_handlers[i]=0;
                    }
                }
            }

            if(x!=NULL) {
                size_t i;
                char s[128];
                char* types[3] = {0};

                assert(tm>0); // must have gotten at least some typemask then
                if((tm & ARG_MASK_NUMBER) == ARG_MASK_INTEGER) {
                    sprintf(s, "%s:int", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & ARG_MASK_NUMBER) == ARG_MASK_REAL) {
                    sprintf(s, "%s:real", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & ARG_MASK_NUMBER) == ARG_MASK_NUMBER) {
                    sprintf(s, "%s:number", ep->numeric->name);
                    types[0] = strdup(s);
                }
                if((tm & ARG_MASK_STRING) == ARG_MASK_STRING) {
                    sprintf(s, "%s:string", ep->string->name);
                    types[1] = strdup(s);
                }
                if((tm & ARG_MASK_BOOLEAN) == ARG_MASK_BOOLEAN) {
                    sprintf(s, "%s:boolean", ep->boolean->name);
                    types[2] = strdup(s);
                }

                yarn_printf(out, "\n%*s ·/<", level*2, "");
                yarn_join(out, '|', types, sizeof(types)/sizeof(types[0]));
                yarn_printf(out, ">");

                for(i=0; i<sizeof(types)/sizeof(types[0]); i++)
                    if(types[i]!=NULL)
                        free(types[i]);

                // recurse if we have more endpoints to travel
                if (x->next) {
                    ev->ep = x->next;
                    rest_uri_debug_print_internal(expr, ev, out, level+1);
                    ev->ep = ep;
                }

            }
        }

    }
    return out;
}

yarn* rest_uri_debug_print(UriExpression* expr, yarn* out)
{
    rest_uri_eval_data ev;
    if(rest_uri_init_eval_data(expr, &ev, NULL)!=0)
        return NULL;
    if(out==NULL)
        out = yarn_create(1000);
    return rest_uri_debug_print_internal(expr, &ev, out, 0);
}
#endif


Endpoints::Literal* Endpoints::newLiteral(Node* ep, Literal* literal)
{
    Literal* _new, *p;
    int _bb_text;
    int _insert;
    if(ep->literals) {
        // todo: this kind of realloc every Literal insert will cause memory fragmentation, use Endpoints shared mem
        // find the end of this list
        Literal *_list = ep->literals;
        while (_list->isValid())
            _list++;
        _insert = (int)(_list - ep->literals);

        // allocate a new list
        _new = (Literal*)realloc(ep->literals, (_insert + 2) * sizeof(Literal));
        //memset(_new+_insert+1, 0, sizeof(Literal));
    } else {
        _new = (Literal*)calloc(2, sizeof(Literal));
        _insert = 0;
    };

    // insert the new literal
    memcpy(_new + _insert, literal, sizeof(Literal));
    ep->literals = _new;

    p = &_new[_insert + 1];
    p->id = -1;
    p->isNumeric = false;
    p->next = NULL;

    return _new + _insert;
}


Endpoints::Literal* Endpoints::addLiteralString(Node* ep, const char* literal_value)
{
    Literal lit;
    lit.isNumeric = false;
    if((lit.id = binbag_find_nocase(text, literal_value)) <0)
        lit.id = binbag_insert(text, literal_value);  // insert value into the binbag, and record the index into the id field
    lit.next = NULL;
    return newLiteral(ep, &lit);
}

Endpoints::Literal* Endpoints::addLiteralNumber(Node* ep, ssize_t literal_value)
{
    Literal lit;
    lit.isNumeric = true;
    lit.id = literal_value;
    lit.next = NULL;
    return newLiteral(ep, &lit);
}


