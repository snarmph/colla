#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <wchar.h>
// #include "strview.h"

#define STRV_NOT_FOUND SIZE_MAX

typedef struct {
    char *buf;
    size_t len;
} str_t;

typedef struct {
    const char *buf;
    size_t len;
} strview_t;

// == STR_T ========================================================

str_t strInit(void);
str_t strInitStr(const char *cstr);
str_t strInitView(strview_t view);
str_t strInitBuf(const char *buf, size_t len);

str_t strFromWCHAR(const wchar_t *src, size_t len);
wchar_t *strToWCHAR(str_t ctx);

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

void strReplace(str_t *ctx, char from, char to);

// if len == SIZE_MAX, copies until end
str_t strSubstr(str_t *ctx, size_t pos, size_t len);
// if len == SIZE_MAX, returns until end
strview_t strSubview(str_t *ctx, size_t pos, size_t len);

void strLower(str_t *ctx);
str_t strToLower(str_t ctx);

#ifdef STR_TESTING
void strTest(void);
#endif

// == STRVIEW_T ====================================================

strview_t strvInit(const char *cstr);
strview_t strvInitStr(str_t str);
strview_t strvInitLen(const char *buf, size_t size);

char strvFront(strview_t ctx);
char strvBack(strview_t ctx);
const char *strvBegin(strview_t *ctx);
const char *strvEnd(strview_t *ctx);
// move view forward by n characters
void strvRemovePrefix(strview_t *ctx, size_t n);
// move view backwards by n characters
void strvRemoveSuffix(strview_t *ctx, size_t n);

bool strvIsEmpty(strview_t ctx);

str_t strvCopy(strview_t ctx);
str_t strvCopyN(strview_t ctx, size_t count, size_t from);
size_t strvCopyBuf(strview_t ctx, char *buf, size_t len, size_t from);

strview_t strvSubstr(strview_t ctx, size_t from, size_t len);
int strvCompare(strview_t ctx, strview_t other);
int strvICompare(strview_t ctx, strview_t other);

bool strvStartsWith(strview_t ctx, char c);
bool strvStartsWithView(strview_t ctx, strview_t view);

bool strvEndsWith(strview_t ctx, char c);
bool strvEndsWithView(strview_t ctx, strview_t view);

bool strvContains(strview_t ctx, char c);
bool strvContainsView(strview_t ctx, strview_t view);

size_t strvFind(strview_t ctx, char c, size_t from);
size_t strvFindView(strview_t ctx, strview_t view, size_t from);

size_t strvRFind(strview_t ctx, char c, size_t from);
size_t strvRFindView(strview_t ctx, strview_t view, size_t from);

// Finds the first occurrence of any of the characters of 'view' in this view
size_t strvFindFirstOf(strview_t ctx, strview_t view, size_t from);
size_t strvFindLastOf(strview_t ctx, strview_t view, size_t from);

size_t strvFindFirstNot(strview_t ctx, char c, size_t from);
size_t strvFindFirstNotOf(strview_t ctx, strview_t view, size_t from);
size_t strvFindLastNot(strview_t ctx, char c, size_t from);
size_t strvFindLastNotOf(strview_t ctx, strview_t view, size_t from);


#ifdef __cplusplus
} // extern "C"
#endif