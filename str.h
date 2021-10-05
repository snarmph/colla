#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "strview.h"

#define STR_TESTING

typedef struct {
    char *buf;
    size_t len;
} str_t;

str_t strInit(void);
str_t strInitStr(const char *cstr);
str_t strInitView(strview_t view);
str_t strInitBuf(const char *buf, size_t len);

void strFree(str_t *ctx);

str_t strMove(str_t *ctx);
str_t strDup(str_t ctx);

strview_t strGetView(str_t *ctx);

char *strBegin(str_t *ctx);
char *strEnd(str_t *ctx);

char strBack(str_t *ctx);

bool strIsEmpty(str_t *ctx);

void strAppend(str_t *ctx, const char *str);
void strAppendStr(str_t *ctx, str_t str);
void strAppendView(str_t *ctx, strview_t view);
void strAppendBuf(str_t *ctx, const char *buf, size_t len);
void strAppendChars(str_t *ctx, char c, size_t count);

void strPush(str_t *ctx, char c);
char strPop(str_t *ctx);

void strSwap(str_t *ctx, str_t *other);

// if len == SIZE_MAX, copies until end
str_t strSubstr(str_t *ctx, size_t pos, size_t len);
// if len == SIZE_MAX, returns until end
strview_t strSubview(str_t *ctx, size_t pos, size_t len);

void strLower(str_t *ctx);
str_t strToLower(str_t ctx);

#ifdef STR_TESTING
void strTest(void);
#endif

#ifdef __cplusplus
} // extern "C"
#endif