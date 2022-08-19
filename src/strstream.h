#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "collatypes.h"
#include "str.h"

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
