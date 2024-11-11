#if COLLA_ONLYCORE
    #define COLLA_NOTHREADS 1
    #define COLLA_NOSOCKETS 1
    #define COLLA_NOHTTP    1
    #define COLLA_NOSERVER  1
#endif

#if COLLA_NOSOCKETS
    #undef COLLA_NOHTTP
    #undef COLLA_NOSERVER
    #define COLLA_NOHTTP    1
    #define COLLA_NOSERVER  1
#endif

#include "src/arena.c"
#include "src/base64.c"
#include "src/file.c"
#include "src/format.c"
#include "src/ini.c"
#include "src/json.c"
#include "src/str.c"
#include "src/strstream.c"
#include "src/tracelog.c"
#include "src/utf8.c"
#include "src/vmem.c"
#include "src/xml.c"
#include "src/hot_reload.c"

#if !COLLA_NOTHREADS
#include "src/cthreads.c"
#endif

#if !COLLA_NOSOCKETS
#include "src/socket.c"
#endif

#if !COLLA_NOHTTP
#include "src/http.c"
#endif

#if !COLLA_NOSERVER 
#include "src/server.c"
#endif
