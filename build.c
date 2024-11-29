#if COLLA_ONLYCORE
    #define COLLA_NOTHREADS   1
    #define COLLA_NOSOCKETS   1
    #define COLLA_NOHTTP      1
    #define COLLA_NOSERVER    1
    #define COLLA_NOHOTRELOAD 1
#endif

#if COLLA_NOSOCKETS
    #undef COLLA_NOHTTP
    #undef COLLA_NOSERVER
    #define COLLA_NOHTTP    1
    #define COLLA_NOSERVER  1
#endif

#include "arena.c"
#include "base64.c"
#include "file.c"
#include "format.c"
#include "ini.c"
#include "json.c"
#include "str.c"
#include "strstream.c"
#include "tracelog.c"
#include "utf8.c"
#include "vmem.c"
#include "xml.c"
#include "sha1.c"
#include "markdown.c"
#include "highlight.c"
#include "dir.c"

#if !COLLA_NOTHREADS
#include "cthreads.c"
#endif

#if !COLLA_NOSOCKETS
#include "socket.c"
#include "websocket.c"
#endif

#if !COLLA_NOHTTP
#include "http.c"
#endif

#if !COLLA_NOSERVER 
#include "server.c"
#endif

#if !COLLA_NOHOTRELOAD
#include "hot_reload.c"
#endif