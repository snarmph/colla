#include "strstream.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h> // HUGE_VALF
#include "tracelog.h"

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
