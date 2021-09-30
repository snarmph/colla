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
    return strvInitLen(cstr, strlen(cstr));
}

strview_t strvInitLen(const char *buf, size_t size) {
    strview_t view;
    view.buf = buf;
    view.size = size;
    return view;
}

char strvFront(strview_t ctx) {
    return ctx.buf[0];
}

char strvBack(strview_t ctx) {
    return ctx.buf[ctx.size - 1];
}

const char *strvBegin(strview_t *ctx) {
    return ctx->buf;
}

const char *strvEnd(strview_t *ctx) {
    return ctx->buf + ctx->size;
}

bool strvIsEmpty(strview_t ctx) {
    return ctx.size == 0;
}

void strvRemovePrefix(strview_t *ctx, size_t n) {
    ctx->buf += n;
    ctx->size -= n;
}

void strvRemoveSuffix(strview_t *ctx, size_t n) {
    ctx->size -= n;
}  

size_t strvCopy(strview_t ctx, char **buf) {
    *buf = malloc(ctx.size + 1);
    memcpy(*buf, ctx.buf, ctx.size);
    (*buf)[ctx.size] = '\0';
    return ctx.size;
}

size_t strvCopyN(strview_t ctx, char **buf, size_t count, size_t from) {
    size_t sz = ctx.size + 1 - from;
    count = min(count, sz);
    *buf = malloc(count + 1);
    memcpy(*buf, ctx.buf + from, count);
    (*buf)[count] = '\0';
    return count;
}

size_t strvCopyBuf(strview_t ctx, char *buf, size_t len, size_t from) {
    size_t sz = ctx.size + 1 - from;
    len = min(len, sz);
    memcpy(buf, ctx.buf + from, len);
    buf[len - 1] = '\0';
    return len - 1;
}

strview_t strvSubstr(strview_t ctx, size_t from, size_t len) {
    if(from > ctx.size) from = ctx.size - len;
    size_t sz = ctx.size - from;
    return strvInitLen(ctx.buf + from, min(len, sz));
}

int strvCompare(strview_t ctx, strview_t other) {
    if(ctx.size < other.size) return -1;
    if(ctx.size > other.size) return  1;
    return memcmp(ctx.buf, other.buf, ctx.size);
}

int strvICompare(strview_t ctx, strview_t other) {
    if(ctx.size < other.size) return -1;
    if(ctx.size > other.size) return  1;
    for(size_t i = 0; i < ctx.size; ++i) {
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
    if(ctx.size < view.size) return false;
    return memcmp(ctx.buf, view.buf, view.size) == 0;
}

bool strvEndsWith(strview_t ctx, char c) {
    return strvBack(ctx) == c;
}

bool strvEndsWithView(strview_t ctx, strview_t view) {
    if(ctx.size < view.size) return false;
    return memcmp(ctx.buf + ctx.size - view.size, view.buf, view.size) == 0;
}

bool strvContains(strview_t ctx, char c) {
    for(size_t i = 0; i < ctx.size; ++i) {
        if(ctx.buf[i] == c) return true;
    }
    return false;
}

bool strvContainsView(strview_t ctx, strview_t view) {
    if(ctx.size < view.size) return false;
    size_t end = ctx.size - view.size;
    for(size_t i = 0; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.size) == 0) return true;
    }
    return false;
}

size_t strvFind(strview_t ctx, char c, size_t from) {
    for(size_t i = from; i < ctx.size; ++i) {
        if(ctx.buf[i] == c) return i;
    }
    return SIZE_MAX;
}

size_t strvFindView(strview_t ctx, strview_t view, size_t from) {
    if(ctx.size < view.size) return SIZE_MAX;
    size_t end = ctx.size - view.size;
    for(size_t i = from; i < end; ++i) {
        if(memcmp(ctx.buf + i, view.buf, view.size) == 0) return i;
    }
    return SIZE_MAX;
}

size_t strvRFind(strview_t ctx, char c, size_t from) {
    if(from >= ctx.size) {
        from = ctx.size - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(*buf == c) return (buf - ctx.buf);
    }

    return SIZE_MAX;
}

size_t strvRFindView(strview_t ctx, strview_t view, size_t from) {
    from = min(from, ctx.size);

    if(view.size > ctx.size) {
        from -= view.size;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(memcmp(buf, view.buf, view.size) == 0) return (buf - ctx.buf);
    }
    return SIZE_MAX;
}

size_t strvFindFirstOf(strview_t ctx, strview_t view, size_t from) {
    if(ctx.size < view.size) return SIZE_MAX;
    for(size_t i = from; i < ctx.size; ++i) {
        for(size_t j = 0; j < view.size; ++j) {
            if(ctx.buf[i] == view.buf[j]) return i;
        }
    }
    return SIZE_MAX;
}

size_t strvFindLastOf(strview_t ctx, strview_t view, size_t from) {
    if(from >= ctx.size) {
        from = ctx.size - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        for(size_t j = 0; j < view.size; ++j) {
            if(*buf == view.buf[j]) return (buf - ctx.buf);
        }
    }

    return SIZE_MAX;
}

size_t strvFindFirstNot(strview_t ctx, char c, size_t from) {
    size_t end = ctx.size - 1;
    for(size_t i = from; i < end; ++i) {
        if(ctx.buf[i] != c) return i;
    }
    return SIZE_MAX;
}

size_t strvFindFirstNotOf(strview_t ctx, strview_t view, size_t from) {
    for(size_t i = from; i < ctx.size; ++i) {
        if(!strvContains(view, ctx.buf[i])) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t strvFindLastNot(strview_t ctx, char c, size_t from) {
    if(from >= ctx.size) {
        from = ctx.size - 1;
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
    if(from >= ctx.size) {
        from = ctx.size - 1;
    }

    const char *buf = ctx.buf + from;
    for(; buf >= ctx.buf; --buf) {
        if(!strvContains(view, *buf)) {
            return buf - ctx.buf;
        }
    }

    return SIZE_MAX;
}
