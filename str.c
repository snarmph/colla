#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
    return strInitBuf(view.buf, view.size);
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
        .size = ctx->len
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
    strAppendBuf(ctx, view.buf, view.size);
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

#include "tracelog.h"

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
        .size = len
    };
}

void strLower(str_t *ctx) {
    for(size_t i = 0; i < ctx->len; ++i) {
        ctx->buf[i] = tolower(ctx->buf[i]);
    }
}

str_t strToLower(str_t ctx) {
    str_t str = strDup(ctx);
    strLower(&str);
    return str;
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
        printf("\"%.*s\" %zu\n", (int)view.size, view.buf, view.size);
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
        printf("0..5: \"%.*s\"\n", (int)v.size, v.buf);
        v = strSubview(&s, 5, SIZE_MAX);
        printf("6..SIZE_MAX: \"%.*s\"\n", (int)v.size, v.buf);

        strFree(&s);
    }

    strFree(&s);
}
#endif