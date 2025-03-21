#ifndef COLLA_NET_HEADER
#define COLLA_NET_HEADER

#include "core.h"
#include "str.h"
#include "os.h"

void net_init(void);
void net_cleanup(void);
iptr net_get_last_error(void);

typedef enum http_method_e {
    HTTP_GET,   
    HTTP_POST,    
    HTTP_HEAD,    
    HTTP_PUT,    
    HTTP_DELETE 
} http_method_e;

const char *http_get_method_string(http_method_e method);
const char *http_get_status_string(int status);

typedef struct http_version_t http_version_t;
struct http_version_t {
    u8 major;
    u8 minor;
};

typedef struct http_header_t http_header_t;
struct http_header_t {
    strview_t key;
    strview_t value;
    http_header_t *next;
};

typedef struct http_req_t http_req_t;
struct http_req_t {
    http_method_e method;
    http_version_t version;
    http_header_t *headers;
    strview_t url;
    strview_t body;
};

typedef struct http_res_t http_res_t;
struct http_res_t {
    int status_code;
    http_version_t version;
    http_header_t *headers;
    strview_t body;
};

http_header_t *http_parse_headers(arena_t *arena, strview_t header_string);

http_req_t http_parse_req(arena_t *arena, strview_t request);
http_res_t http_parse_res(arena_t *arena, strview_t response);

str_t http_req_to_str(arena_t *arena, http_req_t *req);
str_t http_res_to_str(arena_t *arena, http_res_t *res);

bool http_has_header(http_header_t *headers, strview_t key);
void http_set_header(http_header_t *headers, strview_t key, strview_t value);
strview_t http_get_header(http_header_t *headers, strview_t key);

str_t http_make_url_safe(arena_t *arena, strview_t string);
str_t http_decode_url_safe(arena_t *arena, strview_t string);

typedef struct {
    strview_t host;
    strview_t uri;
} http_url_t;

http_url_t http_split_url(strview_t url);

typedef struct {
    arena_t *arena;
    strview_t url;
    http_version_t version; // 1.1 by default
    http_method_e request_type;
    http_header_t *headers; 
    int header_count;
    strview_t body;
} http_request_desc_t;

// arena_t *arena, strview_t url, [ http_header_t *headers, int header_count, strview_t body ]
#define http_get(arena, url, ...) http_request(&(http_request_desc_t){ arena, url, .request_type = HTTP_GET, .version = { 1, 1 }, __VA_ARGS__ })

http_res_t http_request(http_request_desc_t *request);

// SOCKETS //////////////////////////

typedef uintptr_t socket_t;
typedef struct skpoll_t skpoll_t;

#define SK_ADDR_LOOPBACK "127.0.0.1"
#define SK_ADDR_ANY "0.0.0.0"
#define SK_ADDR_BROADCAST "255.255.255.255"

struct skpoll_t {
    uintptr_t  socket;
    short events;
    short revents;
};

#define SOCKET_ERROR (-1)

typedef enum {
    SOCK_TCP,
    SOCK_UDP,
} sktype_e;

typedef enum {
    SOCK_EVENT_NONE,
    SOCK_EVENT_READ    = 1 << 0,
    SOCK_EVENT_WRITE   = 1 << 1,
    SOCK_EVENT_ACCEPT  = 1 << 2,
    SOCK_EVENT_CONNECT = 1 << 3,
    SOCK_EVENT_CLOSE   = 1 << 4,
} skevent_e;

// Opens a socket
socket_t sk_open(sktype_e type);
// Opens a socket using 'protocol', options are 
// ip, icmp, ggp, tcp, egp, pup, udp, hmp, xns-idp, rdp
socket_t sk_open_protocol(const char *protocol);

// Checks that a opened socket is valid, returns true on success
bool sk_is_valid(socket_t sock);

// Closes a socket, returns true on success
bool sk_close(socket_t sock);

// Fill out a sk_addrin_t structure with "ip" and "port"
// skaddrin_t sk_addrin_init(const char *ip, uint16_t port);

// Associate a local address with a socket
bool sk_bind(socket_t sock, const char *ip, u16 port);

// Place a socket in a state in which it is listening for an incoming connection
bool sk_listen(socket_t sock, int backlog);

// Permits an incoming connection attempt on a socket
socket_t sk_accept(socket_t sock);

// Connects to a server (e.g. "127.0.0.1" or "google.com") with a port(e.g. 1234), returns true on success
bool sk_connect(socket_t sock, const char *server, u16 server_port);

// Sends data on a socket, returns true on success
int sk_send(socket_t sock, const void *buf, int len);
// Receives data from a socket, returns byte count on success, 0 on connection close or -1 on error
int sk_recv(socket_t sock, void *buf, int len);

// Wait for an event on some sockets
int sk_poll(skpoll_t *to_poll, int num_to_poll, int timeout);

oshandle_t sk_bind_event(socket_t sock, skevent_e event);
void sk_destroy_event(oshandle_t handle);
void sk_reset_event(oshandle_t handle);

// WEBSOCKETS ///////////////////////

bool websocket_init(arena_t scratch, socket_t socket, strview_t key);
buffer_t websocket_encode(arena_t *arena, strview_t message);
str_t websocket_decode(arena_t *arena, buffer_t message);

// SHA 1 ////////////////////////////

typedef struct sha1_t sha1_t;
struct sha1_t {
    u32 digest[5];
    u8 block[64];
    usize block_index;
    usize byte_count;
};

typedef struct sha1_result_t sha1_result_t;
struct sha1_result_t {
    u32 digest[5];
};

sha1_t sha1_init(void);
sha1_result_t sha1(sha1_t *ctx, const void *buf, usize len);
str_t sha1_str(arena_t *arena, sha1_t *ctx, const void *buf, usize len);

// BASE 64 //////////////////////////

buffer_t base64_encode(arena_t *arena, buffer_t buffer);
buffer_t base64_decode(arena_t *arena, buffer_t buffer);

#endif