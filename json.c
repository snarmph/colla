#include "json.h"

#include "warnings/colla_warn_beg.h"

#include <stdio.h>

#include "strstream.h"
#include "file.h"
#include "tracelog.h"

// #define json__logv() warn("%s:%d", __FILE__, __LINE__)
#define json__logv() 
#define json__ensure(c) json__check_char(in, c)

static bool json__check_char(instream_t *in, char c) {
    if (istrGet(in) == c) {
        return true;
    }
    istrRewindN(in, 1);
    err("wrong character at %zu, should be '%c' but is 0x%02x '%c'", istrTell(*in), c, istrPeek(in), istrPeek(in));
    json__logv();
    return false;
}

static bool json__parse_pair(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out);
static bool json__parse_value(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out);

static bool json__is_value_finished(instream_t *in) {
    usize old_pos = istrTell(*in);
    
    istrSkipWhitespace(in);
    switch(istrPeek(in)) {
        case '}': // fallthrough
        case ']': // fallthrough
        case ',':
            return true;
    }

    in->cur = in->start + old_pos;
    return false;
}

static bool json__parse_null(instream_t *in) {
    strview_t null_view = istrGetViewLen(in, 4);
    bool is_valid = true;
    
    if (!strvEquals(null_view, strv("null"))) {
        err("should be null but is: (%.*s) at %zu", null_view.len, null_view.buf, istrTell(*in));
        is_valid = false;
    }

    if (!json__is_value_finished(in)) {
        err("null, should be finished, but isn't at %zu", istrTell(*in));
        is_valid = false;
    }

    if (!is_valid) json__logv();

    return is_valid;
}

static bool json__parse_array(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out) {
    jsonval_t *head = NULL;
    
    if (!json__ensure('[')) {
        json__logv();
        goto fail;
    }

    istrSkipWhitespace(in);

    // if it is an empty array
    if (istrPeek(in) == ']') {
        istrSkip(in, 1);
        goto success;
    }
    
    if (!json__parse_value(arena, in, flags, &head)) {
        json__logv();
        goto fail;
    }

    jsonval_t *cur = head;
    
    while (true) {
        istrSkipWhitespace(in);
        switch (istrGet(in)) {
            case ']':
                return head;
            case ',':
            {
                istrSkipWhitespace(in);
                // trailing comma
                if (istrPeek(in) == ']') {
                    if (flags & JSON_NO_TRAILING_COMMAS) {
                        err("trailing comma in array at at %zu: (%c)(%d)", istrTell(*in), *in->cur, *in->cur);
                        json__logv();
                        goto fail;
                    }
                    else {
                        continue;
                    }
                }

                jsonval_t *next = NULL;
                if (!json__parse_value(arena, in, flags, &next)) {
                    json__logv();
                    goto fail;
                }
                cur->next = next;
                next->prev = cur;
                cur = next;
                break;
            }
            default:
                istrRewindN(in, 1);
                err("unknown char after array at %zu: (%c)(%d)", istrTell(*in), *in->cur, *in->cur);
                json__logv();
                goto fail;
        }
    }

success:
    *out = head;
    return true;
fail:
    *out = NULL;
    return false;
}

static bool json__parse_string(arena_t *arena, instream_t *in, str_t *out) {
    istrSkipWhitespace(in); 

    if (!json__ensure('"')) {
        json__logv();
        goto fail;
    }

    const char *from = in->cur;
    
    for (; !istrIsFinished(*in) && *in->cur != '"'; ++in->cur) {
        if (istrPeek(in) == '\\') {
            ++in->cur;
        }
    }
    
    usize len = in->cur - from;

    *out = str(arena, from, len);

    if (!json__ensure('"')) {
        json__logv();
        goto fail;
    }

    return true;
fail:
    *out = STR_EMPTY;
    return false;
}

static bool json__parse_number(instream_t *in, double *out) {
    return istrGetDouble(in, out);
}

static bool json__parse_bool(instream_t *in, bool *out) {
    size_t remaining = istrRemaining(*in);
    if (remaining >= 4 && memcmp(in->cur, "true", 4) == 0) {
        istrSkip(in, 4);
        *out = true;
    }
    else if (remaining >= 5 && memcmp(in->cur, "false", 5) == 0) {
        istrSkip(in, 5);
        *out = false;
    }
    else {
        err("unknown boolean at %zu: %.10s", istrTell(*in), in->cur);
        json__logv();
        return false;
    }

    return true;
}

static bool json__parse_obj(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out) {
    if (!json__ensure('{')) {
        json__logv();
        goto fail;
    }

    istrSkipWhitespace(in);

    // if it is an empty object
    if (istrPeek(in) == '}') {
        istrSkip(in, 1);
        *out = NULL;
        return true;
    }

    jsonval_t *head = NULL;
    if (!json__parse_pair(arena, in, flags, &head)) {
        json__logv();
        goto fail;
    }
    jsonval_t *cur = head;

    while (true) {
        istrSkipWhitespace(in);
        switch (istrGet(in)) {
            case '}':
                goto success;
            case ',':
            {
                istrSkipWhitespace(in);
                // trailing commas
                if (!(flags & JSON_NO_TRAILING_COMMAS) && istrPeek(in) == '}') {
                    goto success;
                }

                jsonval_t *next = NULL;
                if (!json__parse_pair(arena, in, flags, &next)) {
                    json__logv();
                    goto fail;
                }
                cur->next = next;
                next->prev = cur;
                cur = next;
                break;
            }
            default:
                istrRewindN(in, 1);
                err("unknown char after object at %zu: (%c)(%d)", istrTell(*in), *in->cur, *in->cur);
                json__logv();
                goto fail;
        }
    }

success:
    *out = head;
    return true;
fail:
    *out = NULL;
    return false;
}

static bool json__parse_pair(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out) {
    str_t key = {0};
    if (!json__parse_string(arena, in, &key)) {
        json__logv();
        goto fail;
    }

    // skip preamble
    istrSkipWhitespace(in);
    if (!json__ensure(':')) {
        json__logv();
        goto fail;
    }

    if (!json__parse_value(arena, in, flags, out)) {
        json__logv();
        goto fail;
    }
    
    (*out)->key = key;
    return true;

fail: 
    *out = NULL;
    return false;
}

static bool json__parse_value(arena_t *arena, instream_t *in, jsonflags_e flags, jsonval_t **out) {
    jsonval_t *val = alloc(arena, jsonval_t);

    istrSkipWhitespace(in);

    switch (istrPeek(in)) {
        // object
        case '{':
            if (!json__parse_obj(arena, in, flags, &val->object)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_OBJECT;
            break;
        // array
        case '[':
            if (!json__parse_array(arena, in, flags, &val->array)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_ARRAY;
            break;
        // string
        case '"':
            if (!json__parse_string(arena, in, &val->string)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_STRING;
            break;
        // boolean
        case 't': // fallthrough
        case 'f':
            if (!json__parse_bool(in, &val->boolean)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_BOOL;
            break;
        // null
        case 'n': 
            if (!json__parse_null(in)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_NULL;
            break;
        // comment
        case '/':
            err("TODO comments");
            break;
        // number
        default:
            if (!json__parse_number(in, &val->number)) {
                json__logv();
                goto fail;
            }
            val->type = JSON_NUMBER;
            break;
    }

    *out = val;
    return true;
fail:
    *out = NULL;
    return false;
}

json_t jsonParse(arena_t *arena, arena_t scratch, strview_t filename, jsonflags_e flags) {
    str_t data = fileReadWholeStr(&scratch, filename);
    return NULL;
    json_t json = jsonParseStr(arena, strv(data), flags);
    return json;
}

json_t jsonParseStr(arena_t *arena, strview_t jsonstr, jsonflags_e flags) {
    arena_t before = *arena;

    jsonval_t *root = alloc(arena, jsonval_t);
    root->type = JSON_OBJECT;
    
    instream_t in = istrInitLen(jsonstr.buf, jsonstr.len);
    
    if (!json__parse_obj(arena, &in, flags, &root->object)) {
        // reset arena
        *arena = before;
        json__logv();
        return NULL;
    }

    return root;
}

jsonval_t *jsonGet(jsonval_t *node, strview_t key) {
    if (!node) return NULL;

    if (node->type != JSON_OBJECT) {
        err("passed type is not an object");
        return NULL;
    }

    node = node->object;

    while (node) {
        if (strvEquals(strv(node->key), key)) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

#include "warnings/colla_warn_end.h"