#include "ini.h"

#include "strstream.h"
#include "file.h"
#include "tracelog.h"

// == INI READER ========================================================================

static const iniopts_t default_opts = {0};

static initable_t *findTable(ini_t *ctx, strview_t name);
static inivalue_t *findValue(vec(inivalue_t) values, strview_t key);
static void addTable(ini_t *ctx, str_istream_t *in, const iniopts_t *options);
static void addValue(initable_t *table, str_istream_t *in, const iniopts_t *options);

void _iniParseInternal(ini_t *ini, const iniopts_t *options) {
    // add root table
    vecAppend(ini->tables, (initable_t){0});
    str_istream_t in = istrInitLen(ini->text.buf, ini->text.len);
        istrSkipWhitespace(&in);
    while (!istrIsFinished(in)) {
        switch(*in.cur) {
        case '[':
            addTable(ini, &in, options);
            break;
        case '#': case ';':
            istrIgnore(&in, '\n');
            break;
        default:
            addValue(&ini->tables[0], &in, options);
            break;
        }
        istrSkipWhitespace(&in);
    }
    
}

ini_t iniParse(const char *filename, const iniopts_t *options) {
    ini_t ini = { .text = fileReadWholeText(filename) };
    if (strIsEmpty(ini.text)) return ini;
    if (!options) options = &default_opts;
    _iniParseInternal(&ini, options);
    return ini;
}

ini_t iniParseString(const char *inistr, const iniopts_t *options) {
    ini_t ini = { .text = strFromStr(inistr) };
    if (!options) options = &default_opts;
    _iniParseInternal(&ini, options);
    return ini;
}

void iniFree(ini_t ctx) {
    strFree(ctx.text);
    for (uint32 i = 0; i < vecLen(ctx.tables); ++i) {
        vecFree(ctx.tables[i].values);
    }
    vecFree(ctx.tables);
}

initable_t *iniGetTable(ini_t *ctx, const char *name) {
    if (!name) {
        return &ctx->tables[0];
    }
    else {
        return findTable(ctx, strvInit(name));
    }
}

inivalue_t *iniGet(initable_t *ctx, const char *key) {
    return ctx ? findValue(ctx->values, strvInit(key)) : NULL;
}

vec(strview_t) iniAsArray(const inivalue_t *value, char delim) {
    if (!value) return NULL;
    if (!delim) delim = ' ';

    vec(strview_t) out = NULL;
    strview_t v = value->value;

    usize start = 0;
    for (usize i = 0; i < v.len; ++i) {
        if (v.buf[i] == delim) {
            strview_t arr_val = strvTrim(strvSub(v, start, i));
            if (!strvIsEmpty(arr_val)) vecAppend(out, arr_val);
            start = i + 1;
        }
    }
    strview_t last = strvTrim(strvSub(v, start, SIZE_MAX));
    if (!strvIsEmpty(last)) vecAppend(out, last);
    return out;
}

vec(strview_t) iniAsArrayU8(const inivalue_t *value, const char *delim) {
    if (!value || !delim) return NULL;

    rune cpdelim = utf8Decode(&delim);
    vec(strview_t) out = NULL;
    strview_t v = value->value;

    const char *start = v.buf;
    const char *buf = v.buf;
    const char *prevbuf = buf;

    for(rune cp = utf8Decode(&buf); 
        buf != (v.buf + v.len); 
        cp = utf8Decode(&buf)
    ) {
        if (cp == cpdelim) {
            usize start_pos = start - v.buf;
            usize end_pos = prevbuf - v.buf;
            strview_t arr_val = strvTrim(strvSub(v, start_pos, end_pos));
            if (!strvIsEmpty(arr_val)) vecAppend(out, arr_val);
            // buf has already gone to the next codepoint, skipping the delimiter
            start = buf;
        }
        prevbuf = buf;
    }

    strview_t last = strvTrim(strvSub(v, start - v.buf, SIZE_MAX));
    if (!strvIsEmpty(last)) vecAppend(out, last);
    return out;
}

uint64 iniAsUInt(const inivalue_t *value) {
    if (!value) return 0;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    uint64 val = 0;
    if (!istrGetu64(&in, &val)) val = 0;
    return val;
}

int64 iniAsInt(const inivalue_t *value) {
    if (!value) return 0;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    int64 val = 0;
    if (!istrGeti64(&in, &val)) val = 0;
    return val;
}

double iniAsNum(const inivalue_t *value) {
    if (!value) return 0.f;
    str_istream_t in = istrInitLen(value->value.buf, value->value.len);
    double val = 0;
    if (!istrGetdouble(&in, &val)) val = 0;
    return val;
}

// == INI WRITER ========================================================================

#include "strstream.h"
#include "file.h"

static const winiopts_t default_wopts = {0};

iniwriter_t winiInit() {
    iniwriter_t out = {0};
    vecAppend(out.tables, (winitable_t){0});
    return out;
}

void winiFree(iniwriter_t ctx) {
    for (winitable_t *tab = ctx.tables; tab != vecEnd(ctx.tables); ++tab) {
        strFree(tab->key);
        for (winivalue_t *val = tab->values; val != vecEnd(tab->values); ++val) {
            strFree(val->key);
            strFree(val->value);
        }
        vecFree(tab->values);
    }
    vecFree(ctx.tables);
}

str_t winiToString(iniwriter_t ctx, const winiopts_t *options) {
    if (!options) options = &default_wopts;

    str_ostream_t out = ostrInitLen(1024 * 20);
    if (!options->no_discalimer) ostrPuts(&out, "# auto-generated by colla's ini.h, do not modify!\n");
    // add root values
    winitable_t *root = &ctx.tables[0];
    for (winivalue_t *val = root->values; val != vecEnd(root->values); ++val) {
        ostrPrintf(&out, "%s = %s\n", val->key.buf, val->value.buf);
    }
    if (root->values) ostrPuts(&out, "\n");
    // add each table
    for (usize i = 1; i < vecLen(ctx.tables); ++i) {
        winitable_t *tab = &ctx.tables[i];
        ostrPrintf(&out, "[%s]\n", tab->key.buf);
        for (winivalue_t *val = tab->values; val != vecEnd(tab->values); ++val) {
            ostrPrintf(&out, "%s = %s\n", val->key.buf, val->value.buf);
        }
        if ((i + 1) < vecLen(ctx.tables)) ostrPuts(&out, "\n");
    }
    return ostrAsStr(out);
}

void winiToFile(iniwriter_t ctx, const char *filename, const winiopts_t *options) {
    if (!options) options = &default_wopts;

    file_t fp = fileOpen(filename, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't write ini to file %s", filename);
        return;
    }
    str_t string = winiToString(ctx, options);
    fileWriteWholeTextFP(fp, strvInitStr(string));
    strFree(string);
    fileClose(fp);
}

winivalue_t *winiAddValEmpty(winitable_t *table) {
    if (!table) return NULL;
    vecAppend(table->values, (winivalue_t){0});
    return &vecBack(table->values);
}

winivalue_t *winiAddVal(winitable_t *table, const char *key, const char *value) {
    if (!table) return NULL;
    winivalue_t val = { .key = strFromStr(key), .value = strFromStr(value) };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winivalue_t *winiAddValStr(winitable_t *table, str_t key, str_t value) {
    if (!table) return NULL;
    winivalue_t val = { .key = key, .value = value };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winivalue_t *winiAddValView(winitable_t *table, strview_t key, strview_t value) {
    if (!table) return NULL;
    winivalue_t val = { .key = strFromView(key), .value = strFromView(value) };
    vecAppend(table->values, val);
    return &vecBack(table->values);
}

winitable_t *winiAddTablEmpty(iniwriter_t *ctx) {
    vecAppend(ctx->tables, (winitable_t){0});
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTab(iniwriter_t *ctx, const char *name) {
    winitable_t tab = { .key = strFromStr(name) };
    vecAppend(ctx->tables, tab);
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTabStr(iniwriter_t *ctx, str_t name) {
    winitable_t tab = { .key = name };
    vecAppend(ctx->tables, tab);    
    return &vecBack(ctx->tables);
}

winitable_t *winiAddTabView(iniwriter_t *ctx, strview_t name) {
    winitable_t tab = { .key = strFromView(name) };
    vecAppend(ctx->tables, tab);
    return &vecBack(ctx->tables);
}

// == PRIVATE FUNCTIONS ========================================================

static initable_t *findTable(ini_t *ctx, strview_t name) {
    if (strvIsEmpty(name)) return NULL;
    for (uint32 i = 1; i < vecLen(ctx->tables); ++i) {
        if (strvCompare(ctx->tables[i].name, name) == 0) {
            return &ctx->tables[i];
        }
    }
    return NULL;
}

static inivalue_t *findValue(vec(inivalue_t) values, strview_t key) {
    if (strvIsEmpty(key)) return NULL;
    for (uint32 i = 0; i < vecLen(values); ++i) {
        if (strvCompare(values[i].key, key) == 0) {
            return &values[i];
        }
    }
    return NULL;
}

static void addTable(ini_t *ctx, str_istream_t *in, const iniopts_t *options) {
    istrSkip(in, 1); // skip [
    strview_t name = istrGetview(in, ']');
    istrSkip(in, 1); // skip ]
    initable_t *table = options->merge_duplicate_tables ? findTable(ctx, name) : NULL;
    if (!table) {
        vecAppend(ctx->tables, (initable_t){ name });
        table = &vecBack(ctx->tables);
    }
    istrIgnore(in, '\n'); istrSkip(in, 1);
    while (!istrIsFinished(*in)) {
        switch (*in->cur) {
        case '\n': case '\r':
            return;
        case '#': case ';':
            istrIgnore(in, '\n');
            break;
        default:
            addValue(table, in, options);
            break;
        }
    }
}

static void addValue(initable_t *table, str_istream_t *in, const iniopts_t *options) {
    if (!table) fatal("table is null");
    
    strview_t key = strvTrim(istrGetview(in, '='));
    istrSkip(in, 1);
    strview_t value = strvTrim(istrGetview(in, '\n'));
    // value might be until EOF, in that case no use in skipping
    if (!istrIsFinished(*in)) istrSkip(in, 1); // skip newline
    inivalue_t *new_value = options->merge_duplicate_keys ? findValue(table->values, key) : NULL;
    if (!new_value) {
        inivalue_t ini_val = (inivalue_t){ key, value };
        vecAppend(table->values, ini_val);
    }
    else {
        new_value->value = value;
    }
}
