#include "highlight.h"

// based on https://github.com/Theldus/kat

#include <ctype.h>

#include "arena.h"
#include "tracelog.h"
#include "strstream.h"

typedef enum {
    HL_STATE_DEFAULT,
    HL_STATE_KEYWORD,
    HL_STATE_NUMBER,
    HL_STATE_CHAR,
    HL_STATE_STRING,
    HL_STATE_COMMENT_MULTI,
    HL_STATE_PREPROCESSOR,
    HL_STATE_PREPROCESSOR_INCLUDE,
    HL_STATE_PREPROCESSOR_INCLUDE_STRING,
} hl_state_e;

typedef enum {
    HL_HTABLE_FAILED,
    HL_HTABLE_REPLACED,
    HL_HTABLE_ADDED,
} hl_htable_result_e;

typedef struct hl_node_t {
    strview_t key;
    hl_color_e value;
    struct hl_node_t *next;
} hl_node_t;

typedef struct {
    hl_node_t **buckets;
    uint count;
    uint used;
    uint collisions;
} hl_hashtable_t;

static hl_hashtable_t hl_htable_init(arena_t *arena, uint pow2_exp);
static hl_htable_result_e hl_htable_add(arena_t *arena, hl_hashtable_t *table, strview_t key, hl_color_e value);
static hl_node_t *hl_htable_get(hl_hashtable_t *table, strview_t key);
static uint64 hl_htable_hash(const void *bytes, usize count);

typedef struct hl_ctx_t {
    hl_state_e state;
    hl_flags_e flags;
    usize kw_beg;
    strview_t colors[HL_COLOR__COUNT]; // todo: maybe should be str_t?
    outstream_t ostr;
    hl_hashtable_t kw_htable;
    bool symbol_table[256];
} hl_ctx_t;

#define KW(str, col) { { str, sizeof(str)-1 }, HL_COLOR_##col }

static hl_keyword_t hl_default_kwrds[] = {
	/* C Types. */
	KW("double",   TYPES), 
    KW("int",      TYPES), 
    KW("long",     TYPES),
	KW("char",     TYPES), 
    KW("float",    TYPES), 
    KW("short",    TYPES),
	KW("unsigned", TYPES), 
    KW("signed",   TYPES),
    KW("bool",     TYPES),

	/* Common typedefs. */
	KW("int8",    TYPES), KW("uint8",    TYPES),
	KW("int16",   TYPES), KW("uint16",   TYPES),
	KW("int32",   TYPES), KW("uint32",   TYPES),
	KW("int64",   TYPES), KW("uint64",   TYPES),

	/* Colla keywords */
	KW("uchar",  TYPES), 
	KW("ushort", TYPES), 
	KW("uint",   TYPES), 
	KW("usize",  TYPES), 
	KW("isize",  TYPES), 
	KW("byte",   TYPES), 

	/* Other keywords. */
	KW("auto",    KEYWORDS), KW("struct",   KEYWORDS), KW("break",   KEYWORDS),
	KW("else",    KEYWORDS), KW("switch",   KEYWORDS), KW("case",    KEYWORDS),
	KW("enum",    KEYWORDS), KW("register", KEYWORDS), KW("typedef", KEYWORDS),
	KW("extern",  KEYWORDS), KW("return",   KEYWORDS), KW("union",   KEYWORDS),
	KW("const",   KEYWORDS), KW("continue", KEYWORDS), KW("for",     KEYWORDS),
	KW("void",    KEYWORDS), KW("default",  KEYWORDS), KW("goto",    KEYWORDS),
	KW("sizeof",  KEYWORDS), KW("volatile", KEYWORDS), KW("do",      KEYWORDS),
	KW("if",      KEYWORDS), KW("static",   KEYWORDS), KW("inline",  KEYWORDS),
	KW("while",   KEYWORDS),
};

#undef KW

static bool hl_default_symbols_table[256] = {
	['['] = true, [']'] = true, ['('] = true, 
    [')'] = true, ['{'] = true, ['}'] = true,
	['*'] = true, [':'] = true, ['='] = true, 
    [';'] = true, ['-'] = true, ['>'] = true,
	['&'] = true, ['+'] = true, ['~'] = true, 
    ['!'] = true, ['/'] = true, ['%'] = true,
	['<'] = true, ['^'] = true, ['|'] = true, 
    ['?'] = true, ['#'] = true,
};

static void hl_write_char(hl_ctx_t *ctx, char c);
static void hl_write(hl_ctx_t *ctx, strview_t v);
static bool hl_is_char_keyword(char c);
static bool hl_highlight_symbol(hl_ctx_t *ctx, char c);
static hl_color_e hl_get_keyword_color(hl_ctx_t *ctx, strview_t keyword);
static bool hl_is_capitalised(strview_t string);
static strview_t hl_finish_keyword(hl_ctx_t *ctx, usize beg, instream_t *in);
static void hl_print_keyword(hl_ctx_t *ctx, strview_t keyword, hl_color_e color);

hl_ctx_t *hlInit(arena_t *arena, hl_config_t *config) {
    if (!config) {
        err("<config> cannot be null");
        return NULL;
    }

    hl_ctx_t *out = alloc(arena, hl_ctx_t);

    out->flags = config->flags;
    
    memcpy(out->symbol_table, hl_default_symbols_table, sizeof(hl_default_symbols_table));
    memcpy(out->colors, config->colors, sizeof(config->colors));
    
    int kw_count = arrlen(hl_default_kwrds);

    out->kw_htable = hl_htable_init(arena, 8);

    for (int i = 0; i < kw_count; ++i) {
        hl_keyword_t *kw = &hl_default_kwrds[i];
        hl_htable_add(arena, &out->kw_htable, kw->keyword, kw->color);
    }

    for (int i = 0; i < config->kwrds_count; ++i) {
        hl_keyword_t *kw = &config->extra_kwrds[i];
        hl_htable_add(arena, &out->kw_htable, kw->keyword, kw->color);
    }

    return out;
}

void hl_next_char(hl_ctx_t *ctx, instream_t *in) {
    char cur = istrGet(in);
    bool is_last = istrIsFinished(*in);

    switch (ctx->state) {
        case HL_STATE_DEFAULT:
        {
            /*
            * If potential keyword.
            *
            * A valid C keyword may contain numbers, but *not*
            * as a suffix.
            */
            if (hl_is_char_keyword(cur) && !isdigit(cur)) {
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_KEYWORD;
            }

            // potential number
            else if (isdigit(cur)) {
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_NUMBER;
            }

            // potential char
            else if (cur == '\'') {
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_CHAR;
            }
            
            // potential string
            else if (cur == '"') {
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_STRING;
            }
            
            // line or multiline comment
            else if (cur == '/') {
                // single line comment
                if (istrPeek(in) == '/') {
                    // rewind before comment begins
                    istrRewindN(in, 1);

                    // comment until the end of line
                    hl_print_keyword(ctx, istrGetLine(in), HL_COLOR_COMMENT);
                }

                // multiline comment
                else if (istrPeek(in) == '*') {
                    ctx->state = HL_STATE_COMMENT_MULTI;
                    ctx->kw_beg = istrTell(*in);
                    istrSkip(in, 1); // skip *
                }

                else {
                    // maybe a symbol?
                    hl_highlight_symbol(ctx, cur);
                }
            }

            // preprocessor
            else if (cur == '#') {
                // print the # as a symbol
                hl_highlight_symbol(ctx, cur);
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_PREPROCESSOR;
            }

            // other suppored symbols
            else if (hl_highlight_symbol(ctx, cur)) {
                // noop
            }

            else {
                hl_write_char(ctx, cur);
            }

            break;
        }
        
        case HL_STATE_KEYWORD:
        {
            // end of keyword, check if it really is a valid keyword
            if (!hl_is_char_keyword(cur)) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);
                hl_color_e kw_color = hl_get_keyword_color(ctx, keyword);

                if (kw_color != HL_COLOR__COUNT) {
                    hl_print_keyword(ctx, keyword, kw_color);
                    
                    // maybe we should highlight this remaining char.
                    if (!hl_highlight_symbol(ctx, cur)) {
                        hl_write_char(ctx, cur);
                    }
                }
                
                /*
                    * If not keyword, maybe its a function call.
                    *
                    * Important to note that this is hacky and will only work
                    * if there is no space between keyword and '('.
                    */
                else if (cur == '(') {
                    hl_print_keyword(ctx, keyword, HL_COLOR_FUNC);
                    
                    // Opening parenthesis will always be highlighted
                    hl_highlight_symbol(ctx, cur);
                }
                else {
                    if (hl_is_capitalised(keyword)) {
                        hl_print_keyword(ctx, keyword, HL_COLOR_MACRO);
                    }
                    else {
                        hl_write(ctx, keyword);
                    }
                    if (!hl_highlight_symbol(ctx, cur)) {
                        hl_write_char(ctx, cur);
                    }
                }
            }
            break;
        }

        case HL_STATE_NUMBER:
        {
            char c = (char)tolower(cur);
            
            /*
                * Should we end the state?.
                *
                * Very important observation:
                * Although the number highlight works fine for most (if not all)
                * of the possible cases, it also assumes that the code is written
                * correctly and the source is able to compile, meaning that:
                *
                * Numbers like: 123, 0xABC123, 12.3e4f, 123ULL....
                * will be correctly identified and highlighted
                *
                * But, 'numbers' like: 123ABC, 0xxxxABCxx123, 123UUUUU....
                * will also be highlighted.
                *
                * It also assumes that no keyword will start with a number
                * and everything starting with a number (except inside strings or
                * comments) will be a number.
                */
            if (!isdigit(c) && 
                (c < 'a' || c > 'f') && 
                c != 'b' && c != 'x' && 
                c != 'u' && c != 'l' && 
                c != '.'
            ) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);

                // if not a valid char keyword: valid number
                if (!hl_is_char_keyword(cur)) {
                    hl_print_keyword(ctx, keyword, HL_COLOR_NUMBER);
                }
                else {
                    hl_write(ctx, keyword);
                }

                // maybe we should highlight this remaining char.
                if (!hl_highlight_symbol(ctx, cur)) {
                    hl_write_char(ctx, cur);
                }
            }

            break;
        }

        case HL_STATE_CHAR:
        {
            if (is_last || (cur == '\'' && istrPeek(in) != '\'')) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);
                keyword.len++;

                hl_print_keyword(ctx, keyword, HL_COLOR_STRING);
            }
            break;
        }

        case HL_STATE_STRING:
        {
            if (is_last || (cur == '"' && istrPrevPrev(in) != '\\')) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);
                keyword.len++;

                hl_print_keyword(ctx, keyword, HL_COLOR_STRING);
            }
            break;
        }

        case HL_STATE_COMMENT_MULTI:
        {
            /*
                * If we are at the end of line _or_ have identified
                * an end of comment...
                */
            if (is_last || (cur == '*' && istrPeek(in) == '/')) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);

                hl_print_keyword(ctx, keyword, HL_COLOR_COMMENT);
            }
            break;
        }

        case HL_STATE_PREPROCESSOR:
        {
            
            if (!hl_is_char_keyword(cur)) {
                hl_write_char(ctx, cur);
                break;
            }

#define hl_check(str, new_state) \
    if (cur == str[0]) { \
        instream_t temp = *in; \
        strview_t a = strvInitLen(&(str[1]), sizeof(str) - 2); \
        strview_t b = istrGetViewLen(&temp, a.len); \
        if (strvEquals(a, b)) { \
            *in = temp; \
            hl_print_keyword(ctx, strvInitLen(str, sizeof(str) - 1), HL_COLOR_PREPROC); \
            ctx->state = new_state; \
            break; \
        } \
    }   
            if (is_last) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);
                hl_print_keyword(ctx, keyword, HL_COLOR_PREPROC);
                break;
            }

            hl_check("include", HL_STATE_PREPROCESSOR_INCLUDE)
            hl_check("define",  HL_STATE_DEFAULT)
            hl_check("undef",   HL_STATE_DEFAULT)
            hl_check("ifdef",   HL_STATE_DEFAULT)
            hl_check("ifndef",  HL_STATE_DEFAULT)
            hl_check("if",   HL_STATE_DEFAULT)
            hl_check("endif",   HL_STATE_DEFAULT)
            hl_check("pragma",  HL_STATE_DEFAULT)

#undef hl_check
            break;
        }

        
        /*
            * Preprocessor/Preprocessor include
            *
            * This is a 'dumb' preprocessor highlighter:
            * it highlights everything with the same color
            * and if and only if an '#include' is detected
            * the included header will be handled as string
            * and thus, will have the same color as the string.
            *
            * In fact, it is somehow similar to what GtkSourceView
            * does (Mousepad, Gedit...) but with one silly difference:
            * single-line/multi-line comments will not be handled
            * while inside the preprocessor state, meaning that
            * comments will also have the same color as the remaining
            * of the line, yeah, ugly.
            */
        case HL_STATE_PREPROCESSOR_INCLUDE:
        {
            if (cur == '<' || cur == '"' || is_last) {
                ctx->kw_beg = istrTell(*in);
                ctx->state = HL_STATE_PREPROCESSOR_INCLUDE_STRING;
            }
            else {
                hl_write_char(ctx, cur);
            }
            break;
        }
        case HL_STATE_PREPROCESSOR_INCLUDE_STRING:
        {
            if (cur == '>' || cur == '"' || is_last) {
                strview_t keyword = hl_finish_keyword(ctx, ctx->kw_beg, in);
                keyword.len += 1;
                hl_print_keyword(ctx, keyword, HL_COLOR_STRING);
            }
            break;
        }
    }
}

str_t hlHighlight(arena_t *arena, hl_ctx_t *ctx, strview_t data) {
    ctx->ostr = ostrInit(arena);

    ctx->state = HL_STATE_DEFAULT;
    ctx->kw_beg = 0;

    instream_t in = istrInitLen(data.buf, data.len);

    while (!istrIsFinished(in)) {
        hl_next_char(ctx, &in);
    }

    hl_next_char(ctx, &in);

    return ostrAsStr(&ctx->ostr);
}

void hlSetSymbolInTable(hl_ctx_t *ctx, char symbol, bool value) {
    if (!ctx) return;
    ctx->symbol_table[(unsigned char)symbol] = value;
}

void hlAddKeyword(arena_t *arena, hl_ctx_t *ctx, hl_keyword_t *keyword) {
    hl_htable_add(arena, &ctx->kw_htable, keyword->keyword, keyword->color);
}

//// HASH TABLE ///////////////////////////////////////////////////

static hl_hashtable_t hl_htable_init(arena_t *arena, uint pow2_exp) {
    uint count = 1 << pow2_exp;
    return (hl_hashtable_t) {
        .count = count,
        .buckets = alloc(arena, hl_node_t*, count),
    };
}

static hl_htable_result_e hl_htable_add(arena_t *arena, hl_hashtable_t *table, strview_t key, hl_color_e value) {
    if (!table) {
        return HL_HTABLE_FAILED;
    }

    if ((float)table->used >= table->count * 0.6f) {
        warn("more than 60%% of the arena is being used: %d/%d", table->used, table->count);
    }

    uint64 hash = hl_htable_hash(key.buf, key.len);
    usize index = hash & (table->count - 1);
    hl_node_t *bucket = table->buckets[index];
    if (bucket) table->collisions++;
    while (bucket) {
        // already exists
        if (strvEquals(bucket->key, key)) {
            bucket->value = value;
            return HL_HTABLE_REPLACED;
        }
        bucket = bucket->next;
    }

    bucket = alloc(arena, hl_node_t);

    bucket->key   = key;
    bucket->value = value;
    bucket->next  = table->buckets[index];

    table->buckets[index] = bucket;
    table->used++;

    return HL_HTABLE_ADDED;
}

static hl_node_t *hl_htable_get(hl_hashtable_t *table, strview_t key) {
    if (!table || table->count == 0) {
        return NULL;
    }
    
    uint64 hash = hl_htable_hash(key.buf, key.len);
    usize index = hash & (table->count - 1);
    hl_node_t *bucket = table->buckets[index];
    while (bucket) {
        if (strvEquals(bucket->key, key)) {
            return bucket;
        }
        bucket = bucket->next;
    }

    return NULL;
}

// uses the sdbm algorithm
static uint64 hl_htable_hash(const void *bytes, usize count) {
    const uint8 *data = bytes;
    uint64 hash = 0;

    for (usize i = 0; i < count; ++i) {
		hash = data[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

//// STATIC FUNCTIONS /////////////////////////////////////////////

static inline void hl_escape_html(outstream_t *out, char c) {
    switch (c) {
        case '&':
            ostrPuts(out, strv("&amp"));
            break;
        case '<':
            ostrPuts(out, strv("&lt"));
            break;
        case '>':
            ostrPuts(out, strv("&gt"));
            break;
        default:
            ostrPutc(out, c);
            break;
    }
}

static void hl_write_char(hl_ctx_t *ctx, char c) {
    if (ctx->flags & HL_FLAG_HTML) {
        hl_escape_html(&ctx->ostr, c);
    }
    else {
        ostrPutc(&ctx->ostr, c);
    }
}

static void hl_write(hl_ctx_t *ctx, strview_t v) {
    if (ctx->flags & HL_FLAG_HTML) {
        for (usize i = 0; i < v.len; ++i) {
            hl_escape_html(&ctx->ostr, v.buf[i]);
        }
    }
    else {
        ostrPuts(&ctx->ostr, v);
    }
}

static bool hl_is_char_keyword(char c) {
    return isalpha(c) || isdigit(c) || c == '_';
}

static bool hl_highlight_symbol(hl_ctx_t *ctx, char c) {
    if (!ctx->symbol_table[(unsigned char)c]) {
        return false;
    }

    ostrPuts(&ctx->ostr, ctx->colors[HL_COLOR_SYMBOL]);
    hl_write_char(ctx, c);
    ostrPuts(&ctx->ostr, ctx->colors[HL_COLOR_NORMAL]);

    return true;
}

static hl_color_e hl_get_keyword_color(hl_ctx_t *ctx, strview_t keyword) {
    // todo: make this an option?
    if (strvEndsWithView(keyword, strv("_t"))) {
        return HL_COLOR_CUSTOM_TYPES;
    }
    
    hl_node_t *node = hl_htable_get(&ctx->kw_htable, keyword);
    return node ? node->value : HL_COLOR__COUNT;
}

static bool hl_is_capitalised(strview_t string) {
    for (usize i = 0; i < string.len; ++i) {
        char c = string.buf[i];
        if (!isdigit(c) && c != '_' && (c < 'A' || c > 'Z')) {
            return false;
        }
    }
    return true;
}

static strview_t hl_finish_keyword(hl_ctx_t *ctx, usize beg, instream_t *in) {
    ctx->state = HL_STATE_DEFAULT;
    beg -= 1;
    usize end = istrTell(*in) - 1;

    return strv(in->start + beg, end - beg);
}

static void hl_print_keyword(hl_ctx_t *ctx, strview_t keyword, hl_color_e color) {
    ostrPuts(&ctx->ostr, ctx->colors[color]);
    hl_write(ctx, keyword);
    ostrPuts(&ctx->ostr, ctx->colors[HL_COLOR_NORMAL]);
}