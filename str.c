#include "str.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#if COLLA_WIN
#include "win/str_win32.c"
#else
#error "platform not supported"
#endif

// == STR_T ========================================================

str_t str_init(arena_t *arena, const char *buf) {
	return str_init_len(arena, buf, buf ? strlen(buf) : 0);
}

str_t str_init_len(arena_t *arena, const char *buf, usize len) {
	if (!buf || !len) return STR_EMPTY;
	char *str = alloc(arena, char, len + 1);
    memmove(str, buf, len);
	return (str_t){ str, len };
}

str_t str_init_view(arena_t *arena, strview_t view) {
	return str_init_len(arena, view.buf, view.len);
}

str_t str_fmt(arena_t *arena, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	str_t out = str_fmtv(arena, fmt, args);
	va_end(args);
	return out;
}

str_t str_fmtv(arena_t *arena, const char *fmt, va_list args) {
    va_list vcopy;
    va_copy(vcopy, args);
    // stb_vsnprintf returns the length + null_term
    int len = fmt_bufferv(NULL, 0, fmt, vcopy);
    va_end(vcopy);

    char *buffer = alloc(arena, char, len + 1);
    fmt_bufferv(buffer, len + 1, fmt, args);

    return (str_t) { .buf = buffer, .len = (usize)len };
}

tstr_t tstr_init(TCHAR *str, usize optional_len) {
    if (str && !optional_len) {
#if COLLA_UNICODE
        optional_len = wcslen(str);
#else
        optional_len = strlen(str);
#endif
    }
    return (tstr_t){
        .buf = str,
        .len = optional_len,
    };
}

str16_t str16_init(u16 *str, usize optional_len) {
    if (str && !optional_len) {
        optional_len = wcslen(str);
    }
    return (str16_t){
        .buf = str,
        .len = optional_len,
    };
}

str_t str_from_str16(arena_t *arena, str16_t src) {
    if (!src.buf) return STR_EMPTY;
    if (!src.len) return STR_EMPTY;

    str_t out = str_os_from_str16(arena, src);

    return out;
}

str_t str_from_tstr(arena_t *arena, tstr_t src) {
#if COLLA_UNICODE
    return str_from_str16(arena, src);
#else
    return str(arena, strv(src));
#endif
}

str16_t str16_from_str(arena_t *arena, str_t src) {
    return strv_to_str16(arena, strv(src));
}

bool str_equals(str_t a, str_t b) {
	return str_compare(a, b) == 0;
}

int str_compare(str_t a, str_t b) {
	// TODO unsinged underflow if a.len < b.len
	return a.len == b.len ? memcmp(a.buf, b.buf, a.len) : (int)(a.len - b.len);
}

str_t str_dup(arena_t *arena, str_t src) {
	return str_init_len(arena, src.buf, src.len);
}

str_t str_cat(arena_t *arena, str_t a, str_t b) {
    str_t out = STR_EMPTY;

    out.len += a.len + b.len;
    out.buf = alloc(arena, char, out.len + 1);
    memcpy(out.buf, a.buf, a.len);
    memcpy(out.buf + a.len, b.buf, b.len);

    return out;
}

bool str_is_empty(str_t ctx) {
	return !ctx.buf || !ctx.len;
}

void str_lower(str_t *src) {
    for (usize i = 0; i < src->len; ++i) {
        if (src->buf[i] >= 'A' && src->buf[i] <= 'Z') {
            src->buf[i] += 'a' - 'A';
        }
    }
}

void str_upper(str_t *src) {
    for (usize i = 0; i < src->len; ++i) {
        if (src->buf[i] >= 'a' && src->buf[i] <= 'z') {
            src->buf[i] -= 'a' - 'A';
        }
    }
}

void str_replace(str_t *ctx, char from, char to) {
    if (!ctx) return;
    char *buf = ctx->buf;
    for (usize i = 0; i < ctx->len; ++i) {
        buf[i] = buf[i] == from ? to : buf[i];
    }
}

strview_t str_sub(str_t ctx, usize from, usize to) {
    if (to > ctx.len) to = ctx.len;
    if (from > to)    from = to;
    return (strview_t){ ctx.buf + from, to - from };
}

// == STRVIEW_T ====================================================

strview_t strv_init(const char *cstr) {
	return strv_init_len(cstr, cstr ? strlen(cstr) : 0);
}

strview_t strv_init_len(const char *buf, usize size) {
    return (strview_t){
        .buf = buf,
        .len = size,
    };
}

strview_t strv_init_str(str_t str) {
    return (strview_t){
        .buf = str.buf,
        .len = str.len
    };
}

bool strv_is_empty(strview_t ctx) {
    return ctx.len == 0 || !ctx.buf;
}

bool strv_equals(strview_t a, strview_t b) {
    return strv_compare(a, b) == 0;
}

int strv_compare(strview_t a, strview_t b) {
	// TODO unsinged underflow if a.len < b.len
    return a.len == b.len ?
        memcmp(a.buf, b.buf, a.len) :
        (int)(a.len - b.len);
}

char strv_front(strview_t ctx) {
    return ctx.len > 0 ? ctx.buf[0] : '\0';
}

char strv_back(strview_t ctx) {
    return ctx.len > 0 ? ctx.buf[ctx.len - 1] : '\0';
}

str16_t strv_to_str16(arena_t *arena, strview_t src) {
    return strv_os_to_str16(arena, src);
}

tstr_t strv_to_tstr(arena_t *arena, strview_t src) {
#if UNICODE
    return strv_to_str16(arena, src);
#else
	return str(arena, src);
#endif
}

strview_t strv_remove_prefix(strview_t ctx, usize n) {
    if (n > ctx.len) n = ctx.len;
    return (strview_t){
        .buf = ctx.buf + n,
        .len = ctx.len - n,
    };
}

strview_t strv_remove_suffix(strview_t ctx, usize n) {
    if (n > ctx.len) n = ctx.len;
    return (strview_t){
        .buf = ctx.buf,
        .len = ctx.len - n,
    };
}

strview_t strv_trim(strview_t ctx) {
	return strv_trim_left(strv_trim_right(ctx));
}

strview_t strv_trim_left(strview_t ctx) {
    strview_t out = ctx;
    for (usize i = 0; i < ctx.len; ++i) {
        char c = ctx.buf[i];
        if (c != ' ' && (c < '\t' || c > '\r')) {
            break;
        }
        out.buf++;
        out.len--;
    }
    return out;
}

strview_t strv_trim_right(strview_t ctx) {
    strview_t out = ctx;
    for (isize i = ctx.len - 1; i >= 0; --i) {
        char c = ctx.buf[i];
        if (c != ' ' && (c < '\t' || c > '\r')) {
            break;
        }
        out.len--;
    }
    return out;
}

strview_t strv_sub(strview_t ctx, usize from, usize to) {
    if (ctx.len == 0) return STRV_EMPTY;
    if (to > ctx.len) to = ctx.len;
    if (from > to) from = to;
    return (strview_t){ ctx.buf + from, to - from };
}

bool strv_starts_with(strview_t ctx, char c) {
    return ctx.len > 0 && ctx.buf[0] == c;
}

bool strv_starts_with_view(strview_t ctx, strview_t view) {
    return ctx.len >= view.len && memcmp(ctx.buf, view.buf, view.len) == 0;
}

bool strv_ends_with(strview_t ctx, char c) {
    return ctx.len > 0 && ctx.buf[ctx.len - 1] == c;
}

bool strv_ends_with_view(strview_t ctx, strview_t view) {
    return ctx.len >= view.len && memcmp(ctx.buf + ctx.len - view.len, view.buf, view.len) == 0;
}

bool strv_contains(strview_t ctx, char c) {
    for(usize i = 0; i < ctx.len; ++i) {
        if(ctx.buf[i] == c) {
            return true;
        }
    }
    return false;
}

bool strv_contains_view(strview_t ctx, strview_t view) {
    if (ctx.len < view.len) return false;
    usize end = ctx.len - view.len;
    for (usize i = 0; i < end; ++i) {
        if (memcmp(ctx.buf + i, view.buf, view.len) == 0) {
            return true;
        }
    }
    return false;
}

bool strv_contains_either(strview_t ctx, strview_t chars) {
    for (usize i = 0; i < ctx.len; ++i) {
        if (strv_contains(chars, ctx.buf[i])) {
            return true;
        }
    }

    return false;
}

usize strv_find(strview_t ctx, char c, usize from) {
    for (usize i = from; i < ctx.len; ++i) {
        if (ctx.buf[i] == c) {
            return i;
        }
    }
    return STR_NONE;
}

usize strv_find_view(strview_t ctx, strview_t view, usize from) {
    usize end = ctx.len - view.len;
    for (usize i = from; i < end; ++i) {
        if (memcmp(ctx.buf + i, view.buf, view.len) == 0) {
            return i;
        }
    }
    return STR_NONE;
}

usize strv_find_either(strview_t ctx, strview_t chars, usize from) {
    if (from > ctx.len) from = ctx.len;
    
    for (usize i = from; i < ctx.len; ++i) {
        if (strv_contains(chars, ctx.buf[i])) {
            return i;
        }
    }

    return STR_NONE;
}

usize strv_rfind(strview_t ctx, char c, usize from_right) {
    if (ctx.len == 0) return STR_NONE;
    if (from_right > ctx.len) from_right = ctx.len;
    isize end = (isize)(ctx.len - from_right);
    for (isize i = end; i >= 0; --i) {
        if (ctx.buf[i] == c) {
            return (usize)i;
        }
    }
    return STR_NONE;
}

usize strv_rfind_view(strview_t ctx, strview_t view, usize from_right) {
    if (ctx.len == 0) return STR_NONE;
    if (from_right > ctx.len) from_right = ctx.len;
    isize end = (isize)(ctx.len - from_right);
    if (end < (isize)view.len) return STR_NONE;
    for (isize i = end - view.len; i >= 0; --i) {
        if (memcmp(ctx.buf + i, view.buf, view.len) == 0) {
            return (usize)i;
        }
    }
    return STR_NONE;
}

// == CTYPE ========================================================

bool char_is_space(char c) {
    return (c >= '\t' && c <= '\r') || c == ' ';
}

bool char_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool char_is_num(char c) {
    return c >= '0' && c <= '9';
}

// == INPUT STREAM =================================================

instream_t istr_init(strview_t str) {
	return (instream_t) {
		.beg = str.buf,
        .cur = str.buf,
        .len = str.len,
	};
}

char istr_get(instream_t *ctx) {
    return istr_remaining(ctx) ? *ctx->cur++ : '\0';
}

char istr_peek(instream_t *ctx) {
    return istr_remaining(ctx) ? *ctx->cur : '\0';
}

char istr_peek_next(instream_t *ctx) {
    return istr_remaining(ctx) > 1 ? *(ctx->cur + 1) : '\0';
}

char istr_prev(instream_t *ctx) {
    return istr_tell(ctx) ? *(ctx->cur - 1) : '\0';
}

char istr_prev_prev(instream_t *ctx) {
    return istr_tell(ctx) > 1 ? *(ctx->cur - 2) : '\0';
}

void istr_ignore(instream_t *ctx, char delim) {
    while (!istr_is_finished(ctx) && *ctx->cur != delim) {
        ctx->cur++;
    }
}

void istr_ignore_and_skip(instream_t *ctx, char delim) {
    istr_ignore(ctx, delim);
    istr_skip(ctx, 1);
}

void istr_skip(instream_t *ctx, usize n) {
    if (!ctx) return;
    usize rem = istr_remaining(ctx);
    if (n > rem) n = rem;
    ctx->cur += n;
}

void istr_skip_whitespace(instream_t *ctx) {
    while (!istr_is_finished(ctx) && char_is_space(*ctx->cur)) {
        ctx->cur++;
    }
}

void istr_rewind(instream_t *ctx) {
    if (ctx) ctx->cur = ctx->beg;
}

void istr_rewind_n(instream_t *ctx, usize amount) {
    if (!ctx) return;
    usize rem = istr_remaining(ctx);
    ctx->cur -= MIN(amount, rem);
}

usize istr_tell(instream_t *ctx) {
    return ctx ? ctx->cur - ctx->beg : 0;
}

usize istr_remaining(instream_t *ctx) {
    return ctx ? ctx->len - (ctx->cur - ctx->beg) : 0;
}
 
bool istr_is_finished(instream_t *ctx) {
    return !(ctx && istr_remaining(ctx) > 0);
}

bool istr_get_bool(instream_t *ctx, bool *val) {
    if (!ctx || !ctx->cur || !val) return false;
    usize rem = istr_remaining(ctx);
    if (rem >= 4 && memcmp(ctx->cur, "true", 4) == 0) {
        *val = true;
        ctx->cur += 4;
        return true;
    }
    if (rem >= 5 && memcmp(ctx->cur, "false", 5) == 0) {
        *val = false;
        ctx->cur += 5;
        return true;
    }
    return false;
}

bool istr_get_u8(instream_t *ctx, u8 *val) {
    u64 out = 0;
    bool result = istr_get_u64(ctx, &out);
    if (result && out < UINT8_MAX) {
        *val = (u8)out;
    }
    return result;
}

bool istr_get_u16(instream_t *ctx, u16 *val) {
    u64 out = 0;
    bool result = istr_get_u64(ctx, &out);
    if (result && out < UINT16_MAX) {
        *val = (u16)out;
    }
    return result;
}

bool istr_get_u32(instream_t *ctx, u32 *val) {
    u64 out = 0;
    bool result = istr_get_u64(ctx, &out);
    if (result && out < UINT32_MAX) {
        *val = (u32)out;
    }
    return result;
}


bool istr_get_u64(instream_t *ctx, u64 *val) {
    if (!ctx || !ctx->cur || !val) return false;
    char *end = NULL;
    *val = strtoull(ctx->cur, &end, 0);

    if (ctx->cur == end) {
        return false;
    }
    else if (*val == ULLONG_MAX) {
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istr_get_i8(instream_t *ctx, i8 *val) {
    i64 out = 0;
    bool result = istr_get_i64(ctx, &out);
    if (result && out > INT8_MIN && out < INT8_MAX) {
        *val = (i8)out;
    }
    return result;
}

bool istr_get_i16(instream_t *ctx, i16 *val) {
    i64 out = 0;
    bool result = istr_get_i64(ctx, &out);
    if (result && out > INT16_MIN && out < INT16_MAX) {
        *val = (i16)out;
    }
    return result;
}

bool istr_get_i32(instream_t *ctx, i32 *val) {
    i64 out = 0;
    bool result = istr_get_i64(ctx, &out);
    if (result && out > INT32_MIN && out < INT32_MAX) {
        *val = (i32)out;
    }
    return result;
}

bool istr_get_i64(instream_t *ctx, i64 *val) {
    if (!ctx || !ctx->cur || !val) return false;
    char *end = NULL;
    *val = strtoll(ctx->cur, &end, 0);

    if (ctx->cur == end) {
        return false;
    }
    else if(*val == INT64_MAX || *val == INT64_MIN) {
        return false;
    }

    ctx->cur = end;
    return true;
}

bool istr_get_num(instream_t *ctx, double *val) {
    if (!ctx || !ctx->cur || !val) return false;
    char *end = NULL;
    *val = strtod(ctx->cur, &end);
    
    if(ctx->cur == end) {
        warn("istrGetDouble: no valid conversion could be performed (%.5s)", ctx->cur);
        return false;
    }
    else if(*val == HUGE_VAL || *val == -HUGE_VAL) {
        warn("istrGetDouble: value read is out of the range of representable values");
        return false;
    }

    ctx->cur = end;
    return true;
}

strview_t istr_get_view(instream_t *ctx, char delim) {
    if (!ctx || !ctx->cur) return STRV_EMPTY;
    const char *from = ctx->cur;
    istr_ignore(ctx, delim);
    usize len = ctx->cur - from;
    return strv(from, len);
}

strview_t istr_get_view_either(instream_t *ctx, strview_t chars) {
    if (!ctx || !ctx->cur) return STRV_EMPTY;
    const char *from = ctx->cur;
    while (!istr_is_finished(ctx) && !strv_contains(chars, *ctx->cur)) {
        ctx->cur++;
    }

    usize len = ctx->cur - from;
    return strv(from, len);
}

strview_t istr_get_view_len(instream_t *ctx, usize len) {
    if (!ctx || !ctx->cur) return STRV_EMPTY;
    const char *from = ctx->cur;
    istr_skip(ctx, len);
    usize buflen = ctx->cur - from;
    return (strview_t){ from, buflen };
}

strview_t istr_get_line(instream_t *ctx) {
    strview_t line = istr_get_view(ctx, '\n');
    istr_skip(ctx, 1);
    if (strv_ends_with(line, '\r')) {
        line = strv_remove_suffix(line, 1);
    }
    return line;
}

// == OUTPUT STREAM ================================================

outstream_t ostr_init(arena_t *exclusive_arena) {
    return (outstream_t) {
        .beg = (char *)(exclusive_arena ? exclusive_arena->cur : NULL),
        .arena = exclusive_arena,
    };
}

void ostr_clear(outstream_t *ctx) {
    arena_pop(ctx->arena, ostr_tell(ctx));
}

usize ostr_tell(outstream_t *ctx) {
    return ctx->arena ? (char *)ctx->arena->cur - ctx->beg : 0;
}

char ostr_back(outstream_t *ctx) {
    usize len = ostr_tell(ctx);
    return len ? ctx->beg[len - 1] : '\0';
}

str_t ostr_to_str(outstream_t *ctx) {
    ostr_putc(ctx, '\0');

    str_t out = {
        .buf = ctx->beg,
        .len = ostr_tell(ctx) - 1,
    };

    memset(ctx, 0, sizeof(outstream_t));
    return out;
}

strview_t ostr_as_view(outstream_t *ctx) {
    return strv(ctx->beg, ostr_tell(ctx));
}

void ostr_pop(outstream_t *ctx, usize count) {
    if (!ctx->arena) return;
    arena_pop(ctx->arena, count);
}

void ostr_print(outstream_t *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ostr_printv(ctx, fmt, args);
    va_end(args);
}

void ostr_printv(outstream_t *ctx, const char *fmt, va_list args) {
    if (!ctx->arena) return;
    str_fmtv(ctx->arena, fmt, args);
    // remove null terminator
    arena_pop(ctx->arena, 1);
}

void ostr_putc(outstream_t *ctx, char c) {
    if (!ctx->arena) return;
    char *newc = alloc(ctx->arena, char);
    *newc = c;
}

void ostr_puts(outstream_t *ctx, strview_t v) {
    if (strv_is_empty(v)) return;
    str(ctx->arena, v);
    // remove null terminator
    arena_pop(ctx->arena, 1);
}

void ostr_append_bool(outstream_t *ctx, bool val) {
    ostr_puts(ctx, val ? strv("true") : strv("false"));
}

void ostr_append_uint(outstream_t *ctx, u64 val) {
    ostr_print(ctx, "%I64u", val);
}

void ostr_append_int(outstream_t *ctx, i64 val) {
    ostr_print(ctx, "%I64d", val);
}

void ostr_append_num(outstream_t *ctx, double val) {
    ostr_print(ctx, "%g", val);
}

// == INPUT BINARY STREAM ==========================================

ibstream_t ibstr_init(buffer_t buffer) {
    return (ibstream_t){
        .beg = buffer.data,
        .cur = buffer.data,
        .len = buffer.len,
    };
}

bool ibstr_is_finished(ibstream_t *ib) {
    return !(ib && ibstr_remaining(ib) > 0);
}

usize ibstr_tell(ibstream_t *ib) {
    return ib && ib->cur ? ib->cur - ib->beg : 0;
}

usize ibstr_remaining(ibstream_t *ib) {
    return ib ? ib->len - ibstr_tell(ib) : 0;
}

usize ibstr_read(ibstream_t *ib, void *buffer, usize len) {
    usize rem = ibstr_remaining(ib);
    if (len > rem) len = rem;
    memmove(buffer, ib->cur, len);
    ib->cur += len;
    return len;
}

void ibstr_skip(ibstream_t *ib, usize count) {
    usize rem = ibstr_remaining(ib);
    if (count > rem) count = rem;
    ib->cur += count;
}

bool ibstr_get_u8(ibstream_t *ib, u8 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_u16(ibstream_t *ib, u16 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_u32(ibstream_t *ib, u32 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_u64(ibstream_t *ib, u64 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_i8(ibstream_t *ib, i8 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_i16(ibstream_t *ib, i16 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_i32(ibstream_t *ib, i32 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}

bool ibstr_get_i64(ibstream_t *ib, i64 *out) {
    return ibstr_read(ib, out, sizeof(*out)) == sizeof(*out);
}
