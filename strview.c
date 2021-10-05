#include "strview.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "tracelog.h"
#include <stdio.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


strview_t strvInit(const char *cstr) {
    return strvInitLen(cstr, cstr ? strlen(cstr) : 0);
}

strview_t strvInitLen(const char *buf, size_t size) {
    strview_t view;
    view.buf = buf;
    view.len = size;
    return view;
}

char strvFront(strview_t ctx) {
    return ctx.buf[0];
}

char strvBack(strview_t ctx) {
    return ctx.buf[ctx.len - 1];
}

const char *strvBegin(strview_t *ctx) {
    return ctx->buf;
}

const char *strvEnd(strview_t *ctx) {
    return ctx->buf + ctx->len;
}

bool strvIsEmpty(strview_t ctx) {
    return ctx.len == 0;
}

void strvRemovePrefix(strview_t *ctx, size_t n) {
    ctx->buf += n;
    ctx->len -= n;
}

void strvRemoveSuffix(strview_t *ctx, size_t n) {
    ctx->len -= n;
}  

size_t strvCopy(strview_t ctx, char **buf) {
    *buf = malloc(ctx.len + 1);
    memcpy(*buf, ctx.buf, ctx.len);
    (*buf)[ctx.len] = '\0';
    return ctx.len;
}

size_t strvCopyN(strview_t ctx, char **buf, size_t count, size_t from) {
    size_t sz = ctx.len + 1 - from;
    count = min(count, sz);
    *buf = malloc(count + 1);
    memcpy(*buf, ctx.buf + from, count);
    (*buf)[count] = '\0';
    return count;
}

size_t strvCopyBuf(strview_t ctx, char *buf, size_t len, size_t from) {
    size_t sz = ctx.len + 1 - from;
    len = min(len, sz);
    memcpy(buf, ctx.buf + from, len);
    buf[len - 1] = '\0';
    return len - 1;
}

strview_t strvSubstr(strview_t ctx, size_t from, size_t len) {
    if(from > ctx.len) from = ctx.len - len;
    size_t sz = ctx.len - from;
    return strvInitLen(ctx.buf + from, min(len, sz));
}

int strvCompare(strview_t ctx, strview_t other) {
    if(ctx.len < other.len) return -1;
    if(ctx.len > other.len) return  1;
    return memcmp(ctx.buf, other.buf, ctx.len);
}

int strvICompare(strview_t ctx, strview_t other) {
    if(ctx.len < other.len) return -1;
    if(ctx.len > other.len) return  1;
    for(size_t i = 0; i < ctx.len; ++i) {
        char a = tolower(ctx.buf[i]);
        char b = tolower(other.buf[i]);
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
    for(size_t i = 0; i < ctx.len; ++i) {
        if(ctx.buf[i] == c) return true;
    }
    return false;
}

bool strvContainsView(strview_t ctx, strview_t view) {
    if(ctx.len < view.len) return false;
    size_t end = ctx.len - view.len;
    for(size_t i = 0; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.len) == 0) return true;
    }
    return false;
}

size_t strvFind(strview_t ctx, char c, size_t from) {
    for(size_t i = from; i < ctx.len; ++i) {
        if(ctx.buf[i] == c) return i;
    }
    return SIZE_MAX;
}

size_t strvFindView(strview_t ctx, strview_t view, size_t from) {
    if(ctx.len < view.len) return SIZE_MAX;
    size_t end = ctx.len - view.len;
    for(size_t i = from; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.len) == 0) return i;
    }
    return SIZE_MAX;
}

size_t strvRFind(strview_t ctx, char c, size_t from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(*buf == c) return (buf - ctx.buf);
    }

    return SIZE_MAX;
}

size_t strvRFindView(strview_t ctx, strview_t view, size_t from) {
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

size_t strvFindFirstOf(strview_t ctx, strview_t view, size_t from) {
    if(ctx.len < view.len) return SIZE_MAX;
    for(size_t i = from; i < ctx.len; ++i) {
        for(size_t j = 0; j < view.len; ++j) {
            if(ctx.buf[i] == view.buf[j]) return i;
        }
    }
    return SIZE_MAX;
}

size_t strvFindLastOf(strview_t ctx, strview_t view, size_t from) {
    if(from >= ctx.len) {
        from = ctx.len - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        for(size_t j = 0; j < view.len; ++j) {
            if(*buf == view.buf[j]) return (buf - ctx.buf);
        }
    }

    return SIZE_MAX;
}

size_t strvFindFirstNot(strview_t ctx, char c, size_t from) {
    size_t end = ctx.len - 1;
    for(size_t i = from; i < end; ++i) {
        if(ctx.buf[i] != c) return i;
    }
    return SIZE_MAX;
}

size_t strvFindFirstNotOf(strview_t ctx, strview_t view, size_t from) {
    for(size_t i = from; i < ctx.len; ++i) {
        if(!strvContains(view, ctx.buf[i])) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t strvFindLastNot(strview_t ctx, char c, size_t from) {
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

size_t strvFindLastNotOf(strview_t ctx, strview_t view, size_t from) {
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
