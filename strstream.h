#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "str.h"

/* == INPUT STREAM ============================================ */

typedef struct {
    const char *start;
    const char *cur;
    size_t size;
} str_istream_t;

// initialize with null-terminated string
str_istream_t istrInit(const char *str);
str_istream_t istrInitLen(const char *str, size_t len);

// get the current character and advance
char istrGet(str_istream_t *ctx);
// get the current character but don't advance
char istrPeek(str_istream_t *ctx);
// ignore characters until the delimiter
void istrIgnore(str_istream_t *ctx, char delim);
// skip n characters
void istrSkip(str_istream_t *ctx, size_t n);
// read len bytes into buffer, the buffer will not be null terminated
void istrRead(str_istream_t *ctx, char *buf, size_t len);
// read a maximum of len bytes into buffer, the buffer will not be null terminated
// returns the number of bytes read
size_t istrReadMax(str_istream_t *ctx, char *buf, size_t len);
// return to the beginning of the stream
void istrRewind(str_istream_t *ctx);
// return the number of bytes read from beginning of stream
size_t istrTell(str_istream_t *ctx);
// return true if the stream doesn't have any new bytes to read
bool istrIsFinished(str_istream_t *ctx);

bool istrGetbool(str_istream_t *ctx, bool *val);
bool istrGetu8(str_istream_t *ctx, uint8_t *val);
bool istrGetu16(str_istream_t *ctx, uint16_t *val);
bool istrGetu32(str_istream_t *ctx, uint32_t *val);
bool istrGetu64(str_istream_t *ctx, uint64_t *val);
bool istrGeti8(str_istream_t *ctx, int8_t *val);
bool istrGeti16(str_istream_t *ctx, int16_t *val);
bool istrGeti32(str_istream_t *ctx, int32_t *val);
bool istrGeti64(str_istream_t *ctx, int64_t *val);
bool istrGetfloat(str_istream_t *ctx, float *val);
bool istrGetdouble(str_istream_t *ctx, double *val);
// get a string until a delimiter, the string is allocated by the function and should be freed
size_t istrGetstring(str_istream_t *ctx, char **val, char delim);
// get a string of maximum size len, the string is not allocated by the function and will be null terminated
size_t istrGetstringBuf(str_istream_t *ctx, char *val, size_t len);
strview_t istrGetview(str_istream_t *ctx, char delim);
strview_t istrGetviewLen(str_istream_t *ctx, size_t off, size_t len);

/* == OUTPUT STREAM =========================================== */

typedef struct {
    char *buf;
    size_t size;
    size_t allocated;
} str_ostream_t;

str_ostream_t ostrInit(void);
str_ostream_t ostrInitLen(size_t initial_alloc);
str_ostream_t ostrInitStr(const char *buf, size_t len);

void ostrFree(str_ostream_t *ctx);
void ostrClear(str_ostream_t *ctx);
str_t ostrMove(str_ostream_t *ctx);
    
char ostrBack(str_ostream_t *ctx);
str_t ostrAsStr(str_ostream_t *ctx);
strview_t ostrAsView(str_ostream_t *ctx);

void ostrReplace(str_ostream_t *ctx, char from, char to);

void ostrPrintf(str_ostream_t *ctx, const char *fmt, ...);
void ostrPutc(str_ostream_t *ctx, char c);

void ostrAppendbool(str_ostream_t *ctx, bool val);
void ostrAppendchar(str_ostream_t *ctx, char val);
void ostrAppendu8(str_ostream_t *ctx, uint8_t val);
void ostrAppendu16(str_ostream_t *ctx, uint16_t val);
void ostrAppendu32(str_ostream_t *ctx, uint32_t val);
void ostrAppendu64(str_ostream_t *ctx, uint64_t val);
void ostrAppendi8(str_ostream_t *ctx, int8_t val);
void ostrAppendi16(str_ostream_t *ctx, int16_t val);
void ostrAppendi32(str_ostream_t *ctx, int32_t val);
void ostrAppendi64(str_ostream_t *ctx, int64_t val);
void ostrAppendfloat(str_ostream_t *ctx, float val);
void ostrAppenddouble(str_ostream_t *ctx, double val);
void ostrAppendview(str_ostream_t *ctx, strview_t view);

#ifdef __cplusplus
} // extern "C"
#endif