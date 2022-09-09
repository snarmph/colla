#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <wchar.h>

#include "collatypes.h"

#define STRV_NOT_FOUND SIZE_MAX

typedef struct str_t {
    char *buf;
    usize len;
} str_t;

typedef struct {
    const char *buf;
    usize len;
} strview_t;

// == STR_T ========================================================

str_t strInit(void);
str_t strFromStr(const char *cstr);
str_t strFromView(strview_t view);
str_t strFromBuf(const char *buf, usize len);
str_t strFromFmt(const char *fmt, ...);

str_t strFromWCHAR(const wchar_t *src, usize len);
wchar_t *strToWCHAR(str_t ctx);

void strFree(str_t ctx);
str_t strDup(str_t ctx);
str_t strMove(str_t *ctx);

strview_t strGetView(str_t ctx);

char *strBegin(str_t ctx);
char *strEnd(str_t ctx);
char strBack(str_t ctx);

bool strIsEmpty(str_t ctx);
void strReplace(str_t *ctx, char from, char to);

// if len == SIZE_MAX, copies until end
str_t strSubstr(str_t ctx, usize from, usize to);
// if len == SIZE_MAX, returns until end
strview_t strSubview(str_t ctx, usize from, usize to);

void strLower(str_t *ctx);
str_t strToLower(str_t ctx);

void strUpper(str_t *ctx);
str_t strToUpper(str_t ctx);

#ifdef STR_TESTING
void strTest(void);
#endif

// == STRVIEW_T ====================================================

strview_t strvInit(const char *cstr);
strview_t strvInitStr(str_t str);
strview_t strvInitLen(const char *buf, usize size);

char strvFront(strview_t ctx);
char strvBack(strview_t ctx);
const char *strvBegin(strview_t ctx);
const char *strvEnd(strview_t ctx);
// move view forward by n characters
strview_t strvRemovePrefix(strview_t ctx, usize n);
// move view backwards by n characters
strview_t strvRemoveSuffix(strview_t ctx, usize n);
// removes whitespace from the beginning and the end
strview_t strvTrim(strview_t ctx);
// removes whitespace from the beginning
strview_t strvTrimLeft(strview_t ctx);
// removes whitespace from the end
strview_t strvTrimRight(strview_t ctx);

bool strvIsEmpty(strview_t ctx);

str_t strvCopy(strview_t ctx);
str_t strvCopyN(strview_t ctx, usize count, usize from);
usize strvCopyBuf(strview_t ctx, char *buf, usize len, usize from);

strview_t strvSub(strview_t ctx, usize from, usize to);
int strvCompare(strview_t ctx, strview_t other);
int strvICompare(strview_t ctx, strview_t other);

bool strvStartsWith(strview_t ctx, char c);
bool strvStartsWithView(strview_t ctx, strview_t view);

bool strvEndsWith(strview_t ctx, char c);
bool strvEndsWithView(strview_t ctx, strview_t view);

bool strvContains(strview_t ctx, char c);
bool strvContainsView(strview_t ctx, strview_t view);

usize strvFind(strview_t ctx, char c, usize from);
usize strvFindView(strview_t ctx, strview_t view, usize from);

usize strvRFind(strview_t ctx, char c, usize from);
usize strvRFindView(strview_t ctx, strview_t view, usize from);

// Finds the first occurrence of any of the characters of 'view' in this view
usize strvFindFirstOf(strview_t ctx, strview_t view, usize from);
usize strvFindLastOf(strview_t ctx, strview_t view, usize from);

usize strvFindFirstNot(strview_t ctx, char c, usize from);
usize strvFindFirstNotOf(strview_t ctx, strview_t view, usize from);
usize strvFindLastNot(strview_t ctx, char c, usize from);
usize strvFindLastNotOf(strview_t ctx, strview_t view, usize from);

#ifdef __cplusplus
} // extern "C"
#endif
