#include "http.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "strutils.h"
#include "tracelog.h"

// == INTERNAL ================================================================

static void _setField(http_field_t **fields_ptr, int *fields_count_ptr, const char *key, const char *value) {
    http_field_t *fields = *fields_ptr;
    int fields_count = *fields_count_ptr;

    // search if the field already exists
    for(int i = 0; i < fields_count; ++i) {
        if(stricmp(fields[i].key, key) == 0) {
            // replace value
            char **curval = &fields[i].value;
            size_t cur = strlen(*curval);
            size_t new = strlen(value);
            if(new > cur) {
                *curval = realloc(*curval, new + 1);
            }
            memcpy(*curval, value, new);
            (*curval)[new] = '\0';
            return;
        }
    }
    // otherwise, add it to the list
    (*fields_count_ptr)++;;
    (*fields_ptr) = realloc(fields, sizeof(http_field_t) * (*fields_count_ptr));
    http_field_t *field = &(*fields_ptr)[(*fields_count_ptr) - 1];
    
    size_t klen = strlen(key);
    size_t vlen = strlen(value);
    field->key = malloc(klen + 1);
    field->value = malloc(vlen + 1);
    memcpy(field->key, key, klen);
    memcpy(field->value, value, vlen);
    field->key[klen] = field->value[vlen] = '\0';
}

// == HTTP VERSION ============================================================

int httpVerNumber(http_version_t ver) {
    return (ver.major * 10) + ver.minor;
}

// == HTTP REQUEST ============================================================

http_request_t reqInit() {
    http_request_t req;
    memset(&req, 0, sizeof(req));
    reqSetUri(&req, "/");
    req.version = (http_version_t){1, 1};
    return req;
}

void reqFree(http_request_t *ctx) {
    for(int i = 0; i < ctx->field_count; ++i) {
        free(ctx->fields[i].key);
        free(ctx->fields[i].value);
    }
    free(ctx->fields);
    free(ctx->uri);
    free(ctx->body);
    memset(ctx, 0, sizeof(http_request_t));
}

bool reqHasField(http_request_t *ctx, const char *key) {
    for(int i = 0; i < ctx->field_count; ++i) {
        if(stricmp(ctx->fields[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

void reqSetField(http_request_t *ctx, const char *key, const char *value) {
    _setField(&ctx->fields, &ctx->field_count, key, value);
}

void reqSetUri(http_request_t *ctx, const char *uri) {
    if(uri == NULL) return;
    size_t len = strlen(uri);
    if(uri[0] != '/') {
        len += 1;
        ctx->uri = realloc(ctx->uri, len + 1);
        ctx->uri[0] = '/';
        memcpy(ctx->uri + 1, uri, len);
        ctx->uri[len] = '\0';
    }
    else {
        ctx->uri = realloc(ctx->uri, len + 1);
        memcpy(ctx->uri, uri, len);
        ctx->uri[len] = '\0';
    }
}

str_ostream_t reqPrepare(http_request_t *ctx) {
    str_ostream_t out = ostrInitLen(1024);

    const char *method = NULL;
    switch(ctx->method) {
    case REQ_GET:    method = "GET";    break;
    case REQ_POST:   method = "POST";   break;
    case REQ_HEAD:   method = "HEAD";   break;
    case REQ_PUT:    method = "PUT";    break;
    case REQ_DELETE: method = "DELETE"; break;
    default: err("unrecognized method: %d", method); goto error;
    }

    ostrPrintf(&out, "%s %s HTTP/%hhu.%hhu\r\n", 
        method, ctx->uri, ctx->version.major, ctx->version.minor
    );

    for(int i = 0; i < ctx->field_count; ++i) {
        ostrPrintf(&out, "%s: %s\r\n", ctx->fields[i].key, ctx->fields[i].value);
    }

    ostrAppendview(&out, strvInit("\r\n"));
    if(ctx->body) {
        ostrAppendview(&out, strvInit(ctx->body));
    }

error:
    return out;
}

size_t reqString(http_request_t *ctx, char **str) {
    str_ostream_t out = reqPrepare(ctx);
    return ostrMove(&out, str);
}

// == HTTP RESPONSE ===========================================================

http_response_t resInit() {
    http_response_t res;
    memset(&res, 0, sizeof(res));
    return res;
}   

void resFree(http_response_t *ctx) {
    for(int i = 0; i < ctx->field_count; ++i) {
        free(ctx->fields[i].key);
        free(ctx->fields[i].value);
    }
    free(ctx->fields);
    free(ctx->body);
    memset(ctx, 0, sizeof(http_response_t));
}

bool resHasField(http_response_t *ctx, const char *key) {
    for(int i = 0; i < ctx->field_count; ++i) {
        if(stricmp(ctx->fields[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

const char *resGetField(http_response_t *ctx, const char *field) {
    for(int i = 0; i < ctx->field_count; ++i) {
        if(stricmp(ctx->fields[i].key, field) == 0) {
            return ctx->fields[i].value;
        }
    }
    return NULL;
}

void resParse(http_response_t *ctx, const char *data) {
    str_istream_t in = istrInit(data);

    char hp[5];
    istrGetstringBuf(&in, hp, 5);
    if(stricmp(hp, "http") != 0) {
        err("response doesn't start with 'HTTP', instead with %c%c%c%c", hp[0], hp[1], hp[2], hp[3]);
        return;
    }
    istrSkip(&in, 1); // skip /
    istrGetu8(&in, &ctx->version.major);
    istrSkip(&in, 1); // skip .
    istrGetu8(&in, &ctx->version.minor);
    istrGeti32(&in, &ctx->status_code);

    istrIgnore(&in, '\n');
    istrSkip(&in, 1); // skip \n

    resParseFields(ctx, &in);

    const char *tran_encoding = resGetField(ctx, "transfer-encoding");
    if(tran_encoding == NULL || stricmp(tran_encoding, "chunked")  != 0) {
        strview_t body = istrGetviewLen(&in, 0, SIZE_MAX);
        free(ctx->body);
        strvCopy(body, &ctx->body);
    }
    else {
        fatal("chunked encoding not implemented yet");
    }
}

void resParseFields(http_response_t *ctx, str_istream_t *in) {
    strview_t line;

    do {
        line = istrGetview(in, '\r');

        size_t pos = strvFind(line, ':', 0);
        if(pos != STRV_NOT_FOUND) {
            strview_t key = strvSubstr(line, 0, pos);
            strview_t value = strvSubstr(line, pos + 2, SIZE_MAX);

            char *key_str = NULL;
            char *value_str = NULL;

            strvCopy(key, &key_str);
            strvCopy(value, &value_str);

            _setField(&ctx->fields, &ctx->field_count, key_str, value_str);

            free(key_str);
            free(value_str);
        }

        istrSkip(in, 2); // skip \r\n
    } while(line.size > 2);
}

// == HTTP CLIENT =============================================================

http_client_t hcliInit() {
    http_client_t client;
    memset(&client, 0, sizeof(client));
    client.port = 80;
    return client;
}

void hcliFree(http_client_t *ctx) {
    free(ctx->host_name);
    memset(ctx, 0, sizeof(http_client_t));
}

void hcliSetHost(http_client_t *ctx, const char *hostname) {
    strview_t hostview = strvInit(hostname);
    // if the hostname starts with http:// (case insensitive)
    if(strvICompare(strvSubstr(hostview, 0, 7), strvInit("http://")) == 0) {
        strvCopy(strvSubstr(hostview, 7, SIZE_MAX), &ctx->host_name);
    }
    else if(strvICompare(strvSubstr(hostview, 0, 8), strvInit("https://")) == 0) {
        err("HTTPS protocol not yet supported");
        return;
    }
    else {
        // undefined protocol, use HTTP
        strvCopy(hostview, &ctx->host_name);
    }
}

http_response_t hcliSendRequest(http_client_t *ctx, http_request_t *req) {
    if(!reqHasField(req, "Host")) {
        reqSetField(req, "Host", ctx->host_name);
    }
    if(!reqHasField(req, "Content-Length")) {
        if(req->body) {
            str_ostream_t out = ostrInitLen(20);
            ostrAppendu64(&out, strlen(req->body));
            reqSetField(req, "Content-Length", out.buf);
            ostrFree(&out);
        }
        else {
            reqSetField(req, "Content-Length", "0");
        }
    }
    if(req->method == REQ_POST && !reqHasField(req, "Content-Type")) {
        reqSetField(req, "Content-Type", "application/x-www-form-urlencoded");
    }
    if(httpVerNumber(req->version) >= 11 && !reqHasField(req, "Connection")) {
        reqSetField(req, "Connection", "close");
    }

    http_response_t res = resInit();
    char *request_str = NULL;
    str_ostream_t received = ostrInit(1024);

    if(!skInit()) {
        err("couldn't initialize sockets %s", skGetErrorString());
        goto skopen_error;
    }

    ctx->socket = skOpen();
    if(ctx->socket == INVALID_SOCKET) {
        err("couldn't open socket %s", skGetErrorString());
        goto error;
    }

    if(skConnect(ctx->socket, ctx->host_name, ctx->port)) {
        size_t len = reqString(req, &request_str);
        if(len == 0) {
            err("couldn't get string from request");
            goto error;
        }

        if(skSend(ctx->socket, request_str, (int)len) == SOCKET_ERROR) {
            err("couldn't send request to socket: %s", skGetErrorString());
            goto error;
        }

        char buffer[1024];
        int read = 0;
        do {
            read = skReceive(ctx->socket, buffer, sizeof(buffer));
            if(read == -1) {
                err("couldn't get the data from the server: %s", skGetErrorString());
                goto error;
            }
            ostrAppendview(&received, strvInitLen(buffer, read));
        } while(read != 0);

        // if the data received is not null terminated
        if(*(received.buf + received.size) != '\0') {
            ostrPutc(&received, '\0');
            received.size--;
        }
        
        resParse(&res, received.buf);
    }

    if(!skClose(ctx->socket)) {
        err("Couldn't close socket");
    }
    
error:
    if(!skCleanup()) {
        err("couldn't clean up sockets %s", skGetErrorString());
    }
skopen_error:
    free(request_str);
    ostrFree(&received);
    return res;
}

http_response_t httpGet(const char *hostname, const char *uri) {
    http_request_t request = reqInit();
    request.method = REQ_GET;
    reqSetUri(&request, uri);

    http_client_t client = hcliInit();
    hcliSetHost(&client, hostname);

    http_response_t res = hcliSendRequest(&client, &request);

    reqFree(&request);
    hcliFree(&client);

    return res;
}

