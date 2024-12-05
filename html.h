#pragma once

#include "strstream.h"

typedef struct {
    outstream_t stream;
    str_t *output;
} html_context_t;

strview_t html__strv_copy(strview_t src) { return src; }

#define html__str_or_strv(str) _Generic(str, \
        strview_t: html__strv_copy, \
        str_t: strvInitStr, \
        const char *: strvInit, \
        char *: strvInit \
    )(str)

#define htmlPrintf(...) ostrPrintf(&__ctx.stream, __VA_ARGS__)
#define htmlPuts(str) ostrPuts(&__ctx.stream, html__str_or_strv(str))

#define htmlBeg(arena_ptr, str_ptr) { \
    html_context_t __ctx = { .stream = ostrInit(arena_ptr), .output = str_ptr }; \
    htmlPrintf("<!DOCTYPE html>\n<html>");
#define htmlEnd()  htmlPrintf("</html>"); *__ctx.output = ostrAsStr(&__ctx.stream); }

#define html__args() \
    X(class)         \
    X(id)            \
    X(style)         \
    X(onclick)       \
    X(href)          \
    X(src)           \

typedef struct {
#define X(name) const char *name;
    html__args()
#undef X
} html_tag_t;

static void html__tag(html_context_t *ctx, const char *tag, html_tag_t *args) {
    ostrPrintf(&ctx->stream, "<%s", tag);

#define X(name, ...) if (args->name) { ostrPrintf(&ctx->stream, " " #name "=\"%s\"", args->name); }
    html__args()
#undef X

    ostrPutc(&ctx->stream, '>');
}

#define tagBeg(tag, ...) do { html_tag_t args = {0, __VA_ARGS__}; html__tag(&__ctx, tag, &args); } while (0)
#define tagEnd(tag) htmlPrintf("</"tag">")

#define html__strv_or_str(s) _Generic(s, str_t: NULL)

#define html__simple_tag(tag, text, ...) do { tagBeg(tag, __VA_ARGS__); htmlPuts(text); tagEnd(tag); } while (0)

#define headBeg(...) tagBeg("head", __VA_ARGS__)
#define headEnd()    tagEnd("head")

#define bodyBeg(...) tagBeg("body", __VA_ARGS__)
#define bodyEnd()    tagEnd("body")

#define divBeg(...)  tagBeg("div", __VA_ARGS__)
#define divEnd()     tagEnd("div")

#define htmlRaw(data) ostrPuts(&__ctx.stream, strv(#data))

#define title(text, ...) html__simple_tag("title", text, __VA_ARGS__)
#define h1(text, ...)    html__simple_tag("h1", text, __VA_ARGS__)
#define p(text, ...)     html__simple_tag("p", text, __VA_ARGS__)
#define span(text, ...)  html__simple_tag("span", text, __VA_ARGS__)
#define a(text, ...)     html__simple_tag("a", text, __VA_ARGS__)
#define img(...)         tagBeg("img", __VA_ARGS__)
#define style(text, ...) html__simple_tag("style", text, __VA_ARGS__)

#define hr()             htmlPuts("<hr>")
