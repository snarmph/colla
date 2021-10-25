#include "socket.h"

#include <stdio.h>
#include "tracelog.h"

#ifndef NDEBUG
// VERY MUCH NOT THREAD SAFE
static int initialize_count = 0;
#endif

#if SOCK_WINDOWS
#include <ws2tcpip.h>

static bool _win_skInit();
static bool _win_skCleanup();
static int _win_skGetError();
static const char *_win_skGetErrorString();

#define SOCK_CALL(fun) _win_##fun

#elif SOCK_POSIX
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> // strerror

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

static bool _posix_skInit();
static bool _posix_skCleanup();
static int _posix_skGetError();
static const char *_posix_skGetErrorString();

#define SOCK_CALL(fun) _posix_##fun

#endif

bool skInit() {
#ifndef NDEBUG
    ++initialize_count;
#endif
    return SOCK_CALL(skInit());
}

bool skCleanup() {
#ifndef NDEBUG
    --initialize_count;
#endif
    return SOCK_CALL(skCleanup());
}

socket_t skOpen(skType type) {
    int sock_type = 0;

    switch(type) {
    case SOCK_TCP: sock_type = SOCK_STREAM; break;
    case SOCK_UDP: sock_type = SOCK_DGRAM;  break;
    default: fatal("skType not recognized: %d", type); break;
    }

    return skOpenPro(AF_INET, sock_type, 0);
}

socket_t skOpenEx(const char *protocol) {
#ifndef NDEBUG
    if(initialize_count <= 0) {
        fatal("skInit has not been called");
    }
#endif
    struct protoent *proto = getprotobyname(protocol);
    if(!proto) {
        return INVALID_SOCKET;
    }
    return skOpenPro(AF_INET, SOCK_STREAM, proto->p_proto);
}

socket_t skOpenPro(int af, int type, int protocol) {
#ifndef NDEBUG
    if(initialize_count <= 0) {
        fatal("skInit has not been called");
    }
#endif
    return socket(af, type, protocol);
}

sk_addrin_t skAddrinInit(const char *ip, uint16_t port) {
    sk_addrin_t addr;
    addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
    return addr;
}

bool skClose(socket_t sock) {
#if SOCK_WINDOWS
    int error = closesocket(sock);
#elif SOCK_POSIX
    int error = close(sock);
#endif
    sock = INVALID_SOCKET;
    return error != SOCKET_ERROR;
}

bool skBind(socket_t sock, const char *ip, uint16_t port) {
    sk_addrin_t addr;
    addr.sin_family = AF_INET;
    // TODO use inet_pton instead
    addr.sin_addr.s_addr = inet_addr(ip);
    
    addr.sin_port = htons(port);

    return skBindPro(sock, (sk_addr_t *) &addr, sizeof(addr));
}

bool skBindPro(socket_t sock, const sk_addr_t *name, socket_len_t namelen) {
    return bind(sock, name, namelen) != SOCKET_ERROR;
}

bool skListen(socket_t sock) {
    return skListenPro(sock, 1);
}

bool skListenPro(socket_t sock, int backlog) {
    return listen(sock, backlog) != SOCKET_ERROR;
}

socket_t skAccept(socket_t sock) {
    sk_addrin_t addr;
    socket_len_t addr_size = (socket_len_t)sizeof(addr);
    return skAcceptPro(sock, (sk_addr_t *) &addr, &addr_size);
}

socket_t skAcceptPro(socket_t sock, sk_addr_t *addr, socket_len_t *addrlen) {
    return accept(sock, addr, addrlen);
}

bool skConnect(socket_t sock, const char *server, unsigned short server_port) {
    // TODO use getaddrinfo insetad
    struct hostent *host = gethostbyname(server);
    // if gethostbyname fails, inet_addr will also fail and return an easier to debug error
    const char *address = server;
    if(host) {
        address = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    }
    
    sk_addrin_t addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address);
    addr.sin_port = htons(server_port);

    return skConnectPro(sock, (sk_addr_t *) &addr, sizeof(addr));
}

bool skConnectPro(socket_t sock, const sk_addr_t *name, socket_len_t namelen) {
    return connect(sock, name, namelen) != SOCKET_ERROR;
}

int skSend(socket_t sock, const char *buf, int len) {
    return skSendPro(sock, buf, len, 0);
}

int skSendPro(socket_t sock, const char *buf, int len, int flags) {
    return send(sock, buf, len, flags);
}

int skSendTo(socket_t sock, const char *buf, int len, const sk_addrin_t *to) {
    return skSendToPro(sock, buf, len, 0, (sk_addr_t*) to, sizeof(sk_addrin_t));
}

int skSendToPro(socket_t sock, const char *buf, int len, int flags, const sk_addr_t *to, int tolen) {
    return sendto(sock, buf, len, flags, to, tolen);
}

int skReceive(socket_t sock, char *buf, int len) {
    return skReceivePro(sock, buf, len, 0);
}

int skReceivePro(socket_t sock, char *buf, int len, int flags) {
    return recv(sock, buf, len, flags);
}

int skReceiveFrom(socket_t sock, char *buf, int len, sk_addrin_t *from) {
    socket_len_t fromlen = sizeof(sk_addr_t);
    return skReceiveFromPro(sock, buf, len, 0, (sk_addr_t*)from, &fromlen);
}

int skReceiveFromPro(socket_t sock, char *buf, int len, int flags, sk_addr_t *from, socket_len_t *fromlen) {
    return recvfrom(sock, buf, len, flags, from, fromlen);
}

bool skIsValid(socket_t sock) {
    return sock != INVALID_SOCKET;
}

int skGetError() {
    return SOCK_CALL(skGetError());
}

const char *skGetErrorString() {
    return SOCK_CALL(skGetErrorString());
}

#ifdef SOCK_WINDOWS
static bool _win_skInit() {
    WSADATA w;
    int error = WSAStartup(0x0202, &w);
    return error == 0;
}

static bool _win_skCleanup() {
    return WSACleanup() == 0;
}

static int _win_skGetError() {
    return WSAGetLastError();
}

static const char *_win_skGetErrorString() {
    switch(_win_skGetError()) {
        case WSA_INVALID_HANDLE: return "Specified event object handle is invalid.";
        case WSA_NOT_ENOUGH_MEMORY: return "Insufficient memory available.";
        case WSA_INVALID_PARAMETER: return "One or more parameters are invalid.";
        case WSA_OPERATION_ABORTED: return "Overlapped operation aborted.";
        case WSA_IO_INCOMPLETE: return "Overlapped I/O event object not in signaled state.";
        case WSA_IO_PENDING: return "Overlapped operations will complete later.";
        case WSAEINTR: return "Interrupted function call.";
        case WSAEBADF: return "File handle is not valid.";
        case WSAEACCES: return "Permission denied.";
        case WSAEFAULT: return "Bad address.";
        case WSAEINVAL: return "Invalid argument.";
        case WSAEMFILE: return "Too many open files.";
        case WSAEWOULDBLOCK: return "Resource temporarily unavailable.";
        case WSAEINPROGRESS: return "Operation now in progress.";
        case WSAEALREADY: return "Operation already in progress.";
        case WSAENOTSOCK: return "Socket operation on nonsocket.";
        case WSAEDESTADDRREQ: return "Destination address required.";
        case WSAEMSGSIZE: return "Message too long.";
        case WSAEPROTOTYPE: return "Protocol wrong type for socket.";
        case WSAENOPROTOOPT: return "Bad protocol option.";
        case WSAEPROTONOSUPPORT: return "Protocol not supported.";
        case WSAESOCKTNOSUPPORT: return "Socket type not supported.";
        case WSAEOPNOTSUPP: return "Operation not supported.";
        case WSAEPFNOSUPPORT: return "Protocol family not supported.";
        case WSAEAFNOSUPPORT: return "Address family not supported by protocol family.";
        case WSAEADDRINUSE: return "Address already in use.";
        case WSAEADDRNOTAVAIL: return "Cannot assign requested address.";
        case WSAENETDOWN: return "Network is down.";
        case WSAENETUNREACH: return "Network is unreachable.";
        case WSAENETRESET: return "Network dropped connection on reset.";
        case WSAECONNABORTED: return "Software caused connection abort.";
        case WSAECONNRESET: return "Connection reset by peer.";
        case WSAENOBUFS: return "No buffer space available.";
        case WSAEISCONN: return "Socket is already connected.";
        case WSAENOTCONN: return "Socket is not connected.";
        case WSAESHUTDOWN: return "Cannot send after socket shutdown.";
        case WSAETOOMANYREFS: return "Too many references.";
        case WSAETIMEDOUT: return "Connection timed out.";
        case WSAECONNREFUSED: return "Connection refused.";
        case WSAELOOP: return "Cannot translate name.";
        case WSAENAMETOOLONG: return "Name too long.";
        case WSAEHOSTDOWN: return "Host is down.";
        case WSAEHOSTUNREACH: return "No route to host.";
        case WSAENOTEMPTY: return "Directory not empty.";
        case WSAEPROCLIM: return "Too many processes.";
        case WSAEUSERS: return "User quota exceeded.";
        case WSAEDQUOT: return "Disk quota exceeded.";
        case WSAESTALE: return "Stale file handle reference.";
        case WSAEREMOTE: return "Item is remote.";
        case WSASYSNOTREADY: return "Network subsystem is unavailable.";
        case WSAVERNOTSUPPORTED: return "Winsock.dll version out of range.";
        case WSANOTINITIALISED: return "Successful WSAStartup not yet performed.";
        case WSAEDISCON: return "Graceful shutdown in progress.";
        case WSAENOMORE: return "No more results.";
        case WSAECANCELLED: return "Call has been canceled.";
        case WSAEINVALIDPROCTABLE: return "Procedure call table is invalid.";
        case WSAEINVALIDPROVIDER: return "Service provider is invalid.";
        case WSAEPROVIDERFAILEDINIT: return "Service provider failed to initialize.";
        case WSASYSCALLFAILURE: return "System call failure.";
        case WSASERVICE_NOT_FOUND: return "Service not found.";
        case WSATYPE_NOT_FOUND: return "Class type not found.";
        case WSA_E_NO_MORE: return "No more results.";
        case WSA_E_CANCELLED: return "Call was canceled.";
        case WSAEREFUSED: return "Database query was refused.";
        case WSAHOST_NOT_FOUND: return "Host not found.";
        case WSATRY_AGAIN: return "Nonauthoritative host not found.";
        case WSANO_RECOVERY: return "This is a nonrecoverable error.";
        case WSANO_DATA: return "Valid name, no data record of requested type.";
        case WSA_QOS_RECEIVERS: return "QoS receivers.";
        case WSA_QOS_SENDERS: return "QoS senders.";
        case WSA_QOS_NO_SENDERS: return "No QoS senders.";
        case WSA_QOS_NO_RECEIVERS: return "QoS no receivers.";
        case WSA_QOS_REQUEST_CONFIRMED: return "QoS request confirmed.";
        case WSA_QOS_ADMISSION_FAILURE: return "QoS admission error.";
        case WSA_QOS_POLICY_FAILURE: return "QoS policy failure.";
        case WSA_QOS_BAD_STYLE: return "QoS bad style.";
        case WSA_QOS_BAD_OBJECT: return "QoS bad object.";
        case WSA_QOS_TRAFFIC_CTRL_ERROR: return "QoS traffic control error.";
        case WSA_QOS_GENERIC_ERROR: return "QoS generic error.";
        case WSA_QOS_ESERVICETYPE: return "QoS service type error.";
        case WSA_QOS_EFLOWSPEC: return "QoS flowspec error.";
        case WSA_QOS_EPROVSPECBUF: return "Invalid QoS provider buffer.";
        case WSA_QOS_EFILTERSTYLE: return "Invalid QoS filter style.";
        case WSA_QOS_EFILTERTYPE: return "Invalid QoS filter type.";
        case WSA_QOS_EFILTERCOUNT: return "Incorrect QoS filter count.";
        case WSA_QOS_EOBJLENGTH: return "Invalid QoS object length.";
        case WSA_QOS_EFLOWCOUNT: return "Incorrect QoS flow count.";
        case WSA_QOS_EUNKOWNPSOBJ: return "Unrecognized QoS object.";
        case WSA_QOS_EPOLICYOBJ: return "Invalid QoS policy object.";
        case WSA_QOS_EFLOWDESC: return "Invalid QoS flow descriptor.";
        case WSA_QOS_EPSFLOWSPEC: return "Invalid QoS provider-specific flowspec.";
        case WSA_QOS_EPSFILTERSPEC: return "Invalid QoS provider-specific filterspec.";
        case WSA_QOS_ESDMODEOBJ: return "Invalid QoS shape discard mode object.";
        case WSA_QOS_ESHAPERATEOBJ: return "Invalid QoS shaping rate object.";
        case WSA_QOS_RESERVED_PETYPE: return "Reserved policy QoS element type.";
    }

    return "(nothing)";
}

#else

static bool _posix_skInit() {
    return true;
}

static bool _posix_skCleanup() {
    return true;
}

static int _posix_skGetError() {
    return errno;
}

static const char *_posix_skGetErrorString() {
    return strerror(errno);
}
#endif