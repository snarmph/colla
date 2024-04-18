#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#include "collatypes.h"
#include "str.h"

typedef struct arena_t arena_t;

/* == INPUT STREAM ============================================ */

typedef struct {
    const char *start;
    const char *cur;
    usize size;
} instream_t;

// initialize with null-terminated string
instream_t istrInit(const char *str);
instream_t istrInitLen(const char *str, usize len);

// get the current character and advance
char istrGet(instream_t *ctx);
// get the current character but don't advance
char istrPeek(instream_t *ctx);
// get the next character but don't advance
char istrPeekNext(instream_t *ctx);
// ignore characters until the delimiter
void istrIgnore(instream_t *ctx, char delim);
// ignore characters until the delimiter and skip it
void istrIgnoreAndSkip(instream_t *ctx, char delim);
// skip n characters
void istrSkip(instream_t *ctx, usize n);
// skips whitespace (' ', '\n', '\t', '\r')
void istrSkipWhitespace(instream_t *ctx);
// read len bytes into buffer, the buffer will not be null terminated
void istrRead(instream_t *ctx, char *buf, usize len);
// read a maximum of len bytes into buffer, the buffer will not be null terminated
// returns the number of bytes read
usize istrReadMax(instream_t *ctx, char *buf, usize len);
// returns to the beginning of the stream
void istrRewind(instream_t *ctx);
// returns back <amount> characters
void istrRewindN(instream_t *ctx, usize amount);
// returns the number of bytes read from beginning of stream
usize istrTell(instream_t ctx);
// returns the number of bytes left to read in the stream
usize istrRemaining(instream_t ctx); 
// return true if the stream doesn't have any new bytes to read
bool istrIsFinished(instream_t ctx);

bool istrGetBool(instream_t *ctx, bool *val);
bool istrGetU8(instream_t *ctx, uint8 *val);
bool istrGetU16(instream_t *ctx, uint16 *val);
bool istrGetU32(instream_t *ctx, uint32 *val);
bool istrGetU64(instream_t *ctx, uint64 *val);
bool istrGetI8(instream_t *ctx, int8 *val);
bool istrGetI16(instream_t *ctx, int16 *val);
bool istrGetI32(instream_t *ctx, int32 *val);
bool istrGetI64(instream_t *ctx, int64 *val);
bool istrGetFloat(instream_t *ctx, float *val);
bool istrGetDouble(instream_t *ctx, double *val);
str_t istrGetStr(arena_t *arena, instream_t *ctx, char delim);
// get a string of maximum size len, the string is not allocated by the function and will be null terminated
usize istrGetBuf(instream_t *ctx, char *buf, usize buflen);
strview_t istrGetView(instream_t *ctx, char delim);
strview_t istrGetViewLen(instream_t *ctx, usize len);

/* == OUTPUT STREAM =========================================== */

typedef struct {
    char *beg;
    arena_t *arena;
} outstream_t;

outstream_t ostrInit(arena_t *exclusive_arena);
void ostrClear(outstream_t *ctx);

usize ostrTell(outstream_t *ctx);

char ostrBack(outstream_t *ctx);
str_t ostrAsStr(outstream_t *ctx);
strview_t ostrAsView(outstream_t *ctx);

void ostrPrintf(outstream_t *ctx, const char *fmt, ...);
void ostrPrintfV(outstream_t *ctx, const char *fmt, va_list args);
void ostrPutc(outstream_t *ctx, char c);
void ostrPuts(outstream_t *ctx, strview_t v);

void ostrAppendBool(outstream_t *ctx, bool val);
void ostrAppendUInt(outstream_t *ctx, uint64 val);
void ostrAppendInt(outstream_t *ctx, int64 val);
void ostrAppendNum(outstream_t *ctx, double val);

#ifdef __cplusplus
} // extern "C"
#endif
