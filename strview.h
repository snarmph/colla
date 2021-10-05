#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include "slice.h"

#define STRV_NOT_FOUND SIZE_MAX

// typedef struct {
//     const char *buf;
//     size_t size;
// } strview_t;

typedef slice_t(const char *) strview_t;

strview_t strvInit(const char *cstr);
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

size_t strvCopy(strview_t ctx, char **buf);
size_t strvCopyN(strview_t ctx, char **buf, size_t count, size_t from);
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