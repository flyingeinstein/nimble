//
// Created by ineoquest on 2/2/18.
//

#include "Rest.h"
#include "RestInternal.h"


#include <string.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


short rest_uri_init_eval_data(UriExpression* expr, rest_uri_eval_data* ev, const char** uri)
{
    memset(ev, 0, sizeof(rest_uri_eval_data));
    ev->pmethodName = ev->methodName;
    ev->ep = expr->ep_head;
    ev->mode = mode_resolve;
    ev->arguments = json_object_new_object();

    memset(&ev->t, 0, sizeof(token));
    memset(&ev->peek, 0, sizeof(token));

    if(uri!=NULL) {
        // scan first token
        if (!rest_uri_scanner(&ev->t, uri, 1))
            return -1;
        if (!rest_uri_scanner(&ev->peek, uri, 1))
            return -1;
        ev->uri = *uri;
    }
    return 0;
}

const char* uri_method_to_string(uint32_t method) {
    switch(method) {
        case MG_GET: return "GET";
        case MG_POST: return "POST";
        case MG_PUT: return "PUT";
        case MG_PATCH: return "PATCH";
        case MG_DELETE: return "DELETE";
        case MG_OPTIONS: return "OPTIONS";
        case MG_ANY: return "ANY";
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

void token_clear(token* token)
{
    if(token && token->s) {
        if(token->id >=500)
            free(token->s);
    }
    token->id = 0;
    token->s = NULL;
    token->i = 0;
    token->d = 0.0;
}

void token_set_string(token* t, short id, const char* _begin, const char* _end)
{
    assert(id >= 500);  // only IDs above 500 can store a string
    t->id = id;
    if(_end == NULL) {
        t->s = strdup(_begin);
    } else {
        t->s = (char *) calloc(1, _end - _begin + 1);
        memcpy(t->s, _begin, _end - _begin);
    }
}


//short scanner(const char **input, char *token)
int rest_uri_scanner(token* t, const char** pinput, short allow_parameters)
{
    const char* input = *pinput;
    char error[512];

    token_clear(t);

    if(*input==0) {
        t->id = TID_EOF;
        *pinput = input;
        return 0;
    }

    // check for single character token
    // note: if we find a single char token we break and then return, otherwise (default) we jump over
    // to check for longer token types like keywords and attributes
    if(strchr("/", *input)!=NULL) {
        t->s = NULL;
        t->id = *input++;
        goto done;
    } else if(allow_parameters && strchr("=:?(|)", *input)!=NULL) {
        // these symbols are allowed when we are scanning a Rest URL match expression
        // but are not valid in normal URLs, or at least considered part of normal URL matching below
        t->s = NULL;
        t->id = *input++;
        goto done;
    }


    // check for literal float
    if(input[0]=='.') {
        if(isdigit(input[1])) {
            // decimal number
            char *p;
            t->id = TID_FLOAT;
            t->d = strtod(input, &p);
            t->i = (int64_t) t->d;
            input = p;
            goto done;
        } else {
            // plain dot symbol
            t->id = '.';
            t->s = NULL;
            input++;
            goto done;
        }
    } else if(input[0]=='0' && input[1]=='x') {
        // hex constant
        char* p;
        t->id = TID_INTEGER;
        t->i = (int64_t)strtoll(input, &p, 16);
        input = p;
        goto done;
    } else if(isdigit(*input)) {
//scan_number:
        // integer or float constant
        char *p;
        t->id = TID_INTEGER;
        t->i = (int64_t)strtoll(input, &p, 0);
        if (*p == '.') {
            t->id = TID_FLOAT;
            t->d = strtod(input, &p);
            input = p;
        } else
            input = p;
        goto done;
    }
        // check for boolean value
    else if(strncasecmp(input, "false", 5) ==0 && !isalnum(input[5])) {
        input += 5;
        t->id = TID_BOOL;
        t->i = 0;
        goto done;
    }
    else if(strncasecmp(input, "true", 4) ==0 && !isalnum(input[4])) {
        input += 4;
        t->id = TID_BOOL;
        t->i = 1;
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
        token_set_string(t, ident, p, input);
        goto done;
    }

    sprintf(error, "syntax error, unexpected '%c' in input", *input);
    input++;
    token_set_string(t, TID_ERROR, error, NULL);

done:
    *pinput = input;
    return 1;
}

UriExpression* uri_endpoint_init(int elements)
{
    UriExpression* ue = (UriExpression*)calloc(1, sizeof(UriExpression));

    memset(ue, 0, sizeof(UriExpression));

    ue->text = binbag_create(1000, 1.5);

    ue->ep_head = ue->ep_tail = ue->ep_end =  (Endpoint*)calloc(elements, sizeof(Endpoint));
    ue->ep_end++; // create root endpoint

    return ue;
}

#define GOTO_STATE(st) { ev->state = st; goto rescan; }
#define NEXT_STATE(st) { ev->state = st; }
#define SCAN { token_clear(&ev->t); ev->t = ev->peek; ev->peek.id=0; rest_uri_scanner(&ev->peek, &ev->uri, 1); }


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

short rest_uri_parse_endpoint_internal(UriExpression* expr, rest_uri_eval_data* ev)
{
    short rv;
    uint64_t ptr;
    long wid;
    Endpoint* epc = ev->ep;
    EndpointLiteral* lit;
    EndpointArgument* arg;

    // read datatype or decl type
    while(ev->t.id!=TID_EOF) {
    rescan:
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
                    SCAN;
                    if((rv=rest_uri_parse_endpoint_internal(expr, ev))!=0)
                        return rv;

                    // we can post-evaluate the branch we just parsed here
                    return 0;
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
                    wid = binbag_find_nocase(expr->text, ev->t.s);
                    if(wid>=0 && epc->literals) {
                        // word exists in dictionary, see if it is a literal of current endpoint
                        lit = epc->literals;
                        while(rest_ep_literal_isValid(lit) && lit->id!=wid)
                            lit++;
                        if(lit->id!=wid)
                            lit=NULL;
                    }

                    // add the new literal if we didnt find an existing one
                    if(lit==NULL) {
                        if(ev->mode == mode_add) {
                            // regular URI word, add to lexicon and generate code
                            lit = rest_ep_add_literal_string(ev->ep, expr->text, ev->t.s);
                            ev->ep = lit->next = rest_ep_new(expr);
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

                    // recursively call parse so we can add more code at the end of this match
                    if((rv=rest_uri_parse_endpoint_internal(expr, ev))!=0)
                        return rv;
                    return 0;   // inner recursive call would have completed call, so we are done too
                } else if(ev->mode==mode_resolve) {
                    GOTO_STATE(expectParameterValue);
                } else
                    NEXT_STATE( errorExpectedIdentifierOrString );
            } break;
            case expectParameterValue: {
                const char* typename=NULL;

                // try to match a parameter by type
                if((ev->t.id==TID_STRING || ev->t.id==TID_IDENTIFIER) && epc->string!=NULL) {
                    // we can match by string argument type (parameter match)
                    json_object_object_add(ev->arguments, epc->string->name, json_object_new_string(ev->t.s));
                    ev->ep = epc->string->next;
                    typename = "string";
                } else if(ev->t.id==TID_INTEGER && epc->numeric!=NULL) {
                    // numeric argument
                    json_object_object_add(ev->arguments, epc->numeric->name, json_object_new_int64(ev->t.i));
                    ev->ep = epc->numeric->next;
                    typename = "int";
                } else if(ev->t.id==TID_FLOAT && epc->numeric!=NULL) {
                    // numeric argument
                    json_object_object_add(ev->arguments, epc->numeric->name, json_object_new_double(ev->t.d));
                    ev->ep = epc->numeric->next;
                    typename = "float";
                } else if(ev->t.id==TID_BOOL && epc->boolean!=NULL) {
                    // numeric argument
                    json_object_object_add(ev->arguments, epc->boolean->name, json_object_new_boolean(ev->t.i>0));
                    ev->ep = epc->boolean->next;
                    typename = "boolean";
                } else
                    NEXT_STATE( errorExpectedIdentifierOrString ); // no match by type

                // add component to method name
                *ev->pmethodName++ = '<';
                strcpy(ev->pmethodName, typename);
                ev->pmethodName += strlen(typename);
                *ev->pmethodName++ = '>';
                *ev->pmethodName = 0;

                // successful match, jump to next endpoint node
                NEXT_STATE( expectPathSep );
                SCAN;

                // recursively call parse so we can add more code at the end of this match
                if((rv=rest_uri_parse_endpoint_internal(expr, ev))!=0)
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
                                typemask |= JSON_BITMASK_INTEGER;
                            else if(strcmp(ev->t.s, "real")==0)
                                typemask |= JSON_BITMASK_REAL;
                            else if(strcmp(ev->t.s, "number")==0)
                                typemask |= JSON_BITMASK_NUMBER;
                            else if(strcmp(ev->t.s, "string")==0)
                                typemask |= JSON_BITMASK_STRING;
                            else if(strcmp(ev->t.s, "boolean")==0 || strcmp(ev->t.s, "bool")==0)
                                typemask |= JSON_BITMASK_BOOLEAN;
                            else if(strcmp(ev->t.s, "any")==0)
                                typemask |= JSON_BITMASK_ANY;
                            else
                                return URL_FAIL_INVALID_TYPE;

                            SCAN;
                        } while(ev->t.id=='|');

                        // expect closing tag
                        if(ev->t.id !=')')
                            return URL_FAIL_SYNTAX;
                    } else typemask = JSON_BITMASK_ANY;

                    // determine the typemask of any already set handlers at this endpoint.
                    // we cannot have two different handlers that handle the same type, but if the typemask
                    // exactly matches we can just consider a match and jump to that endpoint.
                    uint16_t tm_values[3] = { JSON_BITMASK_NUMBER, JSON_BITMASK_STRING, JSON_BITMASK_BOOLEAN };
                    EndpointArgument* tm_handlers[3] = { epc->numeric, epc->string, epc->boolean };

                    // we loop through our list of handlers, we save the first non-NULL handler encountered and
                    // then find more instances of that handler and build a typemask. We set the handlers in our
                    // list of handlers to NULL for each matching one so eventually we will have all NULL handlers
                    // and each distinct handler will have been checked.
                    arg = NULL;
                    while(arg==NULL && (tm_handlers[0]!=NULL || tm_handlers[1]!=NULL || tm_handlers[2]!=NULL)) {
                        int i;
                        uint16_t tm=0;
                        EndpointArgument *x=NULL, *y=NULL;
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
                            uint16_t _typemask = (uint16_t)(((typemask & JSON_BITMASK_NUMBER)>0) ? typemask | JSON_BITMASK_NUMBER : typemask);
                            assert(tm>0); // must have gotten at least some typemask then
                            if(tm == _typemask) {
                                // exact match, we can jump to the endpoint
                                ev->ep = x->next;
                                arg = x;
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
                        arg = calloc(1, sizeof(EndpointArgument));
                        arg->name = strdup(name);
                        ev->ep = arg->next = rest_ep_new(expr);

                        if ((typemask & JSON_BITMASK_NUMBER) > 0) {
                            // int or real
                            if (epc->numeric == NULL)
                                epc->numeric = arg;
                        }
                        if ((typemask & JSON_BITMASK_BOOLEAN) > 0) {
                            // boolean
                            if (epc->boolean == NULL)
                                epc->boolean = arg;
                        }
                        if ((typemask & JSON_BITMASK_STRING) > 0) {
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
                    if((rv=rest_uri_parse_endpoint_internal(expr, ev))!=0)
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

short rest_uri_token_is_type(token t, uint32_t typemask)
{
    if(t.id==TID_INTEGER && (typemask & JSON_BITMASK_INTEGER)>0)
        return 1;
    else if(t.id==TID_FLOAT && (typemask & JSON_BITMASK_REAL)>0)
        return 1;
    else if(t.id==TID_BOOL && (typemask & JSON_BITMASK_BOOLEAN)>0)
        return 1;
    else if((t.id==TID_STRING || t.id==TID_IDENTIFIER) && (typemask & JSON_BITMASK_STRING)>0)
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


yarn* rest_uri_debug_print_internal(UriExpression* expr, rest_uri_eval_data* ev, yarn* out, int level)
{
    Endpoint* ep = ev->ep;
    EndpointArgument* arg;
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
        EndpointLiteral* lit = ep->literals;
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
        uint16_t tm_values[3] = { JSON_BITMASK_NUMBER, JSON_BITMASK_STRING, JSON_BITMASK_BOOLEAN };
        EndpointArgument* tm_handlers[3] = { ep->numeric, ep->string, ep->boolean };

        // we loop through our list of handlers, we save the first non-NULL handler encountered and
        // then find more instances of that handler and build a typemask. We set the handlers in our
        // list of handlers to NULL for each matching one so eventually we will have all NULL handlers
        // and each distinct handler will have been checked.
        arg = NULL;
        while(arg==NULL && (tm_handlers[0]!=NULL || tm_handlers[1]!=NULL || tm_handlers[2]!=NULL)) {
            int i;
            uint16_t tm=0;
            EndpointArgument *x=NULL, *y=NULL;
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
                if((tm & JSON_BITMASK_NUMBER) == JSON_BITMASK_INTEGER) {
                    sprintf(s, "%s:int", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & JSON_BITMASK_NUMBER) == JSON_BITMASK_REAL) {
                    sprintf(s, "%s:real", ep->numeric->name);
                    types[0] = strdup(s);
                }
                else if((tm & JSON_BITMASK_NUMBER) == JSON_BITMASK_NUMBER) {
                    sprintf(s, "%s:number", ep->numeric->name);
                    types[0] = strdup(s);
                }
                if((tm & JSON_BITMASK_STRING) == JSON_BITMASK_STRING) {
                    sprintf(s, "%s:string", ep->string->name);
                    types[1] = strdup(s);
                }
                if((tm & JSON_BITMASK_BOOLEAN) == JSON_BITMASK_BOOLEAN) {
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

short rest_add_endpoints(UriExpression *expr, const UriEndpoint *endpoints, int count)
{
    short r;
    while(count-- >0) {
        if((r= rest_add_endpoint(expr, endpoints)) <0)
            return r;
        endpoints++;
    }
    return 0;
}

short rest_add_endpoint(UriExpression *expr, const UriEndpoint *endpoint)
{
    short rs;
    rest_uri_eval_data ev;
    const char* uri = endpoint->uri;
    if(rest_uri_init_eval_data(expr, &ev, &uri)!=0)
        return -1;
    ev.endpoint = endpoint;     // the endpoint constant we are adding
    ev.mode = mode_add;         // tell the parser we are adding this endpoint

    if((rs = rest_uri_parse_endpoint_internal(expr, &ev)) !=0) {
        printf("parse-eval-error %d   %s\n", rs, ev.uri);
        return rs;
    } else {
        // attach the handlers to this endpoint
        long wid = 0;
        Endpoint* epc = ev.ep;
        const UriEndpointMethodHandler* handler = ev.endpoint->handlers;
        while(handler->handler && wid<6) {
            RestMethodHandler* target = NULL;
            switch(handler->method) {
                case MG_GET: target = &epc->GET; break;
                case MG_POST: target = &epc->POST; break;
                case MG_PUT: target = &epc->PUT; break;
                case MG_PATCH: target = &epc->PATCH; break;
                case MG_DELETE: target = &epc->DELETE; break;
                case MG_OPTIONS: target = &epc->OPTIONS; break;
                case MG_ANY:
                    // attach to all remaining method handlers
                    if(!epc->GET) epc->GET = handler->handler;
                    if(!epc->POST) epc->POST = handler->handler;
                    if(!epc->PUT) epc->PUT = handler->handler;
                    if(!epc->PATCH) epc->PATCH = handler->handler;
                    if(!epc->DELETE) epc->DELETE = handler->handler;
                    if(!epc->GET) epc->GET = handler->handler;
                    break;
                default:
                    assert(FALSE); // unknown method given
            }
            if(target !=NULL) {
                // set the target method handler but error if it was already set by a previous endpoint declaration
                if(*target !=NULL && *target != handler->handler) {
                    fprintf(stderr, "fatal: endpoint %s %s was previously set to a different handler\n",
                            uri_method_to_string(handler->method), endpoint->uri);
                    abort();
                } else
                    *target = handler->handler;
            }
            handler++;
            wid++;
        }
    }

    return 0;
}

short rest_uri_resolve_endpoint(UriExpression *expr, uint32_t method, const char *uri,
                                ResolvedUriEndpoint *endpoint_out)
{
    short rs;
    rest_uri_eval_data ev;

    // set head resolved Uri endpoint output
    if (endpoint_out != NULL) {
        memset(endpoint_out, 0, sizeof(ResolvedUriEndpoint));
        endpoint_out->method = method;
        endpoint_out->requestUri = strdup(uri);
    }

    // setup the parser with initial state
    // FYI this eats two tokens of 'uri' variable
    if(rest_uri_init_eval_data(expr, &ev, &uri)!=0)
        return -1;

    // parse the input
    if((rs=rest_uri_parse_endpoint_internal(expr, &ev )) ==0) {
        // successfully resolved the endpoint
        if(endpoint_out!=NULL) {
            Endpoint* epc = ev.ep;
            switch(method) {
                case MG_GET: endpoint_out->handler = epc->GET; break;
                case MG_POST: endpoint_out->handler = epc->POST; break;
                case MG_PUT: endpoint_out->handler = epc->PUT; break;
                case MG_PATCH: endpoint_out->handler = epc->PATCH; break;
                case MG_DELETE: endpoint_out->handler = epc->DELETE; break;
                case MG_OPTIONS: endpoint_out->handler = epc->OPTIONS; break;
                default: return URL_FAIL_NO_HANDLER;
            }

            endpoint_out->name = strdup(ev.methodName);
            endpoint_out->arguments = json_object_get(ev.arguments);
            if(endpoint_out->handler==NULL)
                return URL_FAIL_NO_HANDLER;
        }
        return URL_MATCHED;
    } else {
        // todo: we shouldnt print the error
        printf("parse-eval-error %d   %s\n", rs, ev.uri);
        return URL_FAIL_NO_ENDPOINT;
    }
    // todo: we need to release memory from EV struct
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


const char* request_param_string(JsonRequest* request, const char* name)
{
     if(request->request !=NULL) {
        json_object* obj;
        if(json_object_object_get_ex(request->request, name, &obj) && (obj!=NULL) && json_object_get_type(obj)==json_type_string) {
            return json_object_get_string(obj);
        }
    }
    return NULL;
}

const char* query_param_string(JsonRequest* request, const char* name)
{
    if(request->query !=NULL) {
        json_object* obj;
        if(json_object_object_get_ex(request->query, name, &obj) && (obj!=NULL) && json_object_get_type(obj)==json_type_string) {
            return json_object_get_string(obj);
        }
    }
    return NULL;
}

BOOL query_param_u32(JsonRequest* request, const char* name, UINT32* value_out)
{
    if(request->query !=NULL) {
        json_object* obj;
        if(json_object_object_get_ex(request->query, name, &obj) && (obj!=NULL)) {
            json_type t = json_object_get_type(obj);
            if (t==json_type_int) {
                if(value_out)
                    *value_out = (UINT32)json_object_get_int64(obj);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL query_param_i64(JsonRequest* request, const char* name, INT64* value_out)
{
    if(request->query !=NULL) {
        json_object* obj;
        if(json_object_object_get_ex(request->query, name, &obj) && (obj!=NULL)) {
            json_type t = json_object_get_type(obj);
            if (t==json_type_int) {
                if(value_out)
                    *value_out = json_object_get_int64(obj);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL query_param_bool(JsonRequest* request, const char* name, BOOL default_value)
{
    if(request->query !=NULL) {
        json_object* obj;
        const char* value;
        if(json_object_object_get_ex(request->query, name, &obj) && (obj!=NULL)) {
            json_type t = json_object_get_type(obj);
            switch(t) {
                case json_type_boolean:
                    return json_object_get_boolean(obj) ? TRUE : FALSE;

                case json_type_int:
                    return (json_object_get_int64(obj)>0) ? TRUE : FALSE;

                case json_type_string:
                    value = json_object_get_string(obj);
                    return (strcasecmp(value, "t") || strcasecmp(value, "true")) ? TRUE : FALSE;

                default:
                    return default_value;
            }
        }
    }
    return default_value;
}




BOOL rest_ep_literal_isValid(EndpointLiteral* lit)
{
    return lit->isNumeric || (lit->id>=0);
}

int rest_ep_literal_count(EndpointLiteral* lit)
{
    int c=0;
    while(rest_ep_literal_isValid(lit++))
        c++;
    return c;
}

EndpointLiteral* rest_ep_add_literal(Endpoint* ep, EndpointLiteral* literal)
{
    EndpointLiteral* _new, *p;
    EndpointLiteral* L[4];
    int _bb_text;
    int _insert;
    memset(L, 0, sizeof(L));
    if(ep->literals) {
        L[0] = &ep->literals[0];
        L[1] = &ep->literals[1];
        L[2] = &ep->literals[2];
        L[3] = &ep->literals[3];

        // find the end of this list
        EndpointLiteral *_list = ep->literals;
        while (rest_ep_literal_isValid(_list))
            _list++;
        _insert = (int)(_list - ep->literals);

        // allocate a new list
        _new = (EndpointLiteral*)realloc(ep->literals, (_insert + 2) * sizeof(EndpointLiteral));
        //memset(_new+_insert+1, 0, sizeof(EndpointLiteral));
    } else {
        _new = calloc(2, sizeof(EndpointLiteral));
        _insert = 0;
    };

    // insert the new literal
    memcpy(_new + _insert, literal, sizeof(EndpointLiteral));
    ep->literals = _new;

    p = &_new[_insert + 1];
    p->id = -1;
    p->isNumeric = FALSE;
    p->next = NULL;

    return _new + _insert;
}

Endpoint* rest_ep_new(UriExpression* exp)
{
    Endpoint* pnew = exp->ep_end++;
    memset(pnew, 0, sizeof(Endpoint));
    return pnew;
}

EndpointLiteral* rest_ep_add_literal_string(Endpoint* ep, binbag* bb, const char* literal_value)
{
    EndpointLiteral lit;
    lit.isNumeric = FALSE;
    if((lit.id = binbag_find_nocase(bb, literal_value)) <0)
        lit.id = binbag_insert(bb, literal_value);  // insert value into the binbag, and record the index into the id field
    lit.next = NULL;
    return rest_ep_add_literal(ep, &lit);
}

EndpointLiteral* rest_ep_add_literal_number(Endpoint* ep, binbag* bb, ssize_t literal_value)
{
    EndpointLiteral lit;
    lit.isNumeric = TRUE;
    lit.id = literal_value;
    lit.next = NULL;
    return rest_ep_add_literal(ep, &lit);
}


