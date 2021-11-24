#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "str.h"
#include "strstream.h"
#include "socket.h"

enum {
    REQ_GET,   
    REQ_POST,    
    REQ_HEAD,    
    REQ_PUT,    
    REQ_DELETE  
};

enum {
    // 2xx: success
    STATUS_OK              = 200,
    STATUS_CREATED         = 201,
    STATUS_ACCEPTED        = 202,
    STATUS_NO_CONTENT      = 204,
    STATUS_RESET_CONTENT   = 205,
    STATUS_PARTIAL_CONTENT = 206,

    // 3xx: redirection
    STATUS_MULTIPLE_CHOICES  = 300,
    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_MOVED_TEMPORARILY = 302,
    STATUS_NOT_MODIFIED      = 304,

    // 4xx: client error
    STATUS_BAD_REQUEST           = 400,
    STATUS_UNAUTHORIZED          = 401,
    STATUS_FORBIDDEN             = 403,
    STATUS_NOT_FOUND             = 404,
    STATUS_RANGE_NOT_SATISFIABLE = 407,

    // 5xx: server error
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED       = 501,
    STATUS_BAD_GATEWAY           = 502,
    STATUS_SERVICE_NOT_AVAILABLE = 503,
    STATUS_GATEWAY_TIMEOUT       = 504,
    STATUS_VERSION_NOT_SUPPORTED = 505,
};

typedef struct {
    uint8_t major;
    uint8_t minor;
} http_version_t;

// translates a http_version_t to a single readable number (e.g. 1.1 -> 11, 1.0 -> 10, etc)
int httpVerNumber(http_version_t ver);

typedef struct {
    char *key;
    char *value;
} http_field_t;

#define T http_field_t
#define VEC_SHORT_NAME field_vec
#define VEC_DISABLE_ERASE_WHEN
#define VEC_NO_IMPLEMENTATION
#include "vec.h"

// == HTTP REQUEST ============================================================

typedef struct {
    int method;
    http_version_t version;
    field_vec_t fields;
    char *uri;
    char *body;
} http_request_t;

http_request_t reqInit(void);
void reqFree(http_request_t *ctx);

bool reqHasField(http_request_t *ctx, const char *key);

void reqSetField(http_request_t *ctx, const char *key, const char *value);
void reqSetUri(http_request_t *ctx, const char *uri);

str_ostream_t reqPrepare(http_request_t *ctx);
str_t reqString(http_request_t *ctx);

// == HTTP RESPONSE ===========================================================

typedef struct {
    int status_code;
    field_vec_t fields;
    http_version_t version;
    str_t body;
} http_response_t;

http_response_t resInit(void);
void resFree(http_response_t *ctx);

bool resHasField(http_response_t *ctx, const char *key);
const char *resGetField(http_response_t *ctx, const char *field);

void resParse(http_response_t *ctx, const char *data);
void resParseFields(http_response_t *ctx, str_istream_t *in);

// == HTTP CLIENT =============================================================

typedef struct {
    str_t host_name;
    uint16_t port;
    socket_t socket;
} http_client_t;

http_client_t hcliInit(void);
void hcliFree(http_client_t *ctx);

void hcliSetHost(http_client_t *ctx, const char *hostname);
http_response_t hcliSendRequest(http_client_t *ctx, http_request_t *request);

// == HTTP ====================================================================

http_response_t httpGet(const char *hostname, const char *uri);

#ifdef __cplusplus
} // extern "C"
#endif