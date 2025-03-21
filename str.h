#ifndef COLLA_STR_H
#define COLLA_STR_H

#include <string.h> // strlen

#include "core.h"
#include "darr.h"

#define STR_NONE SIZE_MAX

typedef struct str_t str_t;
struct str_t {
    char *buf;
    usize len;
};

typedef struct str16_t str16_t;
struct str16_t {
    u16 *buf;
    usize len;
};

#if COLLA_UNICODE
typedef str16_t tstr_t;
#else
typedef str_t tstr_t;
#endif

typedef struct strview_t strview_t;
struct strview_t {
    const char *buf;
    usize len;
};

darr_define(str_list_t, str_t);
darr_define(strv_list_t, strview_t);

// == STR_T ========================================================

#define str__1(arena, x)               \
    _Generic((x),                      \
        const char *:    str_init,     \
        char *:          str_init,     \
        strview_t:       str_init_view \
    )(arena, x)

#define str__2(arena, cstr, clen) str_init_len(arena, cstr, clen)
#define str__impl(_1, _2, n, ...) str__##n

// either:
//     arena_t arena, [const] char *cstr, [usize len]
//     arena_t arena, strview_t view
#define str(arena, ...) str__impl(__VA_ARGS__, 2, 1, 0)(arena, __VA_ARGS__)

#define STR_EMPTY (str_t){0}

str_t str_init(arena_t *arena, const char *buf);
str_t str_init_len(arena_t *arena, const char *buf, usize len);
str_t str_init_view(arena_t *arena, strview_t view);
str_t str_fmt(arena_t *arena, const char *fmt, ...);
str_t str_fmtv(arena_t *arena, const char *fmt, va_list args);

tstr_t tstr_init(TCHAR *str, usize optional_len);
str16_t str16_init(u16 *str, usize optional_len);

str_t str_from_str16(arena_t *arena, str16_t src);
str_t str_from_tstr(arena_t *arena, tstr_t src);
str16_t str16_from_str(arena_t *arena, str_t src);

bool str_equals(str_t a, str_t b);
int str_compare(str_t a, str_t b);

str_t str_dup(arena_t *arena, str_t src);
str_t str_cat(arena_t *arena, str_t a, str_t b);
bool str_is_empty(str_t ctx);

void str_lower(str_t *src);
void str_upper(str_t *src);

void str_replace(str_t *ctx, char from, char to);
// if len == SIZE_MAX, copies until end
strview_t str_sub(str_t ctx, usize from, usize to);

// == STRVIEW_T ====================================================

// these macros might be THE worst code ever written, but they work ig
// detects if you're trying to create a string view from either:
//  - a str_t          -> calls strv_init_str
//  - a string literal -> calls strv_init_len with comptime size
//  - a c string       -> calls strv_init with runtime size

#define STRV_EMPTY (strview_t){0}

// needed for strv__init_literal _Generic implementation, it's never actually called
inline strview_t strv__ignore(str_t s, size_t l) { 
    COLLA_UNUSED(s); COLLA_UNUSED(l); 
    return STRV_EMPTY; 
}

#define strv__check(x, ...) ((#x)[0] == '"')
#define strv__init_literal(x, ...) \
    _Generic((x), \
        char *: strv_init_len, \
        const char *: strv_init_len, \
        str_t: strv__ignore \
    )(x, sizeof(x) - 1)

#define strv__1(x)                   \
    _Generic((x),                    \
        char *:       strv_init,     \
        const char *: strv_init,     \
        str_t:        strv_init_str  \
    )(x)

#define strv__2(cstr, clen) strv_init_len(cstr, clen)

#define strv__impl(_1, _2, n, ...) strv__##n

#define strv(...) strv__check(__VA_ARGS__) ? strv__init_literal(__VA_ARGS__) : strv__impl(__VA_ARGS__, 2, 1, 0)(__VA_ARGS__)

strview_t strv_init(const char *cstr);
strview_t strv_init_len(const char *buf, usize size);
strview_t strv_init_str(str_t str);

bool strv_is_empty(strview_t ctx);
bool strv_equals(strview_t a, strview_t b);
int strv_compare(strview_t a, strview_t b);

char strv_front(strview_t ctx);
char strv_back(strview_t ctx);

str16_t strv_to_str16(arena_t *arena, strview_t src);
tstr_t strv_to_tstr(arena_t *arena, strview_t src);

strview_t strv_remove_prefix(strview_t ctx, usize n);
strview_t strv_remove_suffix(strview_t ctx, usize n);
strview_t strv_trim(strview_t ctx);
strview_t strv_trim_left(strview_t ctx);
strview_t strv_trim_right(strview_t ctx);

strview_t strv_sub(strview_t ctx, usize from, usize to);

bool strv_starts_with(strview_t ctx, char c);
bool strv_starts_with_view(strview_t ctx, strview_t view);

bool strv_ends_with(strview_t ctx, char c);
bool strv_ends_with_view(strview_t ctx, strview_t view);

bool strv_contains(strview_t ctx, char c);
bool strv_contains_view(strview_t ctx, strview_t view);
bool strv_contains_either(strview_t ctx, strview_t chars);

usize strv_find(strview_t ctx, char c, usize from);
usize strv_find_view(strview_t ctx, strview_t view, usize from);
usize strv_find_either(strview_t ctx, strview_t chars, usize from);

usize strv_rfind(strview_t ctx, char c, usize from_right);
usize strv_rfind_view(strview_t ctx, strview_t view, usize from_right);

// == CTYPE ========================================================

bool char_is_space(char c);
bool char_is_alpha(char c);
bool char_is_num(char c);

// == INPUT STREAM =================================================

typedef struct instream_t instream_t;
struct instream_t {
    const char *beg;
    const char *cur;
    usize len;
};

instream_t istr_init(strview_t str);

// get the current character and advance
char istr_get(instream_t *ctx);
// get the current character but don't advance
char istr_peek(instream_t *ctx);
// get the next character but don't advance
char istr_peek_next(instream_t *ctx);
// returns the previous character
char istr_prev(instream_t *ctx);
// returns the character before the previous
char istr_prev_prev(instream_t *ctx);
// ignore characters until the delimiter
void istr_ignore(instream_t *ctx, char delim);
// ignore characters until the delimiter and skip it
void istr_ignore_and_skip(instream_t *ctx, char delim);
// skip n characters
void istr_skip(instream_t *ctx, usize n);
// skips whitespace (' ', '\\n', '\\t', '\\r')
void istr_skip_whitespace(instream_t *ctx);
// returns to the beginning of the stream
void istr_rewind(instream_t *ctx);
// returns back <amount> characters
void istr_rewind_n(instream_t *ctx, usize amount);
// returns the number of bytes read from beginning of stream
usize istr_tell(instream_t *ctx);
// returns the number of bytes left to read in the stream
usize istr_remaining(instream_t *ctx); 
// return true if the stream doesn't have any new bytes to read
bool istr_is_finished(instream_t *ctx);

bool istr_get_bool(instream_t *ctx, bool *val);
bool istr_get_u8(instream_t *ctx, u8 *val);
bool istr_get_u16(instream_t *ctx, u16 *val);
bool istr_get_u32(instream_t *ctx, u32 *val);
bool istr_get_u64(instream_t *ctx, u64 *val);
bool istr_get_i8(instream_t *ctx, i8 *val);
bool istr_get_i16(instream_t *ctx, i16 *val);
bool istr_get_i32(instream_t *ctx, i32 *val);
bool istr_get_i64(instream_t *ctx, i64 *val);
bool istr_get_num(instream_t *ctx, double *val);
strview_t istr_get_view(instream_t *ctx, char delim);
strview_t istr_get_view_either(instream_t *ctx, strview_t chars);
strview_t istr_get_view_len(instream_t *ctx, usize len);
strview_t istr_get_line(instream_t *ctx);

// == OUTPUT STREAM ================================================

typedef struct outstream_t outstream_t;
struct outstream_t {
    char *beg;
    arena_t *arena;
};

outstream_t ostr_init(arena_t *exclusive_arena);
void ostr_clear(outstream_t *ctx);

usize ostr_tell(outstream_t *ctx);

char ostr_back(outstream_t *ctx);
str_t ostr_to_str(outstream_t *ctx);
strview_t ostr_as_view(outstream_t *ctx);

void ostr_pop(outstream_t *ctx, usize count);

void ostr_print(outstream_t *ctx, const char *fmt, ...);
void ostr_printv(outstream_t *ctx, const char *fmt, va_list args);
void ostr_putc(outstream_t *ctx, char c);
void ostr_puts(outstream_t *ctx, strview_t v);

void ostr_append_bool(outstream_t *ctx, bool val);
void ostr_append_uint(outstream_t *ctx, u64 val);
void ostr_append_int(outstream_t *ctx, i64 val);
void ostr_append_num(outstream_t *ctx, double val);

// == INPUT BINARY STREAM ==========================================

typedef struct {
    const u8 *beg;
    const u8 *cur;
    usize len;
} ibstream_t;

ibstream_t ibstr_init(buffer_t buffer);

bool ibstr_is_finished(ibstream_t *ib);
usize ibstr_tell(ibstream_t *ib);
usize ibstr_remaining(ibstream_t *ib);
usize ibstr_read(ibstream_t *ib, void *buffer, usize len);
void ibstr_skip(ibstream_t *ib, usize count);

bool ibstr_get_u8(ibstream_t *ib, u8 *out);
bool ibstr_get_u16(ibstream_t *ib, u16 *out);
bool ibstr_get_u32(ibstream_t *ib, u32 *out);
bool ibstr_get_u64(ibstream_t *ib, u64 *out);

bool ibstr_get_i8(ibstream_t *ib, i8 *out);
bool ibstr_get_i16(ibstream_t *ib, i16 *out);
bool ibstr_get_i32(ibstream_t *ib, i32 *out);
bool ibstr_get_i64(ibstream_t *ib, i64 *out);

#endif