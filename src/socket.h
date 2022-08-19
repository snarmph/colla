#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
    #define SOCK_WINDOWS 1
#else
    #define SOCK_POSIX 1
#endif

#if SOCK_WINDOWS
    #pragma warning(disable:4996) // _WINSOCK_DEPRECATED_NO_WARNINGS
    #include "win32_slim.h"
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
    typedef int sk_len_t;
#elif SOCK_POSIX
    #include <sys/socket.h> 
    #include <netinet/in.h> 
    #include <arpa/inet.h>
    typedef int socket_t;
    typedef uint32_t sk_len_t;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
#endif

typedef struct sockaddr sk_addr_t;
typedef struct sockaddr_in sk_addrin_t;

typedef enum {
    SOCK_TCP,
    SOCK_UDP,
} sktype_t;

// == RAW SOCKETS ==========================================

// Initialize sockets, returns true on success
bool skInit(void);
// Terminates sockets, returns true on success
bool skCleanup(void);

// Opens a socket, check socket_t with skValid
socket_t skOpen(sktype_t type);
// Opens a socket using 'protocol', options are 
// ip, icmp, ggp, tcp, egp, pup, udp, hmp, xns-idp, rdp
// check socket_t with skValid
socket_t skOpenEx(const char *protocol);
// Opens a socket, check socket_t with skValid
socket_t skOpenPro(int af, int type, int protocol);

// Fill out a sk_addrin_t structure with "ip" and "port"
sk_addrin_t skAddrinInit(const char *ip, uint16_t port);

// Closes a socket, returns true on success
bool skClose(socket_t sock);

// Associate a local address with a socket
bool skBind(socket_t sock, const char *ip, uint16_t port);
// Associate a local address with a socket
bool skBindPro(socket_t sock, const sk_addr_t *name, sk_len_t namelen);

// Place a socket in a state in which it is listening for an incoming connection
bool skListen(socket_t sock);
// Place a socket in a state in which it is listening for an incoming connection
bool skListenPro(socket_t sock, int backlog);

// Permits an incoming connection attempt on a socket
socket_t skAccept(socket_t sock);
// Permits an incoming connection attempt on a socket
socket_t skAcceptPro(socket_t sock, sk_addr_t *addr, sk_len_t *addrlen);

// Connects to a server (e.g. "127.0.0.1" or "google.com") with a port(e.g. 1234), returns true on success
bool skConnect(socket_t sock, const char *server, unsigned short server_port);
// Connects to a server, returns true on success
bool skConnectPro(socket_t sock, const sk_addr_t *name, sk_len_t namelen);

// Sends data on a socket, returns true on success
int skSend(socket_t sock, const void *buf, int len);
// Sends data on a socket, returns true on success
int skSendPro(socket_t sock, const void *buf, int len, int flags);
// Sends data to a specific destination
int skSendTo(socket_t sock, const void *buf, int len, const sk_addrin_t *to);
// Sends data to a specific destination
int skSendToPro(socket_t sock, const void *buf, int len, int flags, const sk_addr_t *to, int tolen);
// Receives data from a socket, returns byte count on success, 0 on connection close or -1 on error
int skReceive(socket_t sock, void *buf, int len);
// Receives data from a socket, returns byte count on success, 0 on connection close or -1 on error
int skReceivePro(socket_t sock, void *buf, int len, int flags);
// Receives a datagram and stores the source address. 
int skReceiveFrom(socket_t sock, void *buf, int len, sk_addrin_t *from);
// Receives a datagram and stores the source address. 
int skReceiveFromPro(socket_t sock, void *buf, int len, int flags, sk_addr_t *from, sk_len_t *fromlen);

// Checks that a opened socket is valid, returns true on success
bool skIsValid(socket_t sock);

// Returns latest socket error, returns 0 if there is no error
int skGetError(void);
// Returns a human-readable string from a skGetError
const char *skGetErrorString(void);

// == UDP SOCKETS ==========================================

typedef socket_t udpsock_t;




#ifdef __cplusplus
} // extern "C"
#endif
