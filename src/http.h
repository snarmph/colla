#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "collatypes.h"
#include "str.h"
#include "strstream.h"
#include "socket.h"

typedef enum {
    REQ_GET,   
    REQ_POST,    
    REQ_HEAD,    
    REQ_PUT,    
    REQ_DELETE  
} reqtype_t;

typedef enum {
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
} resstatus_t;

typedef struct {
    uint8 major;
    uint8 minor;
} http_version_t;

// translates a http_version_t to a single readable number (e.g. 1.1 -> 11, 1.0 -> 10, etc)
int httpVerNumber(http_version_t ver);

typedef struct {
    char *key;
    char *value;
} http_field_t;

#include "vec.h"

// == HTTP REQUEST ============================================================

typedef struct {
    reqtype_t method;
    http_version_t version;
    vec(http_field_t) fields;
    char *uri;
    char *body;
} http_request_t;

http_request_t reqInit(void);
void reqFree(http_request_t *ctx);

bool reqHasField(http_request_t *ctx, const char *key);

void reqSetField(http_request_t *ctx, const char *key, const char *value);
void reqSetUri(http_request_t *ctx, strview_t uri);

str_ostream_t reqPrepare(http_request_t *ctx);
str_t reqString(http_request_t *ctx);

// == HTTP RESPONSE ===========================================================

typedef struct {
    resstatus_t status_code;
    vec(http_field_t) fields;
    http_version_t version;
    vec(uint8) body;
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
    uint16 port;
    socket_t socket;
} http_client_t;

http_client_t hcliInit(void);
void hcliFree(http_client_t *ctx);

void hcliSetHost(http_client_t *ctx, strview_t hostname);
http_response_t hcliSendRequest(http_client_t *ctx, http_request_t *request);

// == HTTP ====================================================================

http_response_t httpGet(strview_t hostname, strview_t uri);

// == URL =====================================================================

typedef struct {
    strview_t host;
    strview_t uri;
} url_split_t;

url_split_t urlSplit(strview_t uri);

#ifdef __cplusplus
} // extern "C"
#endif
