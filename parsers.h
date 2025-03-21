#ifndef COLLA_PARSERS_H
#define COLLA_PARSERS_H

#include "core.h"
#include "os.h"
#include "str.h"

// == INI ============================================

typedef struct inivalue_t inivalue_t;
struct inivalue_t {
    strview_t key;
    strview_t value;
    inivalue_t *next;
};

typedef struct initable_t initable_t;
struct initable_t {
    strview_t name;
    inivalue_t *values;
    inivalue_t *tail;
    initable_t *next;
};

typedef struct ini_t ini_t;
struct ini_t {
    strview_t text;
    initable_t *tables;
    initable_t *tail;
};

typedef struct iniopt_t iniopt_t;
struct iniopt_t {
    bool merge_duplicate_tables; // default false
    bool merge_duplicate_keys;   // default false
    char key_value_divider;      // default =
    strview_t comment_vals;      // default ;#
};

typedef struct iniarray_t iniarray_t;
struct iniarray_t {
    strview_t *values;
    usize count;
};

#define INI_ROOT strv("__ROOT__")

ini_t ini_parse(arena_t *arena, strview_t filename, iniopt_t *opt);
ini_t ini_parse_fp(arena_t *arena, oshandle_t file, iniopt_t *opt);
ini_t ini_parse_str(arena_t *arena, strview_t str, iniopt_t *opt);

bool ini_is_valid(ini_t *ini);

initable_t *ini_get_table(ini_t *ini, strview_t name);
inivalue_t *ini_get(initable_t *table, strview_t key);

iniarray_t ini_as_arr(arena_t *arena, inivalue_t *value, char delim);
u64 ini_as_uint(inivalue_t *value);
i64 ini_as_int(inivalue_t *value);
double ini_as_num(inivalue_t *value);
bool ini_as_bool(inivalue_t *value);

// == JSON ===========================================

typedef enum jsontype_e {
    JSON_NULL,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_OBJECT,
} jsontype_e;

typedef enum jsonflags_e {
    JSON_DEFAULT            = 0,
    JSON_NO_TRAILING_COMMAS = 1 << 0,
    JSON_NO_COMMENTS        = 1 << 1,
} jsonflags_e;

typedef struct json_t json_t;
struct json_t {
    json_t *next;
    json_t *prev;

    strview_t key;

    union {
        json_t *array;
        strview_t string;
        double number;
        bool boolean;
        json_t *object;
    };
    jsontype_e type;
};

json_t *json_parse(arena_t *arena, strview_t filename, jsonflags_e flags);
json_t *json_parse_str(arena_t *arena, strview_t str, jsonflags_e flags);

json_t *json_get(json_t *node, strview_t key);

#define json_check(val, js_type) ((val) && (val)->type == js_type)
#define json_for(it, arr) for (json_t *it = json_check(arr, JSON_ARRAY) ? arr->array : NULL; it; it = it->next)

typedef enum json_pretty_colours_e {
    JSON_PRETTY_COLOUR_KEY,
    JSON_PRETTY_COLOUR_STRING,
    JSON_PRETTY_COLOUR_NUM,
    JSON_PRETTY_COLOUR_NULL,
    JSON_PRETTY_COLOUR_TRUE,
    JSON_PRETTY_COLOUR_FALSE,
    JSON_PRETTY_COLOUR__COUNT,
} json_pretty_colours_e;

typedef struct json_pretty_opts_t json_pretty_opts_t;
struct json_pretty_opts_t {
    oshandle_t custom_target;
    bool use_custom_colours;
    os_log_colour_e colours[JSON_PRETTY_COLOUR__COUNT];
};

void json_pretty_print(json_t *root, const json_pretty_opts_t *options);

// == XML ============================================

typedef struct xmlattr_t xmlattr_t;
struct xmlattr_t {
    strview_t key;
    strview_t value;
    xmlattr_t *next;
};

typedef struct xmltag_t xmltag_t;
struct xmltag_t {
    strview_t key;
    xmlattr_t *attributes;
    strview_t content;
    xmltag_t *child;
    xmltag_t *tail;
    xmltag_t *next;
};

typedef struct xml_t xml_t;
struct xml_t {
    strview_t text;
    xmltag_t *root;
    xmltag_t *tail;
};

xml_t xml_parse(arena_t *arena, strview_t filename);
xml_t xml_parse_str(arena_t *arena, strview_t xmlstr);

xmltag_t *xml_get_tag(xmltag_t *parent, strview_t key, bool recursive);
strview_t xml_get_attribute(xmltag_t *tag, strview_t key);

#endif