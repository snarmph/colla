/*
    colla.h -- All colla libraries in a single header
    Do the following in *one* C file to create the implementation
      #define COLLA_IMPL
    Use the following in the same C file for options
      #define COLLA_NO_THREADS // don't include the threads module
      #define COLLA_NO_NET     // don't include networking stuff
*/
#pragma once

#include <stdint.h>
#include <stddef.h>

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef size_t    usize;
typedef ptrdiff_t isize;

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Define any of this to turn on the option
 * -> TLOG_NO_COLOURS:         print without using colours
 * -> TLOG_VS:                 print to visual studio console, also turns on TLOG_NO_COLOURS
 * -> TLOG_DONT_EXIT_ON_FATAL: don't call 'exit(1)' when using LogFatal
*/

#include <stdbool.h>
#include <stdarg.h>

enum {
    LogAll, LogTrace, LogDebug, LogInfo, LogWarning, LogError, LogFatal
};

void traceLog(int level, const char *fmt, ...);
void traceLogVaList(int level, const char *fmt, va_list args);
void traceUseNewline(bool use_newline);

#define tall(...)  traceLog(LogAll, __VA_ARGS__)
#define trace(...) traceLog(LogTrace, __VA_ARGS__)
#define debug(...) traceLog(LogDebug, __VA_ARGS__)
#define info(...)  traceLog(LogInfo, __VA_ARGS__)
#define warn(...)  traceLog(LogWarning, __VA_ARGS__)
#define err(...)   traceLog(LogError, __VA_ARGS__)
#define fatal(...) traceLog(LogFatal, __VA_ARGS__)

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <wchar.h>

/* #include "collatypes.h" */

#define STRV_NOT_FOUND SIZE_MAX

typedef struct str_t {
    char *buf;
    usize len;
} str_t;

typedef struct {
    const char *buf;
    usize len;
} strview_t;

// == STR_T ========================================================

str_t strInit(void);
str_t strFromStr(const char *cstr);
str_t strFromView(strview_t view);
str_t strFromBuf(const char *buf, usize len);
str_t strFromFmt(const char *fmt, ...);

str_t strFromWCHAR(const wchar_t *src, usize len);
wchar_t *strToWCHAR(str_t ctx);

void strFree(str_t ctx);
str_t strDup(str_t ctx);
str_t strMove(str_t *ctx);

strview_t strGetView(str_t ctx);

char *strBegin(str_t ctx);
char *strEnd(str_t ctx);
char strBack(str_t ctx);

bool strIsEmpty(str_t ctx);
void strReplace(str_t *ctx, char from, char to);

// if len == SIZE_MAX, copies until end
str_t strSubstr(str_t ctx, usize from, usize to);
// if len == SIZE_MAX, returns until end
strview_t strSubview(str_t ctx, usize from, usize to);

void strLower(str_t *ctx);
str_t strToLower(str_t ctx);

void strUpper(str_t *ctx);
str_t strToUpper(str_t ctx);

#ifdef STR_TESTING
void strTest(void);
#endif

// == STRVIEW_T ====================================================

strview_t strvInit(const char *cstr);
strview_t strvInitStr(str_t str);
strview_t strvInitLen(const char *buf, usize size);

char strvFront(strview_t ctx);
char strvBack(strview_t ctx);
const char *strvBegin(strview_t ctx);
const char *strvEnd(strview_t ctx);
// move view forward by n characters
strview_t strvRemovePrefix(strview_t ctx, usize n);
// move view backwards by n characters
strview_t strvRemoveSuffix(strview_t ctx, usize n);
// removes whitespace from the beginning and the end
strview_t strvTrim(strview_t ctx);
// removes whitespace from the beginning
strview_t strvTrimLeft(strview_t ctx);
// removes whitespace from the end
strview_t strvTrimRight(strview_t ctx);

bool strvIsEmpty(strview_t ctx);

str_t strvCopy(strview_t ctx);
str_t strvCopyN(strview_t ctx, usize count, usize from);
usize strvCopyBuf(strview_t ctx, char *buf, usize len, usize from);

strview_t strvSub(strview_t ctx, usize from, usize to);
int strvCompare(strview_t ctx, strview_t other);
int strvICompare(strview_t ctx, strview_t other);

bool strvStartsWith(strview_t ctx, char c);
bool strvStartsWithView(strview_t ctx, strview_t view);

bool strvEndsWith(strview_t ctx, char c);
bool strvEndsWithView(strview_t ctx, strview_t view);

bool strvContains(strview_t ctx, char c);
bool strvContainsView(strview_t ctx, strview_t view);

usize strvFind(strview_t ctx, char c, usize from);
usize strvFindView(strview_t ctx, strview_t view, usize from);

usize strvRFind(strview_t ctx, char c, usize from);
usize strvRFindView(strview_t ctx, strview_t view, usize from);

// Finds the first occurrence of any of the characters of 'view' in this view
usize strvFindFirstOf(strview_t ctx, strview_t view, usize from);
usize strvFindLastOf(strview_t ctx, strview_t view, usize from);

usize strvFindFirstNot(strview_t ctx, char c, usize from);
usize strvFindFirstNotOf(strview_t ctx, strview_t view, usize from);
usize strvFindLastNot(strview_t ctx, char c, usize from);
usize strvFindLastNotOf(strview_t ctx, strview_t view, usize from);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define vec(T)                  T *

#define vecFree(vec)            ((vec) ? free(_vecheader(vec)),NULL : NULL)
#define vecCopy(src, dest)      (vecFree(dest), vecAdd(dest, vecCount(src)), memcpy(dest, src, vecCount(src)))

#define vecAppend(vec, val)     (_vecmaygrow(vec, 1), (vec)[_veclen(vec)] = (val), _veclen(vec)++)
#define vecLen(vec)             ((vec) ? _veclen(vec) : 0)
#define vecCap(vec)             ((vec) ? _veccap(vec) : 0)

#define vecBeg(vec)             (vec)
#define vecEnd(vec)             ((vec) ? (vec) + _veclen(vec) : NULL)
#define vecBack(vec)            ((vec)[_veclen(vec) - 1])

#define vecAdd(vec, n)          (_vecmaygrow(vec, (n)), _veclen(vec) += (size_type)(n), &(vec)[_veclen(vec)-(n)])
#define vecReserve(vec, n)      (_vecmaygrow(vec, (n)))
#define vecShrink(vec)          (_vecshrink((void **)&(vec), _veclen(vec), sizeof(*(vec))))

#define vecClear(vec)           ((vec) ? _veclen(vec) = 0 : 0)
#define vecPop(vec)             ((vec)[--_veclen(vec)])

// == IMPLEMENTATION ==========================================================================================

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#ifndef size_type
    #define size_type uint32_t
#endif

#define _vecheader(vec)         ((size_type *)(vec) - 2)
#define _veccap(vec)            _vecheader(vec)[0]
#define _veclen(vec)            _vecheader(vec)[1]

#define _vecneedgrow(vec, n)    ((vec) == NULL || _veclen(vec) + n >= _veccap(vec))
#define _vecmaygrow(vec, n)     (_vecneedgrow(vec, (n)) ? _vecgrow(vec, (size_type)(n)) : (void)0)
#define _vecgrow(vec, n)        _vecgrowimpl((void **)&(vec), (n), sizeof(*(vec)))

inline static void _vecgrowimpl(void **arr, size_type increment, size_type itemsize) {
    int newcap = *arr ? 2 * _veccap(*arr) + increment : increment + 1;
    void *ptr = realloc(*arr ? _vecheader(*arr) : 0, itemsize * newcap + sizeof(size_type) * 2);
    assert(ptr);
    if (ptr) {
        if (!*arr) ((size_type *)ptr)[1] = 0;
        *arr = (void *) ((size_type *)ptr + 2);
        _veccap(*arr) = newcap;
    }
}

inline static void _vecshrink(void **arr, size_type newcap, size_t itemsize) {
    if (newcap == _veccap(*arr) || !*arr) return;
    void *ptr = realloc(_vecheader(*arr), itemsize * newcap + sizeof(size_type) * 2);
    assert(ptr);
    if (ptr) {
        *arr = (void *) ((size_type *)ptr + 2);
        if (_veclen(*arr) < newcap) _veclen(*arr) = newcap;
        _veccap(*arr) = newcap;
    }
}

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

/* #include "collatypes.h" */
/* #include "vec.h" */
/* #include "str.h" */

/*
Example usage:
hashSetSeed(time(NULL));
vec(const char *) strings = NULL;
hashmap_t map = hmInit(32);

// mapGet returns 0 in case it doesn't find anything, this way we don't need
// to check its return value
vecAppend(strings, "nil");

hmSet(&map, hashCStr("english"), vecAppend(strings, "hello"));
hmSet(&map, hashCStr("french"),  vecAppend(strings, "bonjour"));
hmSet(&map, hashCStr("italian"), vecAppend(strings, "ciao"));

printf("english: %s\n", strings[hmGet(map, hashCStr("english"))]);
printf("french: %s\n",  strings[hmGet(map, hashCStr("french"))]);
printf("italian: %s\n", strings[hmGet(map, hashCStr("italian"))]);

mapFree(map);
vecFree(strings);
*/

typedef struct {
    uint64 hash;
    uint64 index;
} hashnode_t;

typedef struct {
    vec(hashnode_t) nodes;
} hashmap_t;

hashmap_t hmInit(usize initial_cap);
void hmFree(hashmap_t map);

void hmSet(hashmap_t *map, uint64 hash, uint64 index);
uint64 hmGet(hashmap_t map, uint64 hash);
void hmDelete(hashmap_t *map, uint64 hash);

void hashSetSeed(uint64 new_seed);
uint64 hash(const void *data, usize len);
uint64 hashStr(str_t str);
uint64 hashView(strview_t view);
uint64 hashCStr(const char *cstr);

#pragma once

/* #include "collatypes.h" */

typedef uint32 rune;

enum {
    UTF8_MAX_SIZE = 4,
    UTF8_INVALID = 0x80
};

// grabs the next UTF-8 codepoint and advances string ptr
rune utf8Decode(const char **str);
// encodes a codepoint as UTF-8 and returns the length
usize utf8Encode(char *str, rune ch);
// returns the size of the next UTF-8 codepoint
int utf8Size(const char *str);
// returns the size of a UTF-8 codepoint
usize utf8CpSize(rune ch);

#pragma once

#include <stdbool.h>
/* #include "str.h" */
/* #include "vec.h" */
/* #include "utf8.h" */

#ifdef __cplusplus
extern "C" {
#endif

// == INI READER ========================================================================

typedef struct {
    strview_t key;
    strview_t value;
} inivalue_t;

typedef struct {
    strview_t name;
    vec(inivalue_t) values;
} initable_t;

typedef struct {
    str_t text;
    vec(initable_t) tables;
} ini_t;

typedef struct {
    bool merge_duplicate_tables;
    bool merge_duplicate_keys;
} iniopts_t;

ini_t iniParse(const char *filename, const iniopts_t *options);
ini_t iniParseString(const char *inistr, const iniopts_t *options);
void iniFree(ini_t ctx);

initable_t *iniGetTable(ini_t *ctx, const char *name);
inivalue_t *iniGet(initable_t *ctx, const char *key);

vec(strview_t) iniAsArray(const inivalue_t *value, char delim);
// delim is expected to be a single utf8 character
vec(strview_t) iniAsArrayU8(const inivalue_t *value, const char *delim);
uint64 iniAsUInt(const inivalue_t *value);
int64 iniAsInt(const inivalue_t *value);
double iniAsNum(const inivalue_t *value);
bool iniAsBool(const inivalue_t *value);

// == INI WRITER ========================================================================

typedef struct {
    str_t key;
    str_t value;
} winivalue_t;

typedef struct {
    str_t key;
    vec(winivalue_t) values;
} winitable_t;

typedef struct {
    vec(winitable_t) tables;
} iniwriter_t;

typedef struct {
    bool no_discalimer;
} winiopts_t;

iniwriter_t winiInit();
void winiFree(iniwriter_t ctx);

str_t winiToString(iniwriter_t ctx, const winiopts_t *options);
void winiToFile(iniwriter_t ctx, const char *filename, const winiopts_t *options);

winivalue_t *winiAddValEmpty(winitable_t *table);
winivalue_t *winiAddVal(winitable_t *table, const char *key, const char *value);
winivalue_t *winiAddValStr(winitable_t *table, str_t key, str_t value);
winivalue_t *winiAddValView(winitable_t *table, strview_t key, strview_t value);

winitable_t *winiAddTablEmpty(iniwriter_t *ctx);
winitable_t *winiAddTab(iniwriter_t *ctx, const char *name);
winitable_t *winiAddTabStr(iniwriter_t *ctx, str_t name);
winitable_t *winiAddTabView(iniwriter_t *ctx, strview_t name);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

/* #include "collatypes.h" */
/* #include "str.h" */

/* == INPUT STREAM ============================================ */

typedef struct {
    const char *start;
    const char *cur;
    usize size;
} str_istream_t;

// initialize with null-terminated string
str_istream_t istrInit(const char *str);
str_istream_t istrInitLen(const char *str, usize len);

// get the current character and advance
char istrGet(str_istream_t *ctx);
// get the current character but don't advance
char istrPeek(str_istream_t *ctx);
// ignore characters until the delimiter
void istrIgnore(str_istream_t *ctx, char delim);
// ignore characters until the delimiter and skip it
void istrIgnoreAndSkip(str_istream_t *ctx, char delim);
// skip n characters
void istrSkip(str_istream_t *ctx, usize n);
// skips whitespace (' ', '\n', '\t', '\r')
void istrSkipWhitespace(str_istream_t *ctx);
// read len bytes into buffer, the buffer will not be null terminated
void istrRead(str_istream_t *ctx, char *buf, usize len);
// read a maximum of len bytes into buffer, the buffer will not be null terminated
// returns the number of bytes read
usize istrReadMax(str_istream_t *ctx, char *buf, usize len);
// returns to the beginning of the stream
void istrRewind(str_istream_t *ctx);
// returns back <amount> characters
void istrRewindN(str_istream_t *ctx, usize amount);
// returns the number of bytes read from beginning of stream
usize istrTell(str_istream_t ctx);
// returns the number of bytes left to read in the stream
usize istrRemaining(str_istream_t ctx); 
// return true if the stream doesn't have any new bytes to read
bool istrIsFinished(str_istream_t ctx);

bool istrGetbool(str_istream_t *ctx, bool *val);
bool istrGetu8(str_istream_t *ctx, uint8 *val);
bool istrGetu16(str_istream_t *ctx, uint16 *val);
bool istrGetu32(str_istream_t *ctx, uint32 *val);
bool istrGetu64(str_istream_t *ctx, uint64 *val);
bool istrGeti8(str_istream_t *ctx, int8 *val);
bool istrGeti16(str_istream_t *ctx, int16 *val);
bool istrGeti32(str_istream_t *ctx, int32 *val);
bool istrGeti64(str_istream_t *ctx, int64 *val);
bool istrGetfloat(str_istream_t *ctx, float *val);
bool istrGetdouble(str_istream_t *ctx, double *val);
// get a string until a delimiter, the string is allocated by the function and should be freed
usize istrGetstring(str_istream_t *ctx, char **val, char delim);
// get a string of maximum size len, the string is not allocated by the function and will be null terminated
usize istrGetstringBuf(str_istream_t *ctx, char *val, usize len);
strview_t istrGetview(str_istream_t *ctx, char delim);
strview_t istrGetviewLen(str_istream_t *ctx, usize from, usize to);

/* == OUTPUT STREAM =========================================== */

typedef struct {
    char *buf;
    usize len;
    usize cap;
} str_ostream_t;

str_ostream_t ostrInit(void);
str_ostream_t ostrInitLen(usize initial_alloc);
str_ostream_t ostrInitStr(const char *buf, usize len);

void ostrFree(str_ostream_t ctx);
void ostrClear(str_ostream_t *ctx);

char ostrBack(str_ostream_t ctx);
str_t ostrAsStr(str_ostream_t ctx);
strview_t ostrAsView(str_ostream_t ctx);

void ostrReplace(str_ostream_t *ctx, char from, char to);

void ostrPrintf(str_ostream_t *ctx, const char *fmt, ...);
void ostrPrintfV(str_ostream_t *ctx, const char *fmt, va_list args);
void ostrPutc(str_ostream_t *ctx, char c);
void ostrPuts(str_ostream_t *ctx, const char *str);

void ostrAppendbool(str_ostream_t *ctx, bool val);
void ostrAppendchar(str_ostream_t *ctx, char val);
void ostrAppendu8(str_ostream_t *ctx, uint8 val);
void ostrAppendu16(str_ostream_t *ctx, uint16 val);
void ostrAppendu32(str_ostream_t *ctx, uint32 val);
void ostrAppendu64(str_ostream_t *ctx, uint64 val);
void ostrAppendi8(str_ostream_t *ctx, int8 val);
void ostrAppendi16(str_ostream_t *ctx, int16 val);
void ostrAppendi32(str_ostream_t *ctx, int32 val);
void ostrAppendi64(str_ostream_t *ctx, int64 val);
void ostrAppendfloat(str_ostream_t *ctx, float val);
void ostrAppenddouble(str_ostream_t *ctx, double val);
void ostrAppendview(str_ostream_t *ctx, strview_t view);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#ifndef WIN32_EXTRA_LEAN
	#define WIN32_EXTRA_LEAN
#endif

#include <windows.h>

#endif // _WIN32 

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* #include "collatypes.h" */
/* #include "str.h" */
/* #include "vec.h" */

typedef enum {
    FILE_READ  = 1 << 0, 
    FILE_WRITE = 1 << 1, 
    FILE_CLEAR = 1 << 2, 
    FILE_BOTH  = 1 << 3
} filemode_t;

typedef uintptr_t file_t;

typedef struct {
    const uint8 *buf;
    usize len;
} filebuf_t;

bool fileExists(const char *fname);

file_t fileOpen(const char *fname, filemode_t mode);
void fileClose(file_t ctx);

bool fileIsValid(file_t ctx);

bool filePutc(file_t ctx, char c);
bool filePuts(file_t ctx, const char *str);
bool filePutstr(file_t ctx, str_t str);
bool filePutview(file_t ctx, strview_t view);

usize fileRead(file_t ctx, void *buf, usize len);
usize fileWrite(file_t ctx, const void *buf, usize len);

bool fileSeekEnd(file_t ctx);
void fileRewind(file_t ctx);

uint64 fileTell(file_t ctx);

vec(uint8) fileReadWhole(const char *fname);
vec(uint8) fileReadWholeFP(file_t ctx);

str_t fileReadWholeText(const char *fname);
str_t fileReadWholeTextFP(file_t ctx);

bool fileWriteWhole(const char *fname, filebuf_t data);
bool fileWriteWholeFP(file_t ctx, filebuf_t data);

bool fileWriteWholeText(const char *fname, strview_t string);
bool fileWriteWholeTextFP(file_t ctx, strview_t string);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* #include "str.h" */

typedef void *dir_t;

typedef enum {
    FS_TYPE_UNKNOWN,
    FS_TYPE_FILE,
    FS_TYPE_DIR,
} fs_type_t;

typedef struct {
    fs_type_t type;
    str_t name;
} dir_entry_t;

dir_t dirOpen(const char *path);
void dirClose(dir_t ctx);

bool dirValid(dir_t ctx);

dir_entry_t *dirNext(dir_t ctx);

void dirCreate(const char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef COLLA_NO_NET
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
    /* #include "win32_slim.h" */
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/* #include "collatypes.h" */
/* #include "str.h" */
/* #include "strstream.h" */
/* #include "socket.h" */

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

/* #include "vec.h" */

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

#endif // COLLA_NO_NET
#if !defined(__TINYC__) && !defined(COLLA_NO_THREADS)
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// == THREAD ===========================================

typedef uintptr_t cthread_t;

typedef int (*cthread_func_t)(void *);

cthread_t thrCreate(cthread_func_t func, void *arg);
bool thrValid(cthread_t ctx);
bool thrDetach(cthread_t ctx);

cthread_t thrCurrent(void);
int thrCurrentId(void);
int thrGetId(cthread_t ctx);

void thrExit(int code);
bool thrJoin(cthread_t ctx, int *code);

// == MUTEX ============================================

typedef uintptr_t cmutex_t;

cmutex_t mtxInit(void);
void mtxDestroy(cmutex_t ctx);

bool mtxValid(cmutex_t ctx);

bool mtxLock(cmutex_t ctx);
bool mtxTryLock(cmutex_t ctx);
bool mtxUnlock(cmutex_t ctx);

#ifdef __cplusplus
// small c++ class to make mutexes easier to use
struct lock_t {
    inline lock_t(cmutex_t mutex)
        : mutex(mutex) {
        if (mtxValid(mutex)) {
            mtxLock(mutex);
        }
    }

    inline ~lock_t() {
        unlock();
    }

    inline void unlock() {
        if (mtxValid(mutex)) {
            mtxUnlock(mutex);
        }
        mutex = 0;
    }
    
    cmutex_t mutex;
};
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // !defined(__TINYC__) && !defined(COLLA_NO_THREADS)
#ifdef COLLA_IMPL
/* #include "tracelog.h" */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32 
    #pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS.
    /* #include "win32_slim.h" */
    #ifndef TLOG_VS
        #define TLOG_WIN32_NO_VS
        #ifndef TLOG_NO_COLOURS
            #define TLOG_NO_COLOURS
        #endif
    #endif
#endif

#ifdef TLOG_VS
    #ifndef _WIN32
        #error "can't use TLOG_VS if not on windows"
    #endif
#endif

#ifdef TLOG_NO_COLOURS
    #define BLACK   ""
    #define RED     ""
    #define GREEN   ""
    #define YELLOW  ""
    #define BLUE    ""
    #define MAGENTA ""
    #define CYAN    ""
    #define WHITE   ""
    #define RESET   ""
    #define BOLD    ""
#else
    #define BLACK   "\033[30m"
    #define RED     "\033[31m"
    #define GREEN   "\033[32m"
    #define YELLOW  "\033[33m"
    #define BLUE    "\033[22;34m"
    #define MAGENTA "\033[35m"
    #define CYAN    "\033[36m"
    #define WHITE   "\033[37m"
    #define RESET   "\033[0m"
    #define BOLD    "\033[1m"
#endif

#define MAX_TRACELOG_MSG_LENGTH 1024

bool use_newline = true;

#ifdef TLOG_WIN32_NO_VS
static void setLevelColour(int level) {
    WORD attribute = 15;
    switch (level) {
        case LogDebug:   attribute = 1; break; 
        case LogInfo:    attribute = 2; break;
        case LogWarning: attribute = 6; break;
        case LogError:   attribute = 4; break;
        case LogFatal:   attribute = 4; break;
    }

    HANDLE hc = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hc, attribute);
}
#endif

void traceLog(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    traceLogVaList(level, fmt, args);
    va_end(args);
}

void traceLogVaList(int level, const char *fmt, va_list args) {
    char buffer[MAX_TRACELOG_MSG_LENGTH];
    memset(buffer, 0, sizeof(buffer));

    const char *beg;
    switch (level) {
        case LogTrace:   beg = BOLD WHITE  "[TRACE]: "   RESET; break; 
        case LogDebug:   beg = BOLD BLUE   "[DEBUG]: "   RESET; break; 
        case LogInfo:    beg = BOLD GREEN  "[INFO]: "    RESET; break; 
        case LogWarning: beg = BOLD YELLOW "[WARNING]: " RESET; break; 
        case LogError:   beg = BOLD RED    "[ERROR]: "   RESET; break; 
        case LogFatal:   beg = BOLD RED    "[FATAL]: "   RESET; break;        
        default:         beg = "";                              break;
    }

    size_t offset = 0;

#ifndef TLOG_WIN32_NO_VS
    offset = strlen(beg);
    strncpy(buffer, beg, sizeof(buffer));
#endif

    vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);

#if defined(TLOG_VS)
    OutputDebugStringA(buffer);
    if(use_newline) OutputDebugStringA("\n");
#elif defined(TLOG_WIN32_NO_VS)
    SetConsoleOutputCP(CP_UTF8);
    setLevelColour(level);
    printf("%s", beg);
    // set back to white
    setLevelColour(LogTrace);
    printf("%s", buffer);
    if(use_newline) puts("");
#else
    printf("%s", buffer);
    if(use_newline) puts("");
#endif

#ifndef TLOG_DONT_EXIT_ON_FATAL
    if (level == LogFatal) exit(1);
#endif
}

void traceUseNewline(bool newline) {
    use_newline = newline;
}

/* #include "strstream.h" */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h> // HUGE_VALF
/* #include "tracelog.h" */

#if defined(_WIN32) && defined(__TINYC__)
#define strtoull _strtoui64
#define strtoll _strtoi64
#define strtof strtod
#endif

/* == INPUT STREAM ============================================ */

str_istream_t istrInit(const char *str) {
    return istrInitLen(str, strlen(str));
}

str_istream_t istrInitLen(const char *str, usize len) {
    str_istream_t res;
    res.start = res.cur = str;
    res.size = len;
    return res;
}

char istrGet(str_istream_t *ctx) {
    return *ctx->cur++;
}

void istrIgnore(str_istream_t *ctx, char delim) {
    usize position = ctx->cur - ctx->start;
    usize i;
    for(i = position; 
        i < ctx->size && *ctx->cur != delim; 
        ++i, ++ctx->cur);
}

void istrIgnoreAndSkip(str_istream_t *ctx, char delim) {
    istrIgnore(ctx, delim);
    istrSkip(ctx, 1);
}

char istrPeek(str_istream_t *ctx) {
    return *ctx->cur;
}

void istrSkip(str_istream_t *ctx, usize n) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    if(n > remaining) {
        warn("skipping more then remaining: %zu -> %zu", n, remaining);
        return;
    }
    ctx->cur += n;
}

void istrSkipWhitespace(str_istream_t *ctx) {
    while (*ctx->cur && isspace(*ctx->cur)) {
        ++ctx->cur;
    }
}

void istrRead(str_istream_t *ctx, char *buf, usize len) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    if(len > remaining) {
        warn("istrRead: trying to read len %zu from remaining %zu", len, remaining);
        return;
    }
    memcpy(buf, ctx->cur, len);
    ctx->cur += len;
}

usize istrReadMax(str_istream_t *ctx, char *buf, usize len) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    len = remaining < len ? remaining : len;
    memcpy(buf, ctx->cur, len);
    ctx->cur += len;
    return len;
}

void istrRewind(str_istream_t *ctx) {
    ctx->cur = ctx->start;
}

void istrRewindN(str_istream_t *ctx, usize amount) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    if (amount > remaining) amount = remaining;
    ctx->cur -= amount;
}

usize istrTell(str_istream_t ctx) {
    return ctx.cur - ctx.start;
}

usize istrRemaining(str_istream_t ctx) {
    return ctx.size - (ctx.cur - ctx.start);
}

bool istrIsFinished(str_istream_t ctx) {
    return (usize)(ctx.cur - ctx.start) >= ctx.size;
}

bool istrGetbool(str_istream_t *ctx, bool *val) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    if(strncmp(ctx->cur, "true", remaining) == 0) {
        *val = true;
        return true;
    }
    if(strncmp(ctx->cur, "false", remaining) == 0) {
        *val = false;
        return true;
    }
    return false;
}

bool istrGetu8(str_istream_t *ctx, uint8 *val) {
    char *end = NULL;
    *val = (uint8) strtoul(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGetu8: no valid conversion could be performed");
        return false;
    }
    else if(*val == UINT8_MAX) {
        warn("istrGetu8: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGetu16(str_istream_t *ctx, uint16 *val) {
    char *end = NULL;
    *val = (uint16) strtoul(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGetu16: no valid conversion could be performed");
        return false;
    }
    else if(*val == UINT16_MAX) {
        warn("istrGetu16: value read is out of the range of representable values");
        return false;
    }
    
    ctx->cur = end;
    return true;
}

bool istrGetu32(str_istream_t *ctx, uint32 *val) {
    char *end = NULL;
    *val = (uint32) strtoul(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGetu32: no valid conversion could be performed");
        return false;
    }
    else if(*val == UINT32_MAX) {
        warn("istrGetu32: value read is out of the range of representable values");
        return false;
    }
    
    ctx->cur = end;
    return true;
}

bool istrGetu64(str_istream_t *ctx, uint64 *val) {
    char *end = NULL;
    *val = strtoull(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGetu64: no valid conversion could be performed");
        return false;
    }
    else if(*val == ULLONG_MAX) {
        warn("istrGetu64: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGeti8(str_istream_t *ctx, int8 *val) {
    char *end = NULL;
    *val = (int8) strtol(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGeti8: no valid conversion could be performed");
        return false;
    }
    else if(*val == INT8_MAX || *val == INT8_MIN) {
        warn("istrGeti8: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGeti16(str_istream_t *ctx, int16 *val) {
    char *end = NULL;
    *val = (int16) strtol(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGeti16: no valid conversion could be performed");
        return false;
    }
    else if(*val == INT16_MAX || *val == INT16_MIN) {
        warn("istrGeti16: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGeti32(str_istream_t *ctx, int32 *val) {
    char *end = NULL;
    *val = (int32) strtol(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGeti32: no valid conversion could be performed");
        return false;
    }
    else if(*val == INT32_MAX || *val == INT32_MIN) {
        warn("istrGeti32: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGeti64(str_istream_t *ctx, int64 *val) {
    char *end = NULL;
    *val = strtoll(ctx->cur, &end, 0);
    
    if(ctx->cur == end) {
        warn("istrGeti64: no valid conversion could be performed");
        return false;
    }
    else if(*val == INT64_MAX || *val == INT64_MIN) {
        warn("istrGeti64: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGetfloat(str_istream_t *ctx, float *val) {
    char *end = NULL;
    *val = strtof(ctx->cur, &end);
    
    if(ctx->cur == end) {
        warn("istrGetfloat: no valid conversion could be performed");
        return false;
    }
    else if(*val == HUGE_VALF || *val == -HUGE_VALF) {
        warn("istrGetfloat: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istrGetdouble(str_istream_t *ctx, double *val) {
    char *end = NULL;
    *val = strtod(ctx->cur, &end);
    
    if(ctx->cur == end) {
        warn("istrGetdouble: no valid conversion could be performed");
        return false;
    }
    else if(*val == HUGE_VAL || *val == -HUGE_VAL) {
        warn("istrGetdouble: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

usize istrGetstring(str_istream_t *ctx, char **val, char delim) {
    const char *from = ctx->cur;
    istrIgnore(ctx, delim);
    // if it didn't actually find it, it just reached the end of the string
    if(*ctx->cur != delim) {
        *val = NULL;
        return 0;
    }
    usize len = ctx->cur - from;
    *val = (char *)malloc(len + 1);
    memcpy(*val, from, len);
    (*val)[len] = '\0';
    return len;
}

usize istrGetstringBuf(str_istream_t *ctx, char *val, usize len) {
    usize remaining = ctx->size - (ctx->cur - ctx->start);
    len -= 1;
    len = remaining < len ? remaining : len;
    memcpy(val, ctx->cur, len);
    val[len] = '\0';
    ctx->cur += len;
    return len;
}

strview_t istrGetview(str_istream_t *ctx, char delim) {
    const char *from = ctx->cur;
    istrIgnore(ctx, delim);
    usize len = ctx->cur - from;
    return strvInitLen(from, len);
}

strview_t istrGetviewLen(str_istream_t *ctx, usize from, usize to) {
    usize len = ctx->size - (ctx->cur - ctx->start) - from;
    if (to > len) to = len;
    if (from > to) from = to;
    return strvInitLen(ctx->cur + from, to - from);
}

/* == OUTPUT STREAM =========================================== */

static void _ostrRealloc(str_ostream_t *ctx, usize needed) {
    ctx->cap = (ctx->cap * 2) + needed;
    ctx->buf = (char *)realloc(ctx->buf, ctx->cap);
}

str_ostream_t ostrInit() {
    return ostrInitLen(1);
}

str_ostream_t ostrInitLen(usize initial_alloc) {
    str_ostream_t stream;
    stream.buf = (char *)calloc(initial_alloc, 1);
    stream.len = 0;
    stream.cap = initial_alloc;
    return stream;
}

str_ostream_t ostrInitStr(const char *cstr, usize len) {
    str_ostream_t stream;
    stream.buf = (char *)malloc(len + 1);
    memcpy(stream.buf, cstr, len);
    stream.len = len;
    stream.cap = len + 1;
    return stream;
}

void ostrFree(str_ostream_t ctx) {
    free(ctx.buf);
}

void ostrClear(str_ostream_t *ctx) {
    ctx->len = 0;
}

char ostrBack(str_ostream_t ctx) {
    if(ctx.len == 0) return '\0';
    return ctx.buf[ctx.len - 1];
}

str_t ostrAsStr(str_ostream_t ctx) {
    return (str_t) {
        .buf = ctx.buf,
        .len = ctx.len
    };
}

strview_t ostrAsView(str_ostream_t ctx) {
    return (strview_t) {
        .buf = ctx.buf,
        .len = ctx.len
    };
}

void ostrReplace(str_ostream_t *ctx, char from, char to) {
    for(usize i = 0; i < ctx->len; ++i) {
        if(ctx->buf[i] == from) {
            ctx->buf[i] = to;
        }
    }
}

void ostrPrintf(str_ostream_t *ctx, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    ostrPrintfV(ctx, fmt, va);
    va_end(va);
}

void ostrPrintfV(str_ostream_t *ctx, const char *fmt, va_list args) {
    va_list vtemp;
    int len;
    usize remaining;
    
    // vsnprintf returns the length of the formatted string, even if truncated
    // we use this to get the actual length of the formatted string
    va_copy(vtemp, args);
    len = vsnprintf(NULL, 0, fmt, vtemp);
    va_end(vtemp);
    if(len < 0) {
        err("couldn't format string \"%s\"", fmt);
        goto error;
    }

    remaining = ctx->cap - ctx->len;
    if(remaining < (usize)len) {
        _ostrRealloc(ctx, len + 1);
        remaining = ctx->cap - ctx->len;
    }

    // actual formatting here
    va_copy(vtemp, args);
    len = vsnprintf(ctx->buf + ctx->len, remaining, fmt, vtemp);
    va_end(vtemp);
    if(len < 0) {
        err("couldn't format stringh \"%s\"", fmt);
        goto error;
    }
    ctx->len += len;

error:
    return;
}


#define APPEND_BUF_LEN 20

void ostrPutc(str_ostream_t *ctx, char c) {
    ostrAppendchar(ctx, c);
}

void ostrPuts(str_ostream_t *ctx, const char *str) {
    ostrAppendview(ctx, strvInit(str));
}

void ostrAppendbool(str_ostream_t *ctx, bool val) {
    ostrAppendview(ctx, strvInit(val ? "true" : "false"));
}

void ostrAppendchar(str_ostream_t *ctx, char val) {
    if(ctx->len >= ctx->cap) {
        _ostrRealloc(ctx, 1);
    }
    ctx->buf[ctx->len++] = val;
    ctx->buf[ctx->len] = '\0';
}

void ostrAppendu8(str_ostream_t *ctx, uint8 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%hhu", val);
    if(len <= 0) {
        err("ostrAppendu8: couldn't write %hhu", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendu16(str_ostream_t *ctx, uint16 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%hu", val);
    if(len <= 0) {
        err("ostrAppendu16: couldn't write %hu", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendu32(str_ostream_t *ctx, uint32 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%u", val);
    if(len <= 0) {
        err("ostrAppendu32: couldn't write %u", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendu64(str_ostream_t *ctx, uint64 val) {
    char buf[APPEND_BUF_LEN];
#if _WIN32
    int len = snprintf(buf, sizeof(buf), "%llu", val);
#else
    int len = snprintf(buf, sizeof(buf), "%lu", val);
#endif
    if(len <= 0) {
        err("ostrAppendu64: couldn't write %lu", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendi8(str_ostream_t *ctx, int8 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%hhi", val);
    if(len <= 0) {
        err("ostrAppendi8: couldn't write %hhi", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendi16(str_ostream_t *ctx, int16 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%hi", val);
    if(len <= 0) {
        err("ostrAppendi16: couldn't write %hi", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendi32(str_ostream_t *ctx, int32 val) {
    char buf[APPEND_BUF_LEN];
    int len = snprintf(buf, sizeof(buf), "%i", val);
    if(len <= 0) {
        err("ostrAppendi32: couldn't write %i", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendi64(str_ostream_t *ctx, int64 val) {
    char buf[APPEND_BUF_LEN];
#if _WIN32
    int len = snprintf(buf, sizeof(buf), "%lli", val);
#else
    int len = snprintf(buf, sizeof(buf), "%li", val);
#endif
    if(len <= 0) {
        err("ostrAppendi64: couldn't write %li", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendfloat(str_ostream_t *ctx, float val) {
    char buf[APPEND_BUF_LEN * 3];
    int len = snprintf(buf, sizeof(buf), "%g", (double)val);
    if(len <= 0) {
        err("ostrAppendfloat: couldn't write %g", (double)val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppenddouble(str_ostream_t *ctx, double val) {
    char buf[APPEND_BUF_LEN * 3];
    int len = snprintf(buf, sizeof(buf), "%g", val);
    if(len <= 0) {
        err("ostrAppenddouble: couldn't write %g", val);
        return;
    } 
    ostrAppendview(ctx, strvInitLen(buf, len));
}

void ostrAppendview(str_ostream_t *ctx, strview_t view) {
    if((ctx->cap - ctx->len) <= view.len) {
        _ostrRealloc(ctx, view.len + 1);
    }
    memcpy(ctx->buf + ctx->len, view.buf, view.len);
    ctx->len += view.len;
    ctx->buf[ctx->len] = '\0';
}

/* #include "str.h" */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

/* #include "tracelog.h" */
/* #include "strstream.h" */

#ifdef _WIN32
/* #include "win32_slim.h" */
#else
#include <iconv.h>
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// == STR_T ========================================================

str_t strInit(void) {
    return (str_t) {
        .buf = NULL,
        .len = 0
    };
}

str_t strFromStr(const char *cstr) {
    return cstr ? strFromBuf(cstr, strlen(cstr)) : strInit();
}

str_t strFromView(strview_t view) {
    return strFromBuf(view.buf, view.len);
}

str_t strFromBuf(const char *buf, usize len) {
    if (!buf) return strInit();
    str_t str;
    str.len = len;
    str.buf = (char *)malloc(len + 1);
    memcpy(str.buf, buf, len);
    str.buf[len] = '\0';
    return str;
}

str_t strFromFmt(const char *fmt, ...) {
    str_ostream_t out = ostrInit();
    va_list va;
    va_start(va, fmt);
    ostrPrintfV(&out, fmt, va);
    va_end(va);
    return ostrAsStr(out);
}

void strFree(str_t ctx) {
    free(ctx.buf);
}

str_t strFromWCHAR(const wchar_t *src, usize len) {
    if(len == 0) len = wcslen(src);

#ifdef _WIN32
    // TODO CP_ACP should be CP_UTF8 but if i put CP_UTF8 it doesn't work?? 
    int result_len = WideCharToMultiByte(
        CP_ACP, 0, 
        src, (int)len, 
        NULL, 0,
        NULL, NULL
    );
    char *buf = (char *)malloc(result_len + 1);
    if(buf) {
        WideCharToMultiByte(
            CP_ACP, 0, 
            src, (int)len, 
            buf, result_len, 
            NULL, NULL
        );
        buf[result_len] = '\0';
    }
    return (str_t) {
        .buf = buf,
        .len = result_len
    };
#else
    usize actual_len = len * sizeof(wchar_t);

    usize dest_len = len * 6;
    char *dest = (char *)malloc(dest_len);

    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    assert(cd);

    usize dest_left = dest_len;
    char *dest_temp = dest;
    char *src_temp = (char*)src;
    usize lost = iconv(cd, &src_temp, &actual_len, &dest_temp, &dest_left);
    assert(lost != ((usize)-1));
    (void)lost;

    dest_len -= dest_left;
    dest = (char *)realloc(dest, dest_len + 1);
    dest[dest_len] = '\0';

    iconv_close(cd);

    return (str_t){
        .buf = dest,
        .len = dest_len
    };
#endif
}

wchar_t *strToWCHAR(str_t ctx) {
#ifdef _WIN32
    UINT codepage = CP_ACP;
    int len = MultiByteToWideChar(
        codepage, 0,
        ctx.buf, (int)ctx.len,
        NULL, 0
    );
    wchar_t *str = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
    if(!str) return NULL;
    len = MultiByteToWideChar(
        codepage, 0,
        ctx.buf, (int)ctx.len,
        str, len
    );
    str[len] = '\0';
    return str;
#else
    usize dest_len = ctx.len * sizeof(wchar_t);
    char *dest = (char *)malloc(dest_len);

    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    assert(cd);

    usize dest_left = dest_len;
    char *dest_temp = dest;
    char *src_temp = ctx.buf;
    usize lost = iconv(cd, &src_temp, &ctx.len, &dest_temp, &dest_left);
    assert(lost != ((usize)-1));
    (void)lost;

    dest_len -= dest_left;
    dest = (char *)realloc(dest, dest_len + 1);
    dest[dest_len] = '\0';

    iconv_close(cd);

    return (wchar_t *)dest;
#endif
}

str_t strDup(str_t ctx) {
    return strFromBuf(ctx.buf, ctx.len);
}

str_t strMove(str_t *ctx) {
    str_t out = *ctx;
    ctx->buf = NULL;
    ctx->len = 0;
    return out;
}

strview_t strGetView(str_t ctx) {
    return (strview_t) {
        .buf = ctx.buf,
        .len = ctx.len
    };
}

char *strBegin(str_t ctx) {
    return ctx.buf;
}

char *strEnd(str_t ctx) {
    return ctx.buf ? ctx.buf + ctx.len : NULL;
}

char strBack(str_t ctx) {
    return ctx.buf ? ctx.buf[ctx.len - 1] : '\0';
}

bool strIsEmpty(str_t ctx) {
    return ctx.len == 0;
}

void strSwap(str_t *ctx, str_t *other) {
    char *buf  = other->buf;
    usize len = other->len;
    other->buf = ctx->buf;
    other->len = ctx->len;
    ctx->buf = buf;
    ctx->len = len;
}

void strReplace(str_t *ctx, char from, char to) {
    for(usize i = 0; i < ctx->len; ++i) {
        if(ctx->buf[i] == from) {
            ctx->buf[i] = to;
        }
    }
}

str_t strSubstr(str_t ctx, usize from, usize to) {
    if(strIsEmpty(ctx)) return strInit();
    if (to > ctx.len) to = ctx.len;
    if (from > to) from = to;
    return strFromBuf(ctx.buf + from, to - from);
}

strview_t strSubview(str_t ctx, usize from, usize to) {
    if(strIsEmpty(ctx)) return strvInit(NULL);
    if (to > ctx.len) to = ctx.len;
    if (from > to) from = to;
    return strvInitLen(ctx.buf + from, to - from);
}

void strLower(str_t *ctx) {
    for(usize i = 0; i < ctx->len; ++i) {
        ctx->buf[i] = (char)tolower(ctx->buf[i]);
    }
}

str_t strToLower(str_t ctx) {
    str_t str = strDup(ctx);
    strLower(&str);
    return str;
}

void strUpper(str_t *ctx) {
    for(usize i = 0; i < ctx->len; ++i) {
        ctx->buf[i] = (char)toupper(ctx->buf[i]);
    }
}

str_t strToUpper(str_t ctx) {
    str_t str = strDup(ctx);
    strUpper(&str);
    return str;
}

// == STRVIEW_T ====================================================

strview_t strvInit(const char *cstr) {
    return strvInitLen(cstr, cstr ? strlen(cstr) : 0);
}

strview_t strvInitStr(str_t str) {
    return strvInitLen(str.buf, str.len);
}

strview_t strvInitLen(const char *buf, usize size) {
    return (strview_t) {
        .buf = buf,
        .len = size
    };
}

char strvFront(strview_t ctx) {
    return ctx.buf[0];
}

char strvBack(strview_t ctx) {
    return ctx.buf[ctx.len - 1];
}

const char *strvBegin(strview_t ctx) {
    return ctx.buf;
}

const char *strvEnd(strview_t ctx) {
    return ctx.buf + ctx.len;
}

bool strvIsEmpty(strview_t ctx) {
    return ctx.len == 0;
}

strview_t strvRemovePrefix(strview_t ctx, usize n) {
    if (n > ctx.len) n = ctx.len;
    return (strview_t){
        .buf = ctx.buf + n,
        .len = ctx.len - n
    };
}

strview_t strvRemoveSuffix(strview_t ctx, usize n) {
    if (n > ctx.len) n = ctx.len;
    return (strview_t){
        .buf = ctx.buf,
        .len = ctx.len - n
    };
}  

strview_t strvTrim(strview_t ctx) {
    return strvTrimLeft(strvTrimRight(ctx));
}

strview_t strvTrimLeft(strview_t ctx) {
    strview_t out = ctx;
    for (usize i = 0; i < ctx.len && isspace(ctx.buf[i]); ++i) {
        ++out.buf;
        --out.len;
    }
    return out;
}

strview_t strvTrimRight(strview_t ctx) {
    strview_t out = ctx;
    for (isize i = ctx.len - 1; i >= 0 && isspace(ctx.buf[i]); --i) {
        --out.len;
    }
    return out;
}

str_t strvCopy(strview_t ctx) {
    return strFromView(ctx);
}

str_t strvCopyN(strview_t ctx, usize count, usize from) {
    usize sz = ctx.len + 1 - from;
    count = min(count, sz);
    return strFromBuf(ctx.buf + from, count);
}

usize strvCopyBuf(strview_t ctx, char *buf, usize len, usize from) {
    usize sz = ctx.len + 1 - from;
    len = min(len, sz);
    memcpy(buf, ctx.buf + from, len);
    buf[len - 1] = '\0';
    return len - 1;
}

strview_t strvSub(strview_t ctx, usize from, usize to) {
    if (to > ctx.len) to = ctx.len;
    if (from > to) from = to;
    return strvInitLen(ctx.buf + from, to - from);
}

int strvCompare(strview_t ctx, strview_t other) {
    if(ctx.len < other.len) return -1;
    if(ctx.len > other.len) return  1;
    return memcmp(ctx.buf, other.buf, ctx.len);
}

int strvICompare(strview_t ctx, strview_t other) {
    if(ctx.len < other.len) return -1;
    if(ctx.len > other.len) return  1;
    for(usize i = 0; i < ctx.len; ++i) {
        int a = tolower(ctx.buf[i]);
        int b = tolower(other.buf[i]);
        if(a != b) return a - b;
    }
    return 0;
}

bool strvStartsWith(strview_t ctx, char c) {
    return strvFront(ctx) == c;
}

bool strvStartsWithView(strview_t ctx, strview_t view) {
    if(ctx.len < view.len) return false;
    return memcmp(ctx.buf, view.buf, view.len) == 0;
}

bool strvEndsWith(strview_t ctx, char c) {
    return strvBack(ctx) == c;
}

bool strvEndsWithView(strview_t ctx, strview_t view) {
    if(ctx.len < view.len) return false;
    return memcmp(ctx.buf + ctx.len - view.len, view.buf, view.len) == 0;
}

bool strvContains(strview_t ctx, char c) {
    for(usize i = 0; i < ctx.len; ++i) {
        if(ctx.buf[i] == c) return true;
    }
    return false;
}

bool strvContainsView(strview_t ctx, strview_t view) {
    if(ctx.len < view.len) return false;
    usize end = ctx.len - view.len;
    for(usize i = 0; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.len) == 0) return true;
    }
    return false;
}

usize strvFind(strview_t ctx, char c, usize from) {
    for(usize i = from; i < ctx.len; ++i) {
        if(ctx.buf[i] == c) return i;
    }
    return SIZE_MAX;
}

usize strvFindView(strview_t ctx, strview_t view, usize from) {
    if(ctx.len < view.len) return SIZE_MAX;
    usize end = ctx.len - view.len;
    for(usize i = from; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.len) == 0) return i;
    }
    return SIZE_MAX;
}

usize strvRFind(strview_t ctx, char c, usize from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(*buf == c) return (buf - ctx.buf);
    }

    return SIZE_MAX;
}

usize strvRFindView(strview_t ctx, strview_t view, usize from) {
    from = min(from, ctx.len);

    if(view.len > ctx.len) {
        from -= view.len;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(memcmp(buf, view.buf, view.len) == 0) return (buf - ctx.buf);
    }
    return SIZE_MAX;
}

usize strvFindFirstOf(strview_t ctx, strview_t view, usize from) {
    if(ctx.len < view.len) return SIZE_MAX;
    for(usize i = from; i < ctx.len; ++i) {
        for(usize j = 0; j < view.len; ++j) {
            if(ctx.buf[i] == view.buf[j]) return i;
        }
    }
    return SIZE_MAX;
}

usize strvFindLastOf(strview_t ctx, strview_t view, usize from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        for(usize j = 0; j < view.len; ++j) {
            if(*buf == view.buf[j]) return (buf - ctx.buf);
        }
    }

    return SIZE_MAX;
}

usize strvFindFirstNot(strview_t ctx, char c, usize from) {
    usize end = ctx.len - 1;
    for(usize i = from; i < end; ++i) {
        if(ctx.buf[i] != c) return i;
    }
    return SIZE_MAX;
}

usize strvFindFirstNotOf(strview_t ctx, strview_t view, usize from) {
    for(usize i = from; i < ctx.len; ++i) {
        if(!strvContains(view, ctx.buf[i])) {
            return i;
        }
    }
    return SIZE_MAX;
}

usize strvFindLastNot(strview_t ctx, char c, usize from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(*buf != c) {
            return buf - ctx.buf;
        }
    }

    return SIZE_MAX;
}

usize strvFindLastNotOf(strview_t ctx, strview_t view, usize from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(!strvContains(view, *buf)) {
            return buf - ctx.buf;
        }
    }

    return SIZE_MAX;
}

#ifdef STR_TESTING
#include <stdio.h>
/* #include "tracelog.h" */

void strTest(void) {
    str_t s;
    debug("== testing init =================");
    {
        s = strInit();
        printf("%s %zu\n", s.buf, s.len);
        strFree(&s);
        s = strInitStr("hello world");
        printf("\"%s\" %zu\n", s.buf, s.len);
        strFree(&s);
        uint8 buf[] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
        s = strFromBuf((char *)buf, sizeof(buf));
        printf("\"%s\" %zu\n", s.buf, s.len);
        strFree(&s);
    }
    debug("== testing view =================");
    {
        s = strInitStr("hello world");
        strview_t view = strGetView(&s);
        printf("\"%.*s\" %zu\n", (int)view.len, view.buf, view.len);
        strFree(&s);
    }
    debug("== testing begin/end ============");
    {
        s = strInitStr("hello world");
        char *beg = strBegin(&s);
        char *end = strEnd(&s);
        printf("[ ");
        for(; beg < end; ++beg) {
            printf("%c ", *beg);
        }
        printf("]\n");
        strFree(&s);
    }
    debug("== testing back/isempty =========");
    {
        s = strInitStr("hello world");
        printf("[ ");
        while(!strIsEmpty(&s)) {
            printf("%c ", strBack(&s));
            strPop(&s);
        }
        printf("]\n");
        strFree(&s);
    }
    debug("== testing append ===============");
    {
        s = strInitStr("hello ");
        printf("\"%s\" %zu\n", s.buf, s.len);
        strAppend(&s, "world");
        printf("\"%s\" %zu\n", s.buf, s.len);
        strAppendView(&s, strvInit(", how is it "));
        printf("\"%s\" %zu\n", s.buf, s.len);
        uint8 buf[] = { 'g', 'o', 'i', 'n', 'g' };
        strAppendBuf(&s, (char*)buf, sizeof(buf));
        printf("\"%s\" %zu\n", s.buf, s.len);
        strAppendChars(&s, '?', 2);
        printf("\"%s\" %zu\n", s.buf, s.len);
        strFree(&s);
    }
    debug("== testing push/pop =============");
    {
        s = strInit();
        str_t s2 = strInitStr("hello world");

        printf("%-14s %-14s\n", "s", "s2");
        printf("----------------------------\n");
        while(!strIsEmpty(&s2)) {
            printf("%-14s %-14s\n", s.buf, s2.buf);
            strPush(&s, strPop(&s2));
        }
        printf("%-14s %-14s\n", s.buf, s2.buf);

        strFree(&s);
        strFree(&s2);
    }
    debug("== testing swap =================");
    {
        s = strInitStr("hello");
        str_t s2 = strInitStr("world");
        printf("%-8s %-8s\n", "s", "s2");
        printf("----------------\n");
        printf("%-8s %-8s\n", s.buf, s2.buf);
        strSwap(&s, &s2);
        printf("%-8s %-8s\n", s.buf, s2.buf);

        strFree(&s);
        strFree(&s2);
    }
    debug("== testing substr ===============");
    {
        s = strInitStr("hello world");
        printf("s: %s\n", s.buf);
        
        printf("-- string\n");
        str_t s2 = strSubstr(&s, 0, 5);
        printf("0..5: \"%s\"\n", s2.buf);
        strFree(&s2);
        
        s2 = strSubstr(&s, 5, SIZE_MAX);
        printf("6..SIZE_MAX: \"%s\"\n", s2.buf);
        strFree(&s2);

        printf("-- view\n");
        strview_t v = strSubview(&s, 0, 5);
        printf("0..5: \"%.*s\"\n", (int)v.len, v.buf);
        v = strSubview(&s, 5, SIZE_MAX);
        printf("6..SIZE_MAX: \"%.*s\"\n", (int)v.len, v.buf);

        strFree(&s);
    }

    strFree(&s);
}
#endif

/* #include "hashmap.h" */

#include <string.h>

static uint64 hash_seed = 0;

hashmap_t hmInit(usize initial_cap) {
    hashmap_t map = {0};
    if (!initial_cap) initial_cap = 512;
    vecReserve(map.nodes, initial_cap);
    memset(map.nodes, 0, sizeof(hashnode_t) * initial_cap);
    return map;
}

void hmFree(hashmap_t map) {
    vecFree(map.nodes);
}

void hmSet(hashmap_t *map, uint64 hash, uint64 index) {
    uint32 hm_index = hash % vecCap(map->nodes);

    while (map->nodes[hm_index].hash) {
        hashnode_t *node = &map->nodes[hm_index];
        if (node->hash == hash) {
            node->index = index;
            return;
        }
        hm_index = (hm_index + 1) % vecCap(map->nodes);
    }

    map->nodes[hm_index].hash = hash;
    map->nodes[hm_index].index = index;
    _veclen(map->nodes)++;

    float load_factor = (float)vecLen(map->nodes) / (float)vecCap(map->nodes);
    if (load_factor > 0.75f) {
        uint32 old_cap = vecCap(map->nodes);
        vecReserve(map->nodes, old_cap);
        for (usize i = old_cap; i < vecCap(map->nodes); ++i) {
            map->nodes[i].hash = 0;
            map->nodes[i].index = 0;
        }
    }
}

uint64 hmGet(hashmap_t map, uint64 hash) {
    uint32 hm_index = hash % vecCap(map.nodes);

    do {
        hashnode_t *node = &map.nodes[hm_index];
        if (node->hash == hash) {
            return node->index;
        }
        hm_index = (hm_index + 1) % vecCap(map.nodes);
    } while (map.nodes[hm_index].hash);

    return 0;
}

void hmDelete(hashmap_t *map, uint64 hash) {
    uint32 hm_index = hash % vecCap(map->nodes);

    do {
        hashnode_t *node = &map->nodes[hm_index];
        if (node->hash == hash) {
            node->hash = 0;
            node->index = 0;
            break;
        }
        hm_index = (hm_index + 1) % vecCap(map->nodes);
    } while (map->nodes[hm_index].hash);

    if(vecLen(map->nodes)) _veclen(map->nodes)--;
}

void hashSetSeed(uint64 new_seed) {
    hash_seed = new_seed;
}

uint64 hash(const void *ptr, usize len) {
    const uint64 m = 0xC6A4A7935BD1E995LLU;
    const int r = 47;

    uint64 h = hash_seed ^ (len * m);

    const uint64 *data = (const uint64 *)ptr;
    const uint64 *end = (len >> 3) + data;

    while (data != end) {
        uint64 k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char *)data;

    switch(len & 7) {
    case 7: h ^= (uint64_t)(data2[6]) << 48;
    case 6: h ^= (uint64_t)(data2[5]) << 40;
    case 5: h ^= (uint64_t)(data2[4]) << 32;
    case 4: h ^= (uint64_t)(data2[3]) << 24;
    case 3: h ^= (uint64_t)(data2[2]) << 16;
    case 2: h ^= (uint64_t)(data2[1]) << 8;
    case 1: h ^= (uint64_t)(data2[0]);
            h *= m;
    };
    
    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

uint64 hashStr(str_t str) {
    return hash(str.buf, str.len);
}

uint64 hashView(strview_t view) {
    return hash(view.buf, view.len);
}

uint64 hashCStr(const char *cstr) {
    return hash(cstr, strlen(cstr));
}

/* #include "utf8.h" */

static const uint8 masks[] = {
    0x7f, // 0111-1111
    0x1f, // 0001-1111
    0x0f, // 0000-1111
    0x07, // 0000-0111
    0x03, // 0000-0011
    0x01  // 0000-0001
};

struct {
    uint8 mask;
    uint8 result;
    int octets;
} sizes[] = {
    { 0x80, 0x00, 1 },  // 1000-0000, 0000-0000
    { 0xE0, 0xC0, 2 },  // 1110-0000, 1100-0000
    { 0xF0, 0xE0, 3 },  // 1111-0000, 1110-0000
    { 0xF8, 0xF0, 4 },  // 1111-1000, 1111-0000
    { 0xFC, 0xF8, 5 },  // 1111-1100, 1111-1000
    { 0xFE, 0xF8, 6 },  // 1111-1110, 1111-1000
    { 0x80, 0x80, -1 }, // 1000-0000, 1000-0000
};


/*
UTF-8 codepoints are encoded using the first bits of the first character

byte 1    | byte 2    | byte 3    | byte 4   
0xxx xxxx |           |           | 
110x xxxx | 10xx xxxx |           | 
1110 xxxx | 10xx xxxx | 10xx xxxx | 
1111 0xxx | 10xx xxxx | 10xx xxxx | 10xx xxxx  

so when we decode it we first find the size of the codepoint (from 1 to 4)
then we apply the mask to the first byte to get the first character
then we keep shifting the rune left 6 and applying the next byte to the mask
until the codepoint is finished (size is 0)

## EXAMPLE

utf8 string (€) = 1110-0010 1000-0010 1010-1100

cp = 0000-0000 0000-0000 0000-0000 0000-0000
size = 3
mask = 0x0f -> 0000-1111
cp = *s & mask = 1110-0010 & 0000-1111 = 0000-0000 0000-0000 0000-0000 0000-0010
++s = 1000-0010

--size = 2
cp <<= 6 = 0000-0000 0000-0000 0000-0000 1000-0000
cp |= *s & 0x3f = 1000-0010 & 0011-1111 = 0000-0000 0000-0000 0000-0000 1000-0010
++s = 1010-1100

--size = 1
cp <<= 6 = 0000-0000 0000-0000 0010-0000 1000-0000
cp |= *s & 0x3f = 1010-1100 & 0011-1111 = 0000-0000 0000-0000 0010-0000 1010-1100
++s = ----------

final codepoint = 0010-0000 1010-1100
€ codepoint     = 0010-0000 1010-1100
*/

rune utf8Decode(const char **char_str) {
    uint8 **s = (uint8 **)char_str;

    rune ch = 0;
    // if is ascii
    if (**s < 128) {
        ch = **s;
        ++*s;
        return ch;
    }
    int size = utf8Size((char *)*s);
    if (size == -1) {
        ++*s;
        return UTF8_INVALID;
    }
    uint8 mask = masks[size - 1];
    ch = **s & mask;
    ++*s;
    while(--size) {
        ch <<= 6;
        ch |= **s & 0x3f; // 0011-1111
        ++*s;
    }
    return ch;
}


/*
to encode a codepoint in a utf8 string we first need to find 
the length of the codepoint
then we start from the rightmost byte and loop for each byte of the codepoint
using the length we got before until the first byte (which we skip)
> and (&) with 0x3f so we ignore the first to bits of the codepoint
> or (|) with 0x80 so we make sure that the first two bits are 10
> bitshift the codepoint right 6

finally, we apply the correct length-mask to the first byte

## EXAMPLE

ch € = 0010-0000 1010-1100
ch < 0x10000 
    first = 0xe0 = 1110-0000
    len = 3

str[2] = (ch & 0x3f) | 0x80 = 1010-1100 & 0011-1111 | 1000-0000
       = 1010-1100
ch >>= 6 = 0010-0000 1010-1100 >> 6 = 1000-0010

str[1] = (ch & 0x3f) | 0x80 = 1000-0010 & 0011-1111 | 1000-000
       = 1000-0010
ch >>= 6 = 1000-0010 >> 6 = 0000-0010

str[0] = ch | first_mask = 0000-0010 | 1111-0000
       = 1111-0010

str    = 1111-0010 1000-0010 1010-1100
utf8 € = 1110-0010 1000-0010 1010-1100
*/

usize utf8Encode(char *str, rune codepoint) {
    usize len = 0;
    uint8 first;

    if (codepoint < 0x80) {            // 0000-0000 0000-0000 0000-0000 1000-0000
        first = 0;
        len = 1;
    }
    else if (codepoint < 0x800) {      // 0000-0000 0000-0000 0000-1000 0000-0000
        first = 0xc0; // 1100-0000
        len = 2;
    }
    else if (codepoint < 0x10000) {    // 0000-0000 0000-0001 0000-0000 0000-0000
        first = 0xe0; // 1110-0000
        len = 3;
    }
    else {
        first = 0xf0; // 1111-0000
        len = 4;
    }

    for (usize i = len - 1; i > 0; --i) {
        // 0x3f -> 0011-1111
        // 0x80 -> 1000-0000
        str[i] = (codepoint & 0x3f) | 0x80;
        codepoint >>= 6;
    }

    str[0] = (char)(codepoint | first);
    return len;
}

int utf8Size(const char *str) {
    uint8 c = (uint8)*str;
    for(usize i = 0; i < (sizeof(sizes) / sizeof(*sizes)); ++i) {
        if ((c & sizes[i].mask) == sizes[i].result) {
            return sizes[i].octets;
        }
    }
    return -1;
}

usize utf8CpSize(rune ch) {
    if (ch < 0x80)         return 1;
    else if (ch < 0x800)   return 2;
    else if (ch < 0x10000) return 3;
    return 4;
}

/* #include "ini.h" */

/* #include "strstream.h" */
/* #include "file.h" */
/* #include "tracelog.h" */

// == INI READER ========================================================================

static const iniopts_t default_opts = {0};

static initable_t *findTable(ini_t *ctx, strview_t name);
static inivalue_t *findValue(vec(inivalue_t) values, strview_t key);
static void addTable(ini_t *ctx, str_istream_t *in, const iniopts_t *options);
static void addValue(initable_t *table, str_istream_t *in, const iniopts_t *options);

void _iniParseInternal(ini_t *ini, const iniopts_t *options) {
    // add root table
    vecAppend(ini->tables, (initable_t){0});
    str_istream_t in = istrInitLen(ini->text.buf, ini->text.len);
        istrSkipWhitespace(&in);
    while (!istrIsFinished(in)) {
        switch(*in.cur) {
        case '[':
            addTable(ini, &in, options);
            break;
        case '#': case ';':
            istrIgnore(&in, '\n');
            break;
        default:
            addValue(&ini->tables[0], &in, options);
            break;
        }
        istrSkipWhitespace(&in);
    }
    
}

ini_t iniParse(const char *filename, const iniopts_t *options) {
    ini_t ini = { .text = fileReadWholeText(filename) };
    if (strIsEmpty(ini.text)) return ini;
    if (!options) options = &default_opts;
    _iniParseInternal(&ini, options);
    return ini;
}

ini_t iniParseString(const char *inistr, const iniopts_t *options) {
    ini_t ini = { .text = strFromStr(inistr) };
    if (!options) options = &default_opts;
    _iniParseInternal(&ini, options);
    return ini;
}

void iniFree(ini_t ctx) {
    strFree(ctx.text);
    for (uint32 i = 0; i < vecLen(ctx.tables); ++i) {
        vecFree(ctx.tables[i].values);
    }
    vecFree(ctx.tables);
}

initable_t *iniGetTable(ini_t *ctx, const char *name) {
    if (!name) {
        return &ctx->tables[0];
    }
    else {
        return findTable(ctx, strvInit(name));
    }
}

inivalue_t *iniGet(initable_t *ctx, const char *key) {
    return ctx ? findValue(ctx->values, strvInit(key)) : NULL;
}

vec(strview_t) iniAsArray(const inivalue_t *value, char delim) {
    if (!value) return NULL;
    if (!delim) delim = ' ';

    vec(strview_t) out = NULL;
    strview_t v = value->value;

    usize start = 0;
    for (usize i = 0; i < v.len; ++i) {
        if (v.buf[i] == delim) {
            strview_t arr_val = strvTrim(strvSub(v, start, i));
            if (!strvIsEmpty(arr_val)) vecAppend(out, arr_val);
            start = i + 1;
        }
    }
    strview_t last = strvTrim(strvSub(v, start, SIZE_MAX));
    if (!strvIsEmpty(last)) vecAppend(out, last);
    return out;
}

vec(strview_t) iniAsArrayU8(const inivalue_t *value, const char *delim) {
    if (!value || !delim) return NULL;

    rune cpdelim = utf8Decode(&delim);
    vec(strview_t) out = NULL;
    strview_t v = value->value;

    const char *start = v.buf;
    const char *buf = v.buf;
    const char *prevbuf = buf;

    for(rune cp = utf8Decode(&buf); 
        buf != (v.buf + v.len); 
        cp = utf8Decode(&buf)
    ) {
        if (cp == cpdelim) {
            usize start_pos = start - v.buf;
            usize end_pos = prevbuf - v.buf;
            strview_t arr_val = strvTrim(strvSub(v, start_pos, end_pos));
            if (!strvIsEmpty(arr_val)) vecAppend(out, arr_val);
            // buf has already gone to the next codepoint, skipping the delimiter
            start = buf;
        }
        prevbuf = buf;
    }

    strview_t last = strvTrim(strvSub(v, start - v.buf, SIZE_MAX));
    if (!strvIsEmpty(last)) vecAppend(out, last);
    return out;
}

uint64 iniAsUInt(const inivalue_t *value) {
    if (!value) return 0;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    uint64 val = 0;
    if (!istrGetu64(&in, &val)) val = 0;
    return val;
}

int64 iniAsInt(const inivalue_t *value) {
    if (!value) return 0;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    int64 val = 0;
    if (!istrGeti64(&in, &val)) val = 0;
    return val;
}

double iniAsNum(const inivalue_t *value) {
    if (!value) return 0.f;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    double val = 0;
    if (!istrGetdouble(&in, &val)) val = 0;
    return val;
}

bool iniAsBool(const inivalue_t *value) {
    if (!value) return false;
    return strvCompare(value->value, strvInit("true")) == 0;
}

// == INI WRITER ========================================================================

/* #include "strstream.h" */
/* #include "file.h" */

static const winiopts_t default_wopts = {0};

iniwriter_t winiInit() {
    iniwriter_t out = {0};
    vecAppend(out.tables, (winitable_t){0});
    return out;
}

void winiFree(iniwriter_t ctx) {
    for (winitable_t *tab = ctx.tables; tab != vecEnd(ctx.tables); ++tab) {
        strFree(tab->key);
        for (winivalue_t *val = tab->values; val != vecEnd(tab->values); ++val) {
            strFree(val->key);
            strFree(val->value);
        }
        vecFree(tab->values);
    }
    vecFree(ctx.tables);
}

str_t winiToString(iniwriter_t ctx, const winiopts_t *options) {
    if (!options) options = &default_wopts;

    str_ostream_t out = ostrInitLen(1024 * 20);
    if (!options->no_discalimer) ostrPuts(&out, "# auto-generated by colla's ini.h, do not modify!\n");
    // add root values
    winitable_t *root = &ctx.tables[0];
    for (winivalue_t *val = root->values; val != vecEnd(root->values); ++val) {
        ostrPrintf(&out, "%s = %s\n", val->key.buf, val->value.buf);
    }
    if (root->values) ostrPuts(&out, "\n");
    // add each table
    for (usize i = 1; i < vecLen(ctx.tables); ++i) {
        winitable_t *tab = &ctx.tables[i];
        ostrPrintf(&out, "[%s]\n", tab->key.buf);
        for (winivalue_t *val = tab->values; val != vecEnd(tab->values); ++val) {
            ostrPrintf(&out, "%s = %s\n", val->key.buf, val->value.buf);
        }
        if ((i + 1) < vecLen(ctx.tables)) ostrPuts(&out, "\n");
    }
    return ostrAsStr(out);
}

void winiToFile(iniwriter_t ctx, const char *filename, const winiopts_t *options) {
    if (!options) options = &default_wopts;

    file_t fp = fileOpen(filename, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't write ini to file %s", filename);
        return;
    }
    str_t string = winiToString(ctx, options);
    fileWriteWholeTextFP(fp, strvInitStr(string));
    strFree(string);
    fileClose(fp);
}

winivalue_t *winiAddValEmpty(winitable_t *table) {
    if (!table) return NULL;
    vecAppend(table->values, (winivalue_t){0});
    return &vecBack(table->values);
}

winivalue_t *winiAddVal(winitable_t *table, const char *key, const char *value) {
    if (!table) return NULL;
    winivalue_t val = { .key = strFromStr(key), .value = strFromStr(value) };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winivalue_t *winiAddValStr(winitable_t *table, str_t key, str_t value) {
    if (!table) return NULL;
    winivalue_t val = { .key = key, .value = value };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winivalue_t *winiAddValView(winitable_t *table, strview_t key, strview_t value) {
    if (!table) return NULL;
    winivalue_t val = { .key = strFromView(key), .value = strFromView(value) };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winitable_t *winiAddTablEmpty(iniwriter_t *ctx) {
    vecAppend(ctx->tables, (winitable_t){0});
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTab(iniwriter_t *ctx, const char *name) {
    winitable_t tab = { .key = strFromStr(name) };
    vecAppend(ctx->tables, tab);
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTabStr(iniwriter_t *ctx, str_t name) {
    winitable_t tab = { .key = name };
    vecAppend(ctx->tables, tab);    
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTabView(iniwriter_t *ctx, strview_t name) {
    winitable_t tab = { .key = strFromView(name) };
    vecAppend(ctx->tables, tab);
    return &vecBack(ctx->tables);
}

// == PRIVATE FUNCTIONS ========================================================

static initable_t *findTable(ini_t *ctx, strview_t name) {
    if (strvIsEmpty(name)) return NULL;
    for (uint32 i = 1; i < vecLen(ctx->tables); ++i) {
        if (strvCompare(ctx->tables[i].name, name) == 0) {
            return &ctx->tables[i];
        }
    }
    return NULL;
}

static inivalue_t *findValue(vec(inivalue_t) values, strview_t key) {
    if (strvIsEmpty(key)) return NULL;
    for (uint32 i = 0; i < vecLen(values); ++i) {
        if (strvCompare(values[i].key, key) == 0) {
            return &values[i];
        }
    }
    return NULL;
}

static void addTable(ini_t *ctx, str_istream_t *in, const iniopts_t *options) {
    istrSkip(in, 1); // skip [
    strview_t name = istrGetview(in, ']');
    istrSkip(in, 1); // skip ]
    initable_t *table = options->merge_duplicate_tables ? findTable(ctx, name) : NULL;
    if (!table) {
        vecAppend(ctx->tables, (initable_t){ name });
        table = &vecBack(ctx->tables);
    }
    istrIgnore(in, '\n'); istrSkip(in, 1);
    while (!istrIsFinished(*in)) {
        switch (*in->cur) {
        case '\n': case '\r':
            return;
        case '#': case ';':
            istrIgnore(in, '\n');
            break;
        default:
            addValue(table, in, options);
            break;
        }
    }
}

static void addValue(initable_t *table, str_istream_t *in, const iniopts_t *options) {
    if (!table) fatal("table is null");
    
    strview_t key = strvTrim(istrGetview(in, '='));
    istrSkip(in, 1);
    strview_t value = strvTrim(istrGetview(in, '\n'));
    // value might be until EOF, in that case no use in skipping
    if (!istrIsFinished(*in)) istrSkip(in, 1); // skip newline
    inivalue_t *new_value = options->merge_duplicate_keys ? findValue(table->values, key) : NULL;
    if (!new_value) {
        inivalue_t ini_val = (inivalue_t){ key, value };
        vecAppend(table->values, ini_val);
    }
    else {
        new_value->value = value;
    }
}

/* #include "file.h" */

/* #include "tracelog.h" */

#ifdef _WIN32
/* #include "win32_slim.h" */
#include <stdlib.h>

static DWORD _toWin32Access(int mode) {
    if(mode & FILE_READ) return GENERIC_READ;
    if(mode & FILE_WRITE) return GENERIC_WRITE;
    if(mode & FILE_BOTH) return GENERIC_READ | GENERIC_WRITE;
    fatal("unrecognized access mode: %d", mode);
    return 0;
}

static DWORD _toWin32Creation(filemode_t mode) {
    if(mode & FILE_READ) return OPEN_EXISTING;
    if(mode == (FILE_WRITE | FILE_CLEAR)) return CREATE_ALWAYS;
    if(mode & FILE_WRITE) return OPEN_ALWAYS;
    if(mode & FILE_BOTH) return OPEN_ALWAYS;
    fatal("unrecognized creation mode: %d", mode);
    return 0;
}

bool fileExists(const char *fname) {
    DWORD dwAttrib = GetFileAttributesA(fname);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
           !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

file_t fileOpen(const char *fname, filemode_t mode) {
    return (file_t)CreateFileA(
        fname, 
        _toWin32Access(mode), 
        0, 
        NULL, 
        _toWin32Creation(mode), 
        FILE_ATTRIBUTE_NORMAL, 
        NULL
    );
}

void fileClose(file_t ctx) {
    if (ctx) {
        CloseHandle((HANDLE)ctx);
    }
}

bool fileIsValid(file_t ctx) {
    return (HANDLE)ctx != INVALID_HANDLE_VALUE;
}

bool filePutc(file_t ctx, char c) {
    return fileWrite(ctx, &c, 1) == 1;
}

bool filePuts(file_t ctx, const char *str) {
    usize len = strlen(str);
    return fileWrite(ctx, str, len) == len;
}

bool filePutstr(file_t ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

usize fileRead(file_t ctx, void *buf, usize len) {
    DWORD bytes_read = 0;
    BOOL result = ReadFile((HANDLE)ctx, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (usize)bytes_read : 0;
}

usize fileWrite(file_t ctx, const void *buf, usize len) {
    DWORD bytes_read = 0;
    BOOL result = WriteFile((HANDLE)ctx, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (usize)bytes_read : 0;
}

bool fileSeekEnd(file_t ctx) {
    return SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, NULL, FILE_END) == TRUE;
}

void fileRewind(file_t ctx) {
    SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, NULL, FILE_BEGIN);
}

uint64 fileTell(file_t ctx) {
    LARGE_INTEGER tell;
    BOOL result = SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, &tell, FILE_CURRENT);
    return result == TRUE ? (uint64)tell.QuadPart : 0;
}

#else
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

const char *_toStdioMode(filemode_t mode) {
    switch(mode) {
        case FILE_READ: return "rb";
        case FILE_BOTH: return "r+b";
        case FILE_WRITE: return "wb";
        default: fatal("mode not recognized: %d", mode); return "";
    }
}

bool fileExists(const char *fname) {
    return access(fname, F_OK) == 0;
}

file_t fileOpen(const char *fname, filemode_t mode) {
    return (file_t)(void*) fopen(fname, _toStdioMode(mode));
}

void fileClose(file_t ctx) {
    if(ctx) {
        fclose((FILE*)ctx);
    }
}

bool fileIsValid(file_t ctx) {
    return (FILE *)ctx != NULL;
}

bool filePutc(file_t ctx, char c) {
    return fputc(c, (FILE*)ctx) == c;
}

bool filePuts(file_t ctx, const char *str) {
    return fputs(str, (FILE*)ctx) != EOF;
}

bool filePutstr(file_t ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

usize fileRead(file_t ctx, void *buf, usize len) {
    return fread(buf, 1, len, (FILE*)ctx);
}

usize fileWrite(file_t ctx, const void *buf, usize len) {
    return fwrite(buf, 1, len, (FILE*)ctx);
}

bool fileSeekEnd(file_t ctx) {
    return fseek((FILE*)ctx, 0, SEEK_END) == 0;
}

void fileRewind(file_t ctx) {
    rewind((FILE*)ctx);
}

uint64 fileTell(file_t ctx) {
    return (uint64)ftell((FILE*)ctx);
}
#endif

static str_t _readWholeInternalStr(file_t ctx) {
    str_t contents = strInit();
    uint64 fsize = 0;
    usize read = 0;

    if(!fileSeekEnd(ctx)) {
        err("file: couldn't read until end");
        goto failed;
    }

    fsize = fileTell(ctx);
    fileRewind(ctx);

    contents.buf = (char *)malloc(fsize + 1);
    contents.len = fsize;
    if(!contents.buf) {
        err("file: couldn't allocate buffer");
        goto failed;
    }
    
    read = fileRead(ctx, contents.buf, fsize);
    if(read != fsize) {
        err("file: read wrong amount of bytes: %zu instead of %zu", read, fsize);
        goto failed_free;
    }

    contents.buf[contents.len] = '\0';

failed:
    return contents;
failed_free:
    strFree(contents);
    return strInit();   
}

static vec(uint8) _readWholeInternalVec(file_t ctx) {
    vec(uint8) contents = NULL;
    uint64 fsize = 0;
    usize read = 0;

    if(!fileSeekEnd(ctx)) {
        err("file: couldn't read until end");
        goto failed;
    }

    fsize = fileTell(ctx);
    fileRewind(ctx);

    vecReserve(contents, fsize);
    if(!contents) {
        err("file: couldn't allocate buffer");
        goto failed;
    }

    read = fileRead(ctx, contents, fsize);
    if(read != fsize) {
        err("file: read wrong amount of bytes: %zu instead of %zu", read, fsize);
        goto failed_free;
    }

    _veclen(contents) = read;

failed:
    return contents;
failed_free:
    vecFree(contents);
    return contents;    
}

vec(uint8) fileReadWhole(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    if (!fileIsValid(fp)) return NULL;
    vec(uint8) contents = fileReadWholeFP(fp);
    fileClose(fp);
    return contents;
}

vec(uint8) fileReadWholeFP(file_t ctx) {
    return _readWholeInternalVec(ctx);
}

str_t fileReadWholeText(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    if(!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return strInit();
    }
    str_t contents = fileReadWholeTextFP(fp);
    fileClose(fp);
    return contents;
}

str_t fileReadWholeTextFP(file_t ctx) {
    return _readWholeInternalStr(ctx);
}

bool fileWriteWhole(const char *fname, filebuf_t data) {
    file_t fp = fileOpen(fname, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return false;
    }
    bool res = fileWriteWholeFP(fp, data);
    fileClose(fp);
    return res;
}

bool fileWriteWholeFP(file_t ctx, filebuf_t data) {
    usize written = fileWrite(ctx, data.buf, data.len);
    return written == data.len;
}

bool fileWriteWholeText(const char *fname, strview_t string) {
    file_t fp = fileOpen(fname, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return false;
    }
    bool res = fileWriteWholeTextFP(fp, string);
    fileClose(fp);
    return res;
}

bool fileWriteWholeTextFP(file_t ctx, strview_t string) {
    return fileWriteWholeFP(ctx, (filebuf_t){ (uint8 *)string.buf, string.len });
}

/* #include "dir.h" */
/* #include "tracelog.h" */

#ifdef _WIN32
/* #include "win32_slim.h" */
#include <stdlib.h>
#include <assert.h>

/* #include "strstream.h" */

typedef struct {
    dir_entry_t cur;
    dir_entry_t next;
    HANDLE handle;
} _dir_internal_t;

static dir_entry_t _fillDirEntry(WIN32_FIND_DATAW *data) {
    return (dir_entry_t) {
        .type = 
            data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 
            FS_TYPE_DIR : FS_TYPE_FILE,
        .name = strFromWCHAR(data->cFileName, 0)
    };
}

static _dir_internal_t _getFirst(const wchar_t *path) {
    _dir_internal_t res = {0};
    WIN32_FIND_DATAW data = {0};

    res.handle = FindFirstFileW(path, &data);

    if(res.handle != INVALID_HANDLE_VALUE) {
        res.next = _fillDirEntry(&data);
    }

    return res;
}

static void _getNext(_dir_internal_t *ctx) {
    WIN32_FIND_DATAW data = {0};

    BOOL result = FindNextFileW(ctx->handle, &data);
    if(!result) {
        if(GetLastError() == ERROR_NO_MORE_FILES) {
            FindClose(ctx->handle);
            ctx->handle = NULL;
            return;
        }
    }
    ctx->next = _fillDirEntry(&data);
}

dir_t dirOpen(const char *path) {
    DWORD n = GetFullPathName(path, 0, NULL, NULL);
    str_ostream_t out = ostrInitLen(n + 3);
    n = GetFullPathName(path, n, out.buf, NULL);
    assert(n > 0);
    out.len += n;
    switch(ostrBack(out)) {
    case '\\':
    case '/':
    case ':':
        // directory ends in path separator
        // NOP
        break;
    default:
        ostrPutc(&out, '\\');
    }
    ostrPutc(&out, '*');

    _dir_internal_t *dir = malloc(sizeof(_dir_internal_t));
    if(dir) {
        wchar_t *wpath = strToWCHAR(ostrAsStr(out));
        assert(wpath);
        *dir = _getFirst(wpath);
        free(wpath);
    }
    ostrFree(out);

    return dir;
}

void dirClose(dir_t ctx) {
    free(ctx);
}

bool dirValid(dir_t ctx) {
    _dir_internal_t *dir = (_dir_internal_t*)ctx;
    return dir->handle != INVALID_HANDLE_VALUE;
}

dir_entry_t *dirNext(dir_t ctx) {
    _dir_internal_t *dir = (_dir_internal_t*)ctx;
    strFree(dir->cur.name);
    if(!dir->handle) return NULL;
    dir->cur = dir->next;
    _getNext(dir);
    return &dir->cur;
}

void dirCreate(const char *path) {
    CreateDirectoryA(path, NULL);
}

#else

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

// taken from https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html
// hopefully shouldn't be needed
#ifndef DT_DIR
    #define DT_DIR 4
#endif
#ifndef DT_REG
    #define DT_REG 8
#endif

typedef struct {
    DIR *dir;
    dir_entry_t next;
} _dir_internal_t;

dir_t dirOpen(const char *path) {
    _dir_internal_t *ctx = (_dir_internal_t *)calloc(1, sizeof(_dir_internal_t));
    if(ctx) ctx->dir = opendir(path);
    return ctx;
}

void dirClose(dir_t ctx) {
    if(ctx) {
        _dir_internal_t *in = (_dir_internal_t *)ctx;
        closedir(in->dir);
        free(in);
    }
}

bool dirValid(dir_t ctx) {
    _dir_internal_t *dir = (_dir_internal_t*)ctx;
    return dir->dir != NULL;
}

dir_entry_t *dirNext(dir_t ctx) {
    if(!ctx) return NULL;
    _dir_internal_t *in = (_dir_internal_t *)ctx;
    strFree(in->next.name);
    struct dirent *dp = readdir(in->dir);
    if(!dp) return NULL;
    
    switch(dp->d_type) {
    case DT_DIR: in->next.type = FS_TYPE_DIR; break;
    case DT_REG: in->next.type = FS_TYPE_FILE; break;
    default: in->next.type = FS_TYPE_UNKNOWN; break;
    }

    in->next.name = strFromStr(dp->d_name);
    return &in->next;
}

void dirCreate(const char *path) {
    mkdir(path, 0700);
}

#endif

#ifndef COLLA_NO_NET
/* #include "socket.h" */

#include <stdio.h>
/* #include "tracelog.h" */

#ifndef NDEBUG
// VERY MUCH NOT THREAD SAFE
static int initialize_count = 0;
#endif

#if SOCK_WINDOWS
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

socket_t skOpen(sktype_t type) {
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

bool skBindPro(socket_t sock, const sk_addr_t *name, sk_len_t namelen) {
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
    sk_len_t addr_size = (sk_len_t)sizeof(addr);
    return skAcceptPro(sock, (sk_addr_t *) &addr, &addr_size);
}

socket_t skAcceptPro(socket_t sock, sk_addr_t *addr, sk_len_t *addrlen) {
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

bool skConnectPro(socket_t sock, const sk_addr_t *name, sk_len_t namelen) {
    return connect(sock, name, namelen) != SOCKET_ERROR;
}

int skSend(socket_t sock, const void *buf, int len) {
    return skSendPro(sock, buf, len, 0);
}

int skSendPro(socket_t sock, const void *buf, int len, int flags) {
    return send(sock, buf, len, flags);
}

int skSendTo(socket_t sock, const void *buf, int len, const sk_addrin_t *to) {
    return skSendToPro(sock, buf, len, 0, (sk_addr_t*) to, sizeof(sk_addrin_t));
}

int skSendToPro(socket_t sock, const void *buf, int len, int flags, const sk_addr_t *to, int tolen) {
    return sendto(sock, buf, len, flags, to, tolen);
}

int skReceive(socket_t sock, void *buf, int len) {
    return skReceivePro(sock, buf, len, 0);
}

int skReceivePro(socket_t sock, void *buf, int len, int flags) {
    return recv(sock, buf, len, flags);
}

int skReceiveFrom(socket_t sock, void *buf, int len, sk_addrin_t *from) {
    sk_len_t fromlen = sizeof(sk_addr_t);
    return skReceiveFromPro(sock, buf, len, 0, (sk_addr_t*)from, &fromlen);
}

int skReceiveFromPro(socket_t sock, void *buf, int len, int flags, sk_addr_t *from, sk_len_t *fromlen) {
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

/* #include "http.h" */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* #include "os.h" */
/* #include "tracelog.h" */

/* #include "vec.h" */

// == INTERNAL ================================================================

static void _setField(vec(http_field_t) *fields_vec, const char *key, const char *value) {
    vec(http_field_t) fields = *fields_vec;

    for (uint32 i = 0; i < vecLen(fields); ++i) {
        if (stricmp(fields[i].key, key) == 0) {
            char **curval = &fields[i].value;
            usize curlen = strlen(*curval);
            usize newlen = strlen(value);
            if(newlen > curlen) {
                *curval = (char *)realloc(*curval, newlen + 1);
            }
            memcpy(*curval, value, newlen);
            (*curval)[newlen] = '\0';
            return;
        }
    }

    // otherwise, add it to the list
    http_field_t field;
    usize klen = strlen(key);
    usize vlen = strlen(value);
    field.key = (char *)malloc(klen + 1);
    field.value = (char *)malloc(vlen + 1);
    memcpy(field.key, key, klen);
    memcpy(field.value, value, vlen);
    field.key[klen] = field.value[vlen] = '\0';

    vecAppend(*fields_vec, field);
}

// == HTTP VERSION ============================================================

int httpVerNumber(http_version_t ver) {
    return (ver.major * 10) + ver.minor;
}

// == HTTP REQUEST ============================================================

http_request_t reqInit() {
    http_request_t req;
    memset(&req, 0, sizeof(req));
    reqSetUri(&req, strvInit("/"));
    req.version = (http_version_t){1, 1};
    return req;
}

void reqFree(http_request_t *ctx) {
    for (uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        free(ctx->fields[i].key);
        free(ctx->fields[i].value);
    }
    vecFree(ctx->fields);
    free(ctx->uri);
    free(ctx->body);
    memset(ctx, 0, sizeof(http_request_t));
}

bool reqHasField(http_request_t *ctx, const char *key) {
    for(uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        if(stricmp(ctx->fields[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

void reqSetField(http_request_t *ctx, const char *key, const char *value) {
    _setField(&ctx->fields, key, value);
}

void reqSetUri(http_request_t *ctx, strview_t uri) {
    if (strvIsEmpty(uri)) return;
    free(ctx->uri);
    if (uri.buf[0] != '/') {
        ctx->uri = (char *)realloc(ctx->uri, uri.len + 1);
        ctx->uri[0] = '/';
        memcpy(ctx->uri + 1, uri.buf, uri.len);
        ctx->uri[uri.len] = '\0';
    }
    else {
        ctx->uri = strvCopy(uri).buf;
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

    for(uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        ostrPrintf(&out, "%s: %s\r\n", ctx->fields[i].key, ctx->fields[i].value);
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
    return ostrAsStr(out);
}

// == HTTP RESPONSE ===========================================================

http_response_t resInit() {
    return (http_response_t) {0};
}   

void resFree(http_response_t *ctx) {
    for(uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        free(ctx->fields[i].key);
        free(ctx->fields[i].value);
    }
    vecFree(ctx->fields);
    vecFree(ctx->body);
    memset(ctx, 0, sizeof(http_response_t));
}

bool resHasField(http_response_t *ctx, const char *key) {
    for(uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        if(stricmp(ctx->fields[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

const char *resGetField(http_response_t *ctx, const char *field) {
    for(uint32 i = 0; i < vecLen(ctx->fields); ++i) {
        if(stricmp(ctx->fields[i].key, field) == 0) {
            return ctx->fields[i].value;
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
    istrGeti32(&in, (int32*)&ctx->status_code);

    istrIgnore(&in, '\n');
    istrSkip(&in, 1); // skip \n

    resParseFields(ctx, &in);

    const char *tran_encoding = resGetField(ctx, "transfer-encoding");
    if(tran_encoding == NULL || stricmp(tran_encoding, "chunked")  != 0) {
        strview_t body = istrGetviewLen(&in, 0, SIZE_MAX);
        vecClear(ctx->body);
        vecReserve(ctx->body, body.len);
        memcpy(ctx->body, body.buf, body.len);
    }
    else {
        fatal("chunked encoding not implemented yet");
    }
}

void resParseFields(http_response_t *ctx, str_istream_t *in) {
    strview_t line;

    do {
        line = istrGetview(in, '\r');

        usize pos = strvFind(line, ':', 0);
        if(pos != STRV_NOT_FOUND) {
            strview_t key = strvSub(line, 0, pos);
            strview_t value = strvSub(line, pos + 2, SIZE_MAX);

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
    strFree(ctx->host_name);
    memset(ctx, 0, sizeof(http_client_t));
}

void hcliSetHost(http_client_t *ctx, strview_t hostname) {
    // if the hostname starts with http:// (case insensitive)
    if(strvICompare(strvSub(hostname, 0, 7), strvInit("http://")) == 0) {
        ctx->host_name = strvCopy(strvSub(hostname, 7, SIZE_MAX));
    }
    else if(strvICompare(strvSub(hostname, 0, 8), strvInit("https://")) == 0) {
        err("HTTPS protocol not yet supported");
        return;
    }
    else {
        // undefined protocol, use HTTP
        ctx->host_name = strvCopy(hostname);
    }
}

http_response_t hcliSendRequest(http_client_t *ctx, http_request_t *req) {
    if (strBack(ctx->host_name) == '/') {
        ctx->host_name.buf[--ctx->host_name.len] = '\0';
    }
    if(!reqHasField(req, "Host")) {
        reqSetField(req, "Host", ctx->host_name.buf);
    }
    if(!reqHasField(req, "Content-Length")) {
        if(req->body) {
            str_ostream_t out = ostrInitLen(20);
            ostrAppendu64(&out, strlen(req->body));
            reqSetField(req, "Content-Length", out.buf);
            ostrFree(out);
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
        if(*(received.buf + received.len) != '\0') {
            ostrPutc(&received, '\0');
            received.len--;
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
    strFree(req_str);
    ostrFree(received);
    return res;
}

http_response_t httpGet(strview_t hostname, strview_t uri) {
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

url_split_t urlSplit(strview_t uri) {
    url_split_t out = {0};

    if (strvStartsWithView(uri, strvInit("https://"))) {
        uri = strvRemovePrefix(uri, 8);
    }
    else if (strvStartsWithView(uri, strvInit("http://"))) {
        uri = strvRemovePrefix(uri, 7);
    }

    out.host = strvSub(uri, 0, strvFind(uri, '/', 0));
    out.uri = strvSub(uri, out.host.len, SIZE_MAX);
    return out;
}

#endif // COLLA_NO_NET
#if !defined(__TINYC__) && !defined(COLLA_NO_THREADS)
/* #include "cthreads.h" */

typedef struct {
    cthread_func_t func;
    void *arg;
} _thr_internal_t;

#ifdef _WIN32
/* #include "win32_slim.h" */
#include <stdlib.h>

// == THREAD ===========================================

static DWORD _thrFuncInternal(void *arg) {
    _thr_internal_t *params = (_thr_internal_t *)arg;
    cthread_func_t func = params->func;
    void *argument = params->arg;
    free(params);
    return (DWORD)func(argument);
}

cthread_t thrCreate(cthread_func_t func, void *arg) {
    HANDLE thread = INVALID_HANDLE_VALUE;
    _thr_internal_t *params = malloc(sizeof(_thr_internal_t));
    
    if(params) {
        params->func = func;
        params->arg = arg;

        thread = CreateThread(NULL, 0, _thrFuncInternal, params, 0, NULL);
    }

    return (cthread_t)thread;
}

bool thrValid(cthread_t ctx) {
    return (HANDLE)ctx != INVALID_HANDLE_VALUE;
}

bool thrDetach(cthread_t ctx) {
    return CloseHandle((HANDLE)ctx);
}

cthread_t thrCurrent(void) {
    return (cthread_t)GetCurrentThread();
}

int thrCurrentId(void) {
    return GetCurrentThreadId();
}

int thrGetId(cthread_t ctx) {
    return GetThreadId((HANDLE)ctx);
}

void thrExit(int code) {
    ExitThread(code);
}

bool thrJoin(cthread_t ctx, int *code) {
    if(!ctx) return false;
    int return_code = WaitForSingleObject((HANDLE)ctx, INFINITE);
    if(code) *code = return_code;
    BOOL success = CloseHandle((HANDLE)ctx);
    return return_code != WAIT_FAILED && success;
}

// == MUTEX ============================================

cmutex_t mtxInit(void) {
    CRITICAL_SECTION *crit_sec = malloc(sizeof(CRITICAL_SECTION));
    if(crit_sec) {
        InitializeCriticalSection(crit_sec);
    }
    return (cmutex_t)crit_sec;
}

void mtxDestroy(cmutex_t ctx) {
    DeleteCriticalSection((CRITICAL_SECTION *)ctx);
}

bool mtxValid(cmutex_t ctx) {
    return (void *)ctx != NULL;
}

bool mtxLock(cmutex_t ctx) {
    EnterCriticalSection((CRITICAL_SECTION *)ctx);
    return true;
}

bool mtxTryLock(cmutex_t ctx) {
    return TryEnterCriticalSection((CRITICAL_SECTION *)ctx);
}

bool mtxUnlock(cmutex_t ctx) {
    LeaveCriticalSection((CRITICAL_SECTION *)ctx);
    return true;
}


#else
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

// == THREAD ===========================================

#define INT_TO_VOIDP(a) ((void *)((uintptr_t)(a)))

static void *_thrFuncInternal(void *arg) {
    _thr_internal_t *params = (_thr_internal_t *)arg;
    cthread_func_t func = params->func;
    void *argument = params->arg;
    free(params);
    return INT_TO_VOIDP(func(argument));
}

cthread_t thrCreate(cthread_func_t func, void *arg) {
    pthread_t handle = (pthread_t)NULL;

    _thr_internal_t *params = malloc(sizeof(_thr_internal_t));
    
    if(params) {
        params->func = func;
        params->arg = arg;

        int result = pthread_create(&handle, NULL, _thrFuncInternal, params);
        if(result) handle = (pthread_t)NULL;
    }

    return (cthread_t)handle;
}

bool thrValid(cthread_t ctx) {
    return (void *)ctx != NULL;
}

bool thrDetach(cthread_t ctx) {
    return pthread_detach((pthread_t)ctx) == 0;
}

cthread_t thrCurrent(void) {
    return (cthread_t)pthread_self();
}

int thrCurrentId(void) {
    return (int)pthread_self();
}

int thrGetId(cthread_t ctx) {
    return (int)ctx;
}

void thrExit(int code) {
    pthread_exit(INT_TO_VOIDP(code));
}

bool thrJoin(cthread_t ctx, int *code) {
    void *result = code;
    return pthread_join((pthread_t)ctx, &result) != 0;
}

// == MUTEX ============================================

cmutex_t mtxInit(void) {
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));

    if(mutex) {
        int res = pthread_mutex_init(mutex, NULL);
        if(res != 0) mutex = NULL;
    }

    return (cmutex_t)mutex;
}

void mtxDestroy(cmutex_t ctx) {
    pthread_mutex_destroy((pthread_mutex_t *)ctx);
}

bool mtxValid(cmutex_t ctx) {
    return (void *)ctx != NULL;
}

bool mtxLock(cmutex_t ctx) {
    return pthread_mutex_lock((pthread_mutex_t *)ctx) == 0;
}

bool mtxTryLock(cmutex_t ctx) {
    return pthread_mutex_trylock((pthread_mutex_t *)ctx) == 0;
}

bool mtxUnlock(cmutex_t ctx) {
    return pthread_mutex_unlock((pthread_mutex_t *)ctx) == 0;
}

#endif

#endif // !defined(__TINYC__) && !defined(COLLA_NO_THREADS)
#endif /* COLLA_IMPL */
/*
    MIT License
    Copyright (c) 1994-2019 Lua.org, PUC-Rio.
    Copyright (c) 2020-2022 snarmph.
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
