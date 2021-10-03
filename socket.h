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

struct sockaddr;

#if SOCK_WINDOWS
    #include <winsock2.h>
    typedef SOCKET socket_t;
    typedef int socket_len_t;
    #define INVALID_SOCKET  (socket_t)(~0)
    #define SOCKET_ERROR              (-1)
#elif SOCK_POSIX
    #include <sys/socket.h> 
    typedef int socket_t;
    typedef uint32_t socket_len_t;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
#endif

// Initialize sockets, returns true on success
bool skInit();
// Terminates sockets, returns true on success
bool skCleanup();

// Opens a socket, check socket_t with skValid
socket_t skOpen();
// Opens a socket using 'protocol', options are 
// ip, icmp, ggp, tcp, egp, pup, udp, hmp, xns-idp, rdp
// check socket_t with skValid
socket_t skOpenEx(const char *protocol);
// Opens a socket, check socket_t with skValid
socket_t skOpenPro(int af, int type, int protocol);

// Closes a socket, returns true on success
bool skClose(socket_t sock);

// Associate a local address with a socket
bool skBind(socket_t sock, const char *ip, uint16_t port);
// Associate a local address with a socket
bool skBindPro(socket_t sock, const struct sockaddr *name, socket_len_t namelen);

// Place a socket in a state in which it is listening for an incoming connection
bool skListen(socket_t sock);
// Place a socket in a state in which it is listening for an incoming connection
bool skListenPro(socket_t sock, int backlog);

// Permits an incoming connection attempt on a socket
socket_t skAccept(socket_t sock);
// Permits an incoming connection attempt on a socket
socket_t skAcceptPro(socket_t sock, struct sockaddr *addr, socket_len_t *addrlen);

// Connects to a server (e.g. "127.0.0.1" or "google.com") with a port(e.g. 1234), returns true on success
bool skConnect(socket_t sock, const char *server, unsigned short server_port);
// Connects to a server, returns true on success
bool skConnectPro(socket_t sock, const struct sockaddr *name, socket_len_t namelen);

// Sends data on a socket, returns true on success
int skSend(socket_t sock, char *buf, int len);
// Sends data on a socket, returns true on success
int skSendPro(socket_t sock, char *buf, int len, int flags);
// Receives data from a socket, returns byte count on success, 0 on connection close or -1 on error
int skReceive(socket_t sock, char *buf, int len);
// Sends data on a socket, returns true on success
int skReceivePro(socket_t sock, char *buf, int len, int flags);

// Checks that a opened socket is valid, returns true on success
bool skIsValid(socket_t sock);

// Returns latest socket error, returns 0 if there is no error
int skGetError();
// Returns a human-readable string from a skGetError
const char *skGetErrorString();

#ifdef __cplusplus
} // extern "C"
#endif