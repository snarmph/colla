#include "http.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "os.h"
#include "tracelog.h"

#define T http_field_t
#define VEC_SHORT_NAME field_vec
#define VEC_DISABLE_ERASE_WHEN
#define VEC_NO_DECLARATION
#include "vec.h"

// == INTERNAL ================================================================

static void _setField(field_vec_t *fields, const char *key, const char *value) {
    // search if the field already exists
    for(size_t i = 0; i < fields->size; ++i) {
        if(stricmp(fields->buf[i].key, key) == 0) {
            // replace value
            char **curval = &fields->buf[i].value;
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
    http_field_t field;
    size_t klen = strlen(key);
    size_t vlen = strlen(value);
    field.key = malloc(klen + 1);
    field.value = malloc(vlen + 1);
    memcpy(field.key, key, klen);
    memcpy(field.value, value, vlen);
    field.key[klen] = field.value[vlen] = '\0';

    field_vecPush(fields, field);
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
    for(size_t i = 0; i < ctx->fields.size; ++i) {
        free(ctx->fields.buf[i].key);
        free(ctx->fields.buf[i].value);
    }
    field_vecFree(&ctx->fields);
    free(ctx->uri);
    free(ctx->body);
    memset(ctx, 0, sizeof(http_request_t));
}

bool reqHasField(http_request_t *ctx, const char *key) {
    for(size_t i = 0; i < ctx->fields.size; ++i) {
        if(stricmp(ctx->fields.buf[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

void reqSetField(http_request_t *ctx, const char *key, const char *value) {
    _setField(&ctx->fields, key, value);
}

void reqSetUri(http_request_t *ctx, const char *uri) {
    if (uri == NULL) return;
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

    for(size_t i = 0; i < ctx->fields.size; ++i) {
        ostrPrintf(&out, "%s: %s\r\n", ctx->fields.buf[i].key, ctx->fields.buf[i].value);
    }

    ostrAppendview(&out, strvInit("\r\n"));
    if(ctx->body) {
        ostrAppendview(&out, strvInit(ctx->body));
    }

error:
    return out;
}

str_t reqString(http_request_t *ctx) {
    str_ostream_t out = reqPrepare(ctx);
    return ostrMove(&out);
}

// == HTTP RESPONSE ===========================================================

http_response_t resInit() {
    return (http_response_t) {0};
}   

void resFree(http_response_t *ctx) {
    for(size_t i = 0; i < ctx->fields.size; ++i) {
        free(ctx->fields.buf[i].key);
        free(ctx->fields.buf[i].value);
    }
    field_vecFree(&ctx->fields);
    strFree(&ctx->body);
    memset(ctx, 0, sizeof(http_response_t));
}

bool resHasField(http_response_t *ctx, const char *key) {
    for(size_t i = 0; i < ctx->fields.size; ++i) {
        if(stricmp(ctx->fields.buf[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

const char *resGetField(http_response_t *ctx, const char *field) {
    for(size_t i = 0; i < ctx->fields.size; ++i) {
        if(stricmp(ctx->fields.buf[i].key, field) == 0) {
            return ctx->fields.buf[i].value;
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
        strFree(&ctx->body);
        ctx->body = strvCopy(body);
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

            key_str = strvCopy(key).buf;
            value_str = strvCopy(value).buf;

            _setField(&ctx->fields, key_str, value_str);

            free(key_str);
            free(value_str);
        }

        istrSkip(in, 2); // skip \r\n
    } while(line.len > 2);
}

// == HTTP CLIENT =============================================================

http_client_t hcliInit() {
    return (http_client_t) {
        .port = 80,
    };
}

void hcliFree(http_client_t *ctx) {
    strFree(&ctx->host_name);
    memset(ctx, 0, sizeof(http_client_t));
}

void hcliSetHost(http_client_t *ctx, const char *hostname) {
    strview_t hostview = strvInit(hostname);
    // if the hostname starts with http:// (case insensitive)
    if(strvICompare(strvSubstr(hostview, 0, 7), strvInit("http://")) == 0) {
        ctx->host_name = strvCopy(strvSubstr(hostview, 7, SIZE_MAX));
    }
    else if(strvICompare(strvSubstr(hostview, 0, 8), strvInit("https://")) == 0) {
        err("HTTPS protocol not yet supported");
        return;
    }
    else {
        // undefined protocol, use HTTP
        ctx->host_name = strvCopy(hostview);
    }
}

http_response_t hcliSendRequest(http_client_t *ctx, http_request_t *req) {
    if (strBack(&ctx->host_name) == '/') {
        strPop(&ctx->host_name);
    }
    if(!reqHasField(req, "Host")) {
        reqSetField(req, "Host", ctx->host_name.buf);
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
    str_t req_str = strInit();
    str_ostream_t received = ostrInitLen(1024);

    if(!skInit()) {
        err("couldn't initialize sockets %s", skGetErrorString());
        goto skopen_error;
    }

    ctx->socket = skOpen(SOCK_TCP);
    if(ctx->socket == INVALID_SOCKET) {
        err("couldn't open socket %s", skGetErrorString());
        goto error;
    }

    if(skConnect(ctx->socket, ctx->host_name.buf, ctx->port)) {
        req_str = reqString(req);
        if(req_str.len == 0) {
            err("couldn't get string from request");
            goto error;
        }

        if(skSend(ctx->socket, req_str.buf, (int)req_str.len) == SOCKET_ERROR) {
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
    else {
        err("Couldn't connect to host %s -> %s", ctx->host_name, skGetErrorString());
    }

    if(!skClose(ctx->socket)) {
        err("Couldn't close socket");
    }
    
error:
    if(!skCleanup()) {
        err("couldn't clean up sockets %s", skGetErrorString());
    }
skopen_error:
    strFree(&req_str);
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

