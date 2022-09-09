#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include "tracelog.h"
#include "strstream.h"

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
