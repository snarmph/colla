#include "parsers.h"

#include "os.h"

// == INI ============================================

void ini__parse(arena_t *arena, ini_t *ini, const iniopt_t *options);

ini_t ini_parse(arena_t *arena, strview_t filename, iniopt_t *opt) {
    oshandle_t fp = os_file_open(filename, FILEMODE_READ);
    ini_t out = ini_parse_fp(arena, fp, opt);
    os_file_close(fp);
    return out;
}

ini_t ini_parse_fp(arena_t *arena, oshandle_t file, iniopt_t *opt) {
    str_t data = os_file_read_all_str_fp(arena, file);
    return ini_parse_str(arena, strv(data), opt);
}

ini_t ini_parse_str(arena_t *arena, strview_t str, iniopt_t *opt) {
    ini_t out = {
        .text = str,
        .tables = NULL,
    };
    ini__parse(arena, &out, opt);
    return out;
}

bool ini_is_valid(ini_t *ini) {
    return ini && !strv_is_empty(ini->text);
}

initable_t *ini_get_table(ini_t *ini, strview_t name) {
    initable_t *t = ini ? ini->tables : NULL;
    while (t) {
        if (strv_equals(t->name, name)) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

inivalue_t *ini_get(initable_t *table, strview_t key) {
    inivalue_t *v = table ? table->values : NULL;
    while (v) {
        if (strv_equals(v->key, key)) {
            return v;
        }
        v = v->next;
    }
    return NULL;
}

iniarray_t ini_as_arr(arena_t *arena, inivalue_t *value, char delim) {
    strview_t v = value ? value->value : STRV_EMPTY;
    if (!delim) delim = ' ';

    strview_t *beg = (strview_t *)arena->cur;
    usize count = 0;

    usize start = 0;
    for (usize i = 0; i < v.len; ++i) {
        if (v.buf[i] == delim) {
            strview_t arrval = strv_trim(strv_sub(v, start, i));
            if (!strv_is_empty(arrval)) {
                strview_t *newval = alloc(arena, strview_t);
                *newval = arrval;
                ++count;
            }
            start = i + 1;
        }
    }

    strview_t last = strv_trim(strv_sub(v, start, SIZE_MAX));
    if (!strv_is_empty(last)) {
        strview_t *newval = alloc(arena, strview_t);
        *newval = last;
        ++count;
    }

    return (iniarray_t){
        .values = beg,
        .count = count,
    };
}

u64 ini_as_uint(inivalue_t *value) {
    strview_t v = value ? value->value : STRV_EMPTY;
    instream_t in = istr_init(v);
    u64 out = 0;
    if (!istr_get_u64(&in, &out)) {
        out = 0;
    }
    return out;
}

i64 ini_as_int(inivalue_t *value) {
    strview_t v = value ? value->value : STRV_EMPTY;
    instream_t in = istr_init(v);
    i64 out = 0;
    if (!istr_get_i64(&in, &out)) {
        out = 0;
    }
    return out;
}

double ini_as_num(inivalue_t *value) {
    strview_t v = value ? value->value : STRV_EMPTY;
    instream_t in = istr_init(v);
    double out = 0;
    if (!istr_get_num(&in, &out)) {
        out = 0;
    }
    return out;
}

bool ini_as_bool(inivalue_t *value) {
    strview_t v = value ? value->value : STRV_EMPTY;
    instream_t in = istr_init(v);
    bool out = 0;
    if (!istr_get_bool(&in, &out)) {
        out = 0;
    }
    return out;
}

///// ini-private ////////////////////////////////////

iniopt_t ini__get_options(const iniopt_t *options) {
    iniopt_t out = {
        .key_value_divider = '=',
        .comment_vals = strv(";#"),
    };

#define SETOPT(v) out.v = options->v ? options->v : out.v

    if (options) {
        SETOPT(key_value_divider);
        SETOPT(merge_duplicate_keys);
        SETOPT(merge_duplicate_tables);
        out.comment_vals = strv_is_empty(options->comment_vals) ? out.comment_vals : options->comment_vals;
    }

#undef SETOPT

    return out;
}


void ini__add_value(arena_t *arena, initable_t *table, instream_t *in, iniopt_t *opts) {
    assert(table);

    strview_t key = strv_trim(istr_get_view(in, opts->key_value_divider));
    istr_skip(in, 1);

    strview_t value = strv_trim(istr_get_view(in, '\n'));
    usize comment_pos = strv_find_either(value, opts->comment_vals, 0);
    if (comment_pos != STR_NONE) {
        value = strv_sub(value, 0, comment_pos);
    }
    istr_skip(in, 1);
    inivalue_t *newval = NULL;
    
    if (opts->merge_duplicate_keys) {
        newval = table->values;
        while (newval) {
            if (strv_equals(newval->key, key)) {
                break;
            }
            newval = newval->next;
        }
    }

    if (newval) {
        newval->value = value;
    }
    else {
        newval = alloc(arena, inivalue_t);
        newval->key = key;
        newval->value = value;

        if (!table->values) {
            table->values = newval;
        }
        else {
            table->tail->next = newval;
        }

        table->tail = newval;
    }
}

void ini__add_table(arena_t *arena, ini_t *ctx, instream_t *in, iniopt_t *options) {
    istr_skip(in, 1); // skip [
    strview_t name = istr_get_view(in, ']');
    istr_skip(in, 1); // skip ]
    initable_t *table = NULL;

    if (options->merge_duplicate_tables) {
        table = ctx->tables;
        while (table) {
            if (strv_equals(table->name, name)) {
                break;
            }
            table = table->next;
        }
    }

    if (!table) {
        table = alloc(arena, initable_t);

        table->name = name;

        if (!ctx->tables) {
            ctx->tables = table;
        }
        else {
            ctx->tail->next = table;
        }

        ctx->tail = table;
    }

    istr_ignore_and_skip(in, '\n');
    while (!istr_is_finished(in)) {
        switch (istr_peek(in)) {
            case '\n': // fallthrough
            case '\r':
                return;
            case '#':  // fallthrough
            case ';':
                istr_ignore_and_skip(in, '\n');
                break;
            default:
                ini__add_value(arena, table, in, options);
                break;
        }
    }
}

void ini__parse(arena_t *arena, ini_t *ini, const iniopt_t *options) {
    iniopt_t opts = ini__get_options(options);

    initable_t *root = alloc(arena, initable_t);
    root->name = INI_ROOT;
    ini->tables = root;
    ini->tail = root;

    instream_t in = istr_init(ini->text);

    while (!istr_is_finished(&in)) {
        istr_skip_whitespace(&in);
        switch (istr_peek(&in)) {
            case '[':
                ini__add_table(arena, ini, &in, &opts);
                break;
            case '#': // fallthrough
            case ';':
                istr_ignore_and_skip(&in, '\n');
                break;
            default:
                ini__add_value(arena, ini->tables, &in, &opts);
                break;
        }
    }
}

// == JSON ===========================================

bool json__parse_obj(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out);

json_t *json_parse(arena_t *arena, strview_t filename, jsonflags_e flags) {
    str_t data = os_file_read_all_str(arena, filename);
    return json_parse_str(arena, strv(data), flags);
}

json_t *json_parse_str(arena_t *arena, strview_t str, jsonflags_e flags) {
    arena_t before = *arena;

    json_t *root = alloc(arena, json_t);
    root->type = JSON_OBJECT;

    instream_t in = istr_init(str);

    if (!json__parse_obj(arena, &in, flags, &root->object)) {
        // reset arena
        *arena = before;
        return NULL;
    }

    return root;
}

json_t *json_get(json_t *node, strview_t key) {
    if (!node) return NULL;

    if (node->type != JSON_OBJECT) {
        err("passed type is not an object");
        return NULL;
    }

    node = node->object;

    while (node) {
        if (strv_equals(node->key, key)) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

void json__pretty_print_value(json_t *value, int indent, const json_pretty_opts_t *options);

void json_pretty_print(json_t *root, const json_pretty_opts_t *options) {
    json_pretty_opts_t default_options = { 0 };
    if (options) {
        memmove(&default_options, options, sizeof(json_pretty_opts_t));
    }

    if (!os_handle_valid(default_options.custom_target)) {
        default_options.custom_target = os_stdout();
    }
    if (!default_options.use_custom_colours) {
        os_log_colour_e default_col[JSON_PRETTY_COLOUR__COUNT] = {
            LOG_COL_YELLOW, // JSON_PRETTY_COLOUR_KEY,
            LOG_COL_CYAN,   // JSON_PRETTY_COLOUR_STRING,
            LOG_COL_BLUE,   // JSON_PRETTY_COLOUR_NUM,
            LOG_COL_BLACK,  // JSON_PRETTY_COLOUR_NULL,
            LOG_COL_GREEN,  // JSON_PRETTY_COLOUR_TRUE,
            LOG_COL_RED,    // JSON_PRETTY_COLOUR_FALSE,
        };
        memmove(default_options.colours, default_col, sizeof(default_col));
    }

    json__pretty_print_value(root, 0, &default_options);
    os_file_putc(default_options.custom_target, '\n');
}

///// json-private ///////////////////////////////////

#define json__ensure(c) json__check_char(in, c)

bool json__parse_value(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out);

bool json__check_char(instream_t *in, char c) {
    if (istr_get(in) == c) {
        return true;
    }
    istr_rewind_n(in, 1);
    err("wrong character at %zu, should be '%c' but is 0x%02x '%c'", istr_tell(in), c, istr_peek(in), istr_peek(in));
    return false;
}

bool json__is_value_finished(instream_t *in) {
    usize old_pos = istr_tell(in);
    
    istr_skip_whitespace(in);
    switch(istr_peek(in)) {
        case '}': // fallthrough
        case ']': // fallthrough
        case ',':
            return true;
    }

    in->cur = in->beg + old_pos;
    return false;
}

bool json__parse_null(instream_t *in) {
    strview_t null_view = istr_get_view_len(in, 4);
    bool is_valid = true;
    
    if (!strv_equals(null_view, strv("null"))) {
        err("should be null but is: (%.*s) at %zu", null_view.len, null_view.buf, istr_tell(in));
        is_valid = false;
    }

    if (!json__is_value_finished(in)) {
        err("null, should be finished, but isn't at %zu", istr_tell(in));
        is_valid = false;
    }

    return is_valid;
}

bool json__parse_array(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out) {
    json_t *head = NULL;
    
    if (!json__ensure('[')) {
        goto fail;
    }

    istr_skip_whitespace(in);

    // if it is an empty array
    if (istr_peek(in) == ']') {
        istr_skip(in, 1);
        goto success;
    }
    
    if (!json__parse_value(arena, in, flags, &head)) {
        goto fail;
    }

    json_t *cur = head;
    
    while (true) {
        istr_skip_whitespace(in);
        switch (istr_get(in)) {
            case ']':
                goto success;
            case ',':
            {
                istr_skip_whitespace(in);
                // trailing comma
                if (istr_peek(in) == ']') {
                    if (flags & JSON_NO_TRAILING_COMMAS) {
                        err("trailing comma in array at at %zu: (%c)(%d)", istr_tell(in), *in->cur, *in->cur);
                        goto fail;
                    }
                    else {
                        continue;
                    }
                }

                json_t *next = NULL;
                if (!json__parse_value(arena, in, flags, &next)) {
                    goto fail;
                }
                cur->next = next;
                next->prev = cur;
                cur = next;
                break;
            }
            default:
                istr_rewind_n(in, 1);
                err("unknown char after array at %zu: (%c)(%d)", istr_tell(in), *in->cur, *in->cur);
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

bool json__parse_string(arena_t *arena, instream_t *in, strview_t *out) {
    COLLA_UNUSED(arena);
    *out = STRV_EMPTY;

    istr_skip_whitespace(in); 

    if (!json__ensure('"')) {
        goto fail;
    }

    const char *from = in->cur;
    
    for (; !istr_is_finished(in) && *in->cur != '"'; ++in->cur) {
        if (istr_peek(in) == '\\') {
            ++in->cur;
        }
    }
    
    usize len = in->cur - from;

    *out = strv(from, len);

    if (!json__ensure('"')) {
        goto fail;
    }

    return true;
fail:
    return false;
}

bool json__parse_pair(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out) {
    strview_t key = {0};
    if (!json__parse_string(arena, in, &key)) {
        goto fail;
    }

    // skip preamble
    istr_skip_whitespace(in);
    if (!json__ensure(':')) {
        goto fail;
    }

    if (!json__parse_value(arena, in, flags, out)) {
        goto fail;
    }
    
    (*out)->key = key;
    return true;

fail: 
    *out = NULL;
    return false;
}

bool json__parse_obj(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out) {
    if (!json__ensure('{')) {
        goto fail;
    }

    istr_skip_whitespace(in);

    // if it is an empty object
    if (istr_peek(in) == '}') {
        istr_skip(in, 1);
        *out = NULL;
        return true;
    }

    json_t *head = NULL;
    if (!json__parse_pair(arena, in, flags, &head)) {
        goto fail;
    }
    json_t *cur = head;

    while (true) {
        istr_skip_whitespace(in);
        switch (istr_get(in)) {
            case '}':
                goto success;
            case ',':
            {
                istr_skip_whitespace(in);
                // trailing commas
                if (!(flags & JSON_NO_TRAILING_COMMAS) && istr_peek(in) == '}') {
                    goto success;
                }

                json_t *next = NULL;
                if (!json__parse_pair(arena, in, flags, &next)) {
                    goto fail;
                }
                cur->next = next;
                next->prev = cur;
                cur = next;
                break;
            }
            default:
                istr_rewind_n(in, 1);
                err("unknown char after object at %zu: (%c)(%d)", istr_tell(in), *in->cur, *in->cur);
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

bool json__parse_value(arena_t *arena, instream_t *in, jsonflags_e flags, json_t **out) {
    json_t *val = alloc(arena, json_t);

    istr_skip_whitespace(in);

    switch (istr_peek(in)) {
        // object
        case '{':
            if (!json__parse_obj(arena, in, flags, &val->object)) {
                goto fail;
            }
            val->type = JSON_OBJECT;
            break;
        // array
        case '[':
            if (!json__parse_array(arena, in, flags, &val->array)) {
                goto fail;
            }
            val->type = JSON_ARRAY;
            break;
        // string
        case '"':
            if (!json__parse_string(arena, in, &val->string)) {
                goto fail;
            }
            val->type = JSON_STRING;
            break;
        // boolean
        case 't': // fallthrough
        case 'f':
            if (!istr_get_bool(in, &val->boolean)) {
                goto fail;
            }
            val->type = JSON_BOOL;
            break;
        // null
        case 'n': 
            if (!json__parse_null(in)) {
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
            if (!istr_get_num(in, &val->number)) {
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

#undef json__ensure

#define JSON_PRETTY_INDENT(ind) for (int i = 0; i < ind; ++i) os_file_puts(options->custom_target, strv("    "))

void json__pretty_print_value(json_t *value, int indent, const json_pretty_opts_t *options) {
    switch (value->type) {
        case JSON_NULL:
            os_log_set_colour(options->colours[JSON_PRETTY_COLOUR_NULL]);
            os_file_puts(options->custom_target, strv("null"));
            os_log_set_colour(LOG_COL_RESET);
            break;
        case JSON_ARRAY:
            os_file_puts(options->custom_target, strv("[\n"));
            for_each (node, value->array) {
                JSON_PRETTY_INDENT(indent + 1);
                json__pretty_print_value(node, indent + 1, options);
                if (node->next) {
                    os_file_putc(options->custom_target, ',');
                }
                os_file_putc(options->custom_target, '\n');
            }
            JSON_PRETTY_INDENT(indent);
            os_file_putc(options->custom_target, ']');
            break;
        case JSON_STRING: 
            os_log_set_colour(options->colours[JSON_PRETTY_COLOUR_STRING]);
            os_file_putc(options->custom_target, '\"');
            os_file_puts(options->custom_target, value->string);
            os_file_putc(options->custom_target, '\"');
            os_log_set_colour(LOG_COL_RESET);
            break;
        case JSON_NUMBER:
        {
            os_log_set_colour(options->colours[JSON_PRETTY_COLOUR_NUM]);
            u8 scratchbuf[256];
            arena_t scratch = arena_make(ARENA_STATIC, sizeof(scratchbuf), scratchbuf);
            os_file_print(
                scratch, 
                options->custom_target, 
                "%g", 
                value->number
            );
            os_log_set_colour(LOG_COL_RESET);
            break;
        } 
        case JSON_BOOL:
            os_log_set_colour(options->colours[value->boolean ? JSON_PRETTY_COLOUR_TRUE : JSON_PRETTY_COLOUR_FALSE]);
            os_file_puts(options->custom_target, value->boolean ? strv("true") : strv("false"));
            os_log_set_colour(LOG_COL_RESET);
            break;
        case JSON_OBJECT:
            os_file_puts(options->custom_target, strv("{\n"));
            for_each(node, value->object) {
                JSON_PRETTY_INDENT(indent + 1);
                os_log_set_colour(options->colours[JSON_PRETTY_COLOUR_KEY]);
                os_file_putc(options->custom_target, '\"');
                os_file_puts(options->custom_target, node->key);
                os_file_putc(options->custom_target, '\"');
                os_log_set_colour(LOG_COL_RESET);

                os_file_puts(options->custom_target, strv(": "));

                json__pretty_print_value(node, indent + 1, options);
                if (node->next) {
                    os_file_putc(options->custom_target, ',');
                }
                os_file_putc(options->custom_target, '\n');
            }
            JSON_PRETTY_INDENT(indent);
            os_file_putc(options->custom_target, '}');
            break;
    }
}

#undef JSON_PRETTY_INDENT


// == XML ============================================

xmltag_t *xml__parse_tag(arena_t *arena, instream_t *in);

xml_t xml_parse(arena_t *arena, strview_t filename) {
    str_t str = os_file_read_all_str(arena, filename);
    return xml_parse_str(arena, strv(str));
}

xml_t xml_parse_str(arena_t *arena, strview_t xmlstr) {
    xml_t out = {
        .text = xmlstr,
        .root = alloc(arena, xmltag_t),
    };
    
    instream_t in = istr_init(xmlstr);

    while (!istr_is_finished(&in)) {
        xmltag_t *tag = xml__parse_tag(arena, &in);

        if (out.tail) out.tail->next = tag;
        else          out.root->child = tag;

        out.tail = tag;
    }

    return out;
}

xmltag_t *xml_get_tag(xmltag_t *parent, strview_t key, bool recursive) {
    xmltag_t *t = parent ? parent->child : NULL;
    while (t) {
        if (strv_equals(key, t->key)) {
            return t;
        }
        if (recursive && t->child) {
            xmltag_t *out = xml_get_tag(t, key, recursive);
            if (out) {
                return out;
            }
        }
        t = t->next;
    }
    return NULL;
}

strview_t xml_get_attribute(xmltag_t *tag, strview_t key) {
    xmlattr_t *a = tag ? tag->attributes : NULL;
    while (a) {
        if (strv_equals(key, a->key)) {
            return a->value;
        }
        a = a->next;
    }
    return STRV_EMPTY;
}

///// xml-private ////////////////////////////////////

xmlattr_t *xml__parse_attr(arena_t *arena, instream_t *in) {
    if (istr_peek(in) != ' ') {
        return NULL;
    }

    strview_t key = strv_trim(istr_get_view(in, '='));
    istr_skip(in, 2); // skip = and "
    strview_t val = strv_trim(istr_get_view(in, '"'));
    istr_skip(in, 1); // skip "

    if (strv_is_empty(key) || strv_is_empty(val)) {
        warn("key or value empty");
        return NULL;
    }
    
    xmlattr_t *attr = alloc(arena, xmlattr_t);
    attr->key = key;
    attr->value = val;
    return attr;
}

xmltag_t *xml__parse_tag(arena_t *arena, instream_t *in) {
    istr_skip_whitespace(in);

    // we're either parsing the body, or we have finished the object
    if (istr_peek(in) != '<' || istr_peek_next(in) == '/') {
        return NULL;
    }

    istr_skip(in, 1); // skip <

    // meta tag, we don't care about these
    if (istr_peek(in) == '?') {
        istr_ignore_and_skip(in, '\n');
        return NULL;
    }

    xmltag_t *tag = alloc(arena, xmltag_t);

    tag->key = strv_trim(istr_get_view_either(in, strv(" >")));

    xmlattr_t *attr = xml__parse_attr(arena, in);
    while (attr) {
        attr->next = tag->attributes;
        tag->attributes = attr;
        attr = xml__parse_attr(arena, in);
    }

    // this tag does not have children, return
    if (istr_peek(in) == '/') {
        istr_skip(in, 2); // skip / and >
        return tag;
    }

    istr_skip(in, 1); // skip >

    xmltag_t *child = xml__parse_tag(arena, in);
    while (child) {
        if (tag->tail) {
            tag->tail->next = child;
            tag->tail = child;
        }
        else {
            tag->child = tag->tail = child;
        }
        child = xml__parse_tag(arena, in);
    }

    // parse content
    istr_skip_whitespace(in);
    tag->content = istr_get_view(in, '<');

    // closing tag
    istr_skip(in, 2); // skip < and /
    strview_t closing = strv_trim(istr_get_view(in, '>'));
    if (!strv_equals(tag->key, closing)) {
        warn("opening and closing tags are different!: (%v) != (%v)", tag->key, closing);
    }
    istr_skip(in, 1); // skip >
    return tag;
}
