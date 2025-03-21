#include "../net.h"
#include "../os.h"

#include <windows.h>

#if !COLLA_TCC
    #include <wininet.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include "../tcc/colla_tcc.h"
#endif

#if COLLA_CMT_LIB
    #pragma comment(lib, "Wininet")
    #pragma comment(lib, "Ws2_32")
#endif

struct {
    HINTERNET internet;
} http_win = {0};

void net_init(void) {
    if (http_win.internet) return;
    http_win.internet = InternetOpen(
        TEXT("COLLA_WININET"),
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
    );

    WSADATA wsdata = {0};
    if (WSAStartup(0x0202, &wsdata)) {
        fatal("couldn't startup sockets: %v", os_get_error_string(WSAGetLastError()));
    }
}

void net_cleanup(void) {
    if (!http_win.internet) return;
    InternetCloseHandle(http_win.internet);
    http_win.internet = NULL;
    WSACleanup();
}

iptr net_get_last_error(void) {
    return WSAGetLastError();
}

http_res_t http_request(http_request_desc_t *req) {
    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    BOOL result = FALSE;
    bool success = false;
    http_res_t res = {0};
    arena_t arena_before = *req->arena;

    if (!http_win.internet) {
        err("net_init has not been called");
        goto failed;
    }

    http_url_t split = http_split_url(req->url);
    strview_t server = split.host;
    strview_t page = split.uri;

    if (strv_starts_with_view(server, strv("http://"))) {
        server = strv_remove_prefix(server, 7);
    }
    
    if (strv_starts_with_view(server, strv("https://"))) {
        server = strv_remove_prefix(server, 8);
    }

    {
        arena_t scratch = *req->arena;

        if (req->version.major == 0) req->version.major = 1;
        if (req->version.minor == 0) req->version.minor = 1;

        const TCHAR *accepted_types[] = { TEXT("*/*"), NULL };
        const char *method = http_get_method_string(req->request_type);
        str_t http_ver = str_fmt(&scratch, "HTTP/%u.%u", req->version.major, req->version.minor);

        tstr_t tserver = strv_to_tstr(&scratch, server);
        tstr_t tpage = strv_to_tstr(&scratch, page);
        tstr_t tmethod = strv_to_tstr(&scratch, strv(method));
        tstr_t thttp_ver = strv_to_tstr(&scratch, strv(http_ver));

        connection = InternetConnect(
            http_win.internet,
            tserver.buf,
            INTERNET_DEFAULT_HTTPS_PORT,
            NULL,
            NULL,
            INTERNET_SERVICE_HTTP,
            0,
            (DWORD_PTR)NULL // userdata
        );
        if (!connection) {
            err("call to InternetConnect failed: %u", os_get_error_string(os_get_last_error()));
            goto failed;
        }

        request = HttpOpenRequest(
            connection,
            tmethod.buf,
            tpage.buf,
            thttp_ver.buf,
            NULL,
            accepted_types,
            INTERNET_FLAG_SECURE,
            (DWORD_PTR)NULL // userdata
        );
        if (!request) {
            err("call to HttpOpenRequest failed: %v", os_get_error_string(os_get_last_error()));
            goto failed;
        }
    }

    for (int i = 0; i < req->header_count; ++i) {
        http_header_t *h = &req->headers[i];
        arena_t scratch = *req->arena;

        str_t header = str_fmt(&scratch, "%v: %v\r\n", h->key, h->value);
        tstr_t theader = strv_to_tstr(&scratch, strv(header));
        HttpAddRequestHeaders(request, theader.buf, (DWORD)theader.len, 0);
    }

    result = HttpSendRequest(
        request,
        NULL,
        0,
        (void *)req->body.buf,
        (DWORD)req->body.len
    );
    if (!result) {
        err("call to HttpSendRequest failed: %v", os_get_error_string(os_get_last_error()));
        goto failed;
    }

    u8 smallbuf[KB(5)];
    DWORD bufsize = sizeof(smallbuf);

    u8 *buffer = smallbuf;

    // try and read it into a static buffer
    result = HttpQueryInfo(request, HTTP_QUERY_RAW_HEADERS_CRLF, smallbuf, &bufsize, NULL);
    
    // buffer is not big enough, allocate one with the arena instead
    if (!result && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        info("buffer is too small");
        buffer = alloc(req->arena, u8, bufsize + 1);
        result = HttpQueryInfo(request, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &bufsize, NULL);
    }

    if (!result) {
        err("couldn't get headers");
        goto failed;
    }

    tstr_t theaders = { (TCHAR *)buffer, bufsize };
    str_t headers = str_from_tstr(req->arena, theaders);

    res.headers = http_parse_headers(req->arena, strv(headers));
    res.version = req->version;

    DWORD status_code = 0;
    DWORD status_code_len = sizeof(status_code);
    result = HttpQueryInfo(request, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &status_code, &status_code_len, 0);
    if (!result) {
        err("couldn't get status code");
        goto failed;
    }

    res.status_code = status_code;

    outstream_t body = ostr_init(req->arena);

    while (true) {
        DWORD read = 0;
        char read_buffer[4096];
        BOOL read_success = InternetReadFile(
            request, read_buffer, sizeof(read_buffer), &read
        );
        if (!read_success || read == 0) {
            break;
        }
        ostr_puts(&body, strv(read_buffer, read));
    }

    res.body = strv(ostr_to_str(&body));

    success = true;

failed:
    if (request) InternetCloseHandle(request);
    if (connection) InternetCloseHandle(connection);
    if (!success) *req->arena = arena_before;
    return res;
}

// SOCKETS //////////////////////////

SOCKADDR_IN sk__addrin_in(const char *ip, u16 port) {
    SOCKADDR_IN sk_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };

    if (!inet_pton(AF_INET, ip, &sk_addr.sin_addr)) {
        err("inet_pton failed: %v", os_get_error_string(net_get_last_error()));
        return (SOCKADDR_IN){0};
    }

    return sk_addr;
}

socket_t sk_open(sktype_e type) {
    int sock_type = 0;

    switch(type) {
        case SOCK_TCP: sock_type = SOCK_STREAM; break;
        case SOCK_UDP: sock_type = SOCK_DGRAM;  break;
        default: fatal("skType not recognized: %d", type); break;
    }

    return socket(AF_INET, sock_type, 0);
}

socket_t sk_open_protocol(const char *protocol) {
    struct protoent *proto = getprotobyname(protocol);
    if(!proto) {
        return INVALID_SOCKET;
    }
    return socket(AF_INET, SOCK_STREAM, proto->p_proto);
}

bool sk_is_valid(socket_t sock) {
    return sock != INVALID_SOCKET;
}

bool sk_close(socket_t sock) {
    return closesocket(sock) != SOCKET_ERROR;
}

bool sk_bind(socket_t sock, const char *ip, u16 port) {
    SOCKADDR_IN sk_addr = sk__addrin_in(ip, port);
    if (sk_addr.sin_family == 0) {
        return false;
    }
    return bind(sock, (SOCKADDR*)&sk_addr, sizeof(sk_addr)) != SOCKET_ERROR;
}

bool sk_listen(socket_t sock, int backlog) {
    return listen(sock, backlog) != SOCKET_ERROR;
}

socket_t sk_accept(socket_t sock) {
    SOCKADDR_IN addr = {0};
    int addr_size = sizeof(addr);
    return accept(sock, (SOCKADDR*)&addr, &addr_size);
}

bool sk_connect(socket_t sock, const char *server, u16 server_port) {
    u8 tmpbuf[1024] = {0};
    arena_t scratch = arena_make(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);

    str16_t wserver = strv_to_str16(&scratch, strv(server));

    ADDRINFOW *addrinfo = NULL;
    int result = GetAddrInfoW(wserver.buf, NULL, NULL, &addrinfo);
    if (result) {
        return false;
    }

    char ip_str[1024] = {0};
    inet_ntop(addrinfo->ai_family, addrinfo->ai_addr, ip_str, sizeof(ip_str));

    SOCKADDR_IN sk_addr = sk__addrin_in(ip_str, server_port);
    if (sk_addr.sin_family == 0) {
        return false;
    }

    return connect(sock, (SOCKADDR*)&sk_addr, sizeof(sk_addr)) != SOCKET_ERROR;
}

int sk_send(socket_t sock, const void *buf, int len) {
    return send(sock, (const char *)buf, len, 0);
}

int sk_recv(socket_t sock, void *buf, int len) {
    return recv(sock, (char *)buf, len, 0);
}

int sk_poll(skpoll_t *to_poll, int num_to_poll, int timeout) {
    return WSAPoll((WSAPOLLFD*)to_poll, (ULONG)num_to_poll, timeout);
}

oshandle_t sk_bind_event(socket_t sock, skevent_e event) {
    if (event == SOCK_EVENT_NONE) {
        return os_handle_zero();
    }

    HANDLE handle = WSACreateEvent();
    if (handle == WSA_INVALID_EVENT) {
        return os_handle_zero();
    }

    int wsa_event = 0;
    if (event & SOCK_EVENT_READ)    wsa_event |= FD_READ;
    if (event & SOCK_EVENT_WRITE)   wsa_event |= FD_WRITE;
    if (event & SOCK_EVENT_ACCEPT)  wsa_event |= FD_ACCEPT;
    if (event & SOCK_EVENT_CONNECT) wsa_event |= FD_CONNECT;
    if (event & SOCK_EVENT_CLOSE)   wsa_event |= FD_CLOSE;

    if (WSAEventSelect(sock, handle, wsa_event) != 0) {
        WSACloseEvent(handle);
        return os_handle_zero();
    }

    return (oshandle_t){ .data = (uptr)handle };
}

void sk_reset_event(oshandle_t handle) {
    if (!os_handle_valid(handle)) {
        warn("invalid handle");
        return;
    }
    WSAResetEvent((HANDLE)handle.data);
}

void sk_destroy_event(oshandle_t handle) {
    if (!os_handle_valid(handle)) return;
    WSACloseEvent((HANDLE)handle.data); 
}

