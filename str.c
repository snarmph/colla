#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include "tracelog.h"

#ifdef _WIN32
#include "win32_slim.h"
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

str_t strInitStr(const char *cstr) {
    return strInitBuf(cstr, strlen(cstr));
}

str_t strInitView(strview_t view) {
    return strInitBuf(view.buf, view.len);
}

str_t strInitBuf(const char *buf, size_t len) {
    str_t str;
    str.len = len;
    str.buf = malloc(len + 1);
    memcpy(str.buf, buf, len);
    str.buf[len] = '\0';
    return str;
}

void strFree(str_t *ctx) {
    free(ctx->buf);
    ctx->buf = NULL;
    ctx->len = 0;
}

str_t strFromWCHAR(const wchar_t *src, size_t len) {
    if(len == 0) len = wcslen(src);

#ifdef _WIN32
    // TODO CP_ACP should be CP_UTF8 but if i put CP_UTF8 it doesn't work?? 
    int result_len = WideCharToMultiByte(
        CP_ACP, 0, 
        src, (int)len, 
        NULL, 0,
        NULL, NULL
    );
    char *buf = malloc(result_len + 1);
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
    size_t actual_len = len * sizeof(wchar_t);

    size_t dest_len = len * 6;
    char *dest = malloc(dest_len);

    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    assert(cd);

    size_t dest_left = dest_len;
    char *dest_temp = dest;
    char *src_temp = (char*)src;
    size_t lost = iconv(cd, &src_temp, &actual_len, &dest_temp, &dest_left);
    assert(lost != ((size_t)-1));

    dest_len -= dest_left;
    dest = realloc(dest, dest_len + 1);
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
    wchar_t *str = malloc(sizeof(wchar_t) * (len + 1));
    if(!str) return NULL;
    len = MultiByteToWideChar(
        codepage, 0,
        ctx.buf, (int)ctx.len,
        str, len
    );
    str[len] = '\0';
    return str;
#else
    size_t dest_len = ctx.len * sizeof(wchar_t);
    char *dest = malloc(dest_len);

    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    assert(cd);

    size_t dest_left = dest_len;
    char *dest_temp = dest;
    char *src_temp = ctx.buf;
    size_t lost = iconv(cd, &src_temp, &ctx.len, &dest_temp, &dest_left);
    assert(lost != ((size_t)-1));

    dest_len -= dest_left;
    dest = realloc(dest, dest_len + 1);
    dest[dest_len] = '\0';

    iconv_close(cd);

    return (wchar_t *)dest;
#endif
}

str_t strMove(str_t *ctx) {
    str_t str = strInitBuf(ctx->buf, ctx->len);
    ctx->buf = NULL;
    ctx->len = 0;
    return str;
}

str_t strDup(str_t ctx) {
    return strInitBuf(ctx.buf, ctx.len);
}

strview_t strGetView(str_t *ctx) {
    return (strview_t) {
        .buf = ctx->buf,
        .len = ctx->len
    };
}

char *strBegin(str_t *ctx) {
    return ctx->buf;
}

char *strEnd(str_t *ctx) {
    return ctx->buf ? ctx->buf + ctx->len : NULL;
}

char strBack(str_t *ctx) {
    return ctx->buf ? ctx->buf[ctx->len - 1] : '\0';
}

bool strIsEmpty(str_t *ctx) {
    return ctx->len == 0;
}

void strAppend(str_t *ctx, const char *str) {
    strAppendBuf(ctx, str, strlen(str));
}

void strAppendStr(str_t *ctx, str_t str) {
    strAppendBuf(ctx, str.buf, str.len);
}

void strAppendView(str_t *ctx, strview_t view) {
    strAppendBuf(ctx, view.buf, view.len);
}

void strAppendBuf(str_t *ctx, const char *buf, size_t len) {
    size_t oldlen = ctx->len;
    ctx->len += len;
    ctx->buf = realloc(ctx->buf, ctx->len + 1);
    memcpy(ctx->buf + oldlen, buf, len);
    ctx->buf[ctx->len] = '\0';
}

void strAppendChars(str_t *ctx, char c, size_t count) {
    size_t oldlen = ctx->len;
    ctx->len += count;
    ctx->buf = realloc(ctx->buf, ctx->len + 1);
    memset(ctx->buf + oldlen, c, count);
    ctx->buf[ctx->len] = '\0';
}

void strPush(str_t *ctx, char c) {
    strAppendChars(ctx, c, 1);
}

char strPop(str_t *ctx) {
    char c = strBack(ctx);
    ctx->buf = realloc(ctx->buf, ctx->len);
    ctx->len -= 1;
    ctx->buf[ctx->len] = '\0';
    return c;
}

void strSwap(str_t *ctx, str_t *other) {
    char *buf  = other->buf;
    size_t len = other->len;
    other->buf = ctx->buf;
    other->len = ctx->len;
    ctx->buf = buf;
    ctx->len = len;
}

void strReplace(str_t *ctx, char from, char to) {
    for(size_t i = 0; i < ctx->len; ++i) {
        if(ctx->buf[i] == from) {
            ctx->buf[i] = to;
        }
    }
}

str_t strSubstr(str_t *ctx, size_t pos, size_t len) {
    if(strIsEmpty(ctx)) return strInit();
    if(len == SIZE_MAX || (pos + len) > ctx->len) len = ctx->len - pos;
    return strInitBuf(ctx->buf + pos, len);
}

strview_t strSubview(str_t *ctx, size_t pos, size_t len) {
    if(strIsEmpty(ctx)) return strvInit(NULL);
    if(len == SIZE_MAX || (pos + len) > ctx->len) len = ctx->len - pos;
    return (strview_t) {
        .buf = ctx->buf + pos,
        .len = len
    };
}

void strLower(str_t *ctx) {
    for(size_t i = 0; i < ctx->len; ++i) {
        ctx->buf[i] = (char)tolower(ctx->buf[i]);
    }
}

str_t strToLower(str_t ctx) {
    str_t str = strDup(ctx);
    strLower(&str);
    return str;
}

void strUpper(str_t *ctx) {
    for(size_t i = 0; i < ctx->len; ++i) {
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

strview_t strvInitLen(const char *buf, size_t size) {
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

str_t strvCopy(strview_t ctx) {
    return strInitView(ctx);
}

str_t strvCopyN(strview_t ctx, size_t count, size_t from) {
    size_t sz = ctx.len + 1 - from;
    count = min(count, sz);
    return strInitBuf(ctx.buf + from, count);
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

#ifdef STR_TESTING
#include <stdio.h>
#include "tracelog.h"

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
        uint8_t buf[] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
        s = strInitBuf((char *)buf, sizeof(buf));
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
        uint8_t buf[] = { 'g', 'o', 'i', 'n', 'g' };
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