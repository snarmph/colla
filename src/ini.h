#pragma once

#include <stdbool.h>
#include "str.h"
#include "vec.h"
#include "utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

// == INI READER ========================================================================

typedef struct {
    strview_t key;
    strview_t value;
} inivalue_t;

typedef struct {
    strview_t name;
    vec(inivalue_t) values;
} initable_t;

typedef struct {
    str_t text;
    vec(initable_t) tables;
} ini_t;

typedef struct {
    bool merge_duplicate_tables;
    bool merge_duplicate_keys;
} iniopts_t;

ini_t iniParse(const char *filename, const iniopts_t *options);
ini_t iniParseString(const char *inistr, const iniopts_t *options);
void iniFree(ini_t ctx);

initable_t *iniGetTable(ini_t *ctx, const char *name);
inivalue_t *iniGet(initable_t *ctx, const char *key);

vec(strview_t) iniAsArray(const inivalue_t *value, char delim);
// delim is expected to be a single utf8 character
vec(strview_t) iniAsArrayU8(const inivalue_t *value, const char *delim);
uint64 iniAsUInt(const inivalue_t *value);
int64 iniAsInt(const inivalue_t *value);
double iniAsNum(const inivalue_t *value);
bool iniAsBool(const inivalue_t *value);

// == INI WRITER ========================================================================

typedef struct {
    str_t key;
    str_t value;
} winivalue_t;

typedef struct {
    str_t key;
    vec(winivalue_t) values;
} winitable_t;

typedef struct {
    vec(winitable_t) tables;
} iniwriter_t;

typedef struct {
    bool no_discalimer;
} winiopts_t;

iniwriter_t winiInit();
void winiFree(iniwriter_t ctx);

str_t winiToString(iniwriter_t ctx, const winiopts_t *options);
void winiToFile(iniwriter_t ctx, const char *filename, const winiopts_t *options);

winivalue_t *winiAddValEmpty(winitable_t *table);
winivalue_t *winiAddVal(winitable_t *table, const char *key, const char *value);
winivalue_t *winiAddValStr(winitable_t *table, str_t key, str_t value);
winivalue_t *winiAddValView(winitable_t *table, strview_t key, strview_t value);

winitable_t *winiAddTablEmpty(iniwriter_t *ctx);
winitable_t *winiAddTab(iniwriter_t *ctx, const char *name);
winitable_t *winiAddTabStr(iniwriter_t *ctx, str_t name);
winitable_t *winiAddTabView(iniwriter_t *ctx, strview_t name);

#ifdef __cplusplus
} // extern "C"
#endif
