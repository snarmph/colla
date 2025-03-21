#include "../str.h"
#include "../arena.h"

#include <windows.h>

#if COLLA_TCC
#include "../tcc/colla_tcc.h"
#endif

str_t str_os_from_str16(arena_t *arena, str16_t src) {
    str_t out = {0};
    
    int outlen = WideCharToMultiByte(
        CP_UTF8, 0,
        src.buf, (int)src.len,
        NULL, 0,
        NULL, NULL
    );

    if (outlen == 0) {
        unsigned long error = GetLastError();
        if (error == ERROR_NO_UNICODE_TRANSLATION) {
            err("couldn't translate wide string (%S) to utf8, no unicode translation", src.buf);
        }
        else {
            err("couldn't translate wide string (%S) to utf8, %v", src.buf, os_get_error_string(os_get_last_error()));
        }

        return STR_EMPTY;
    }

    out.buf = alloc(arena, char, outlen + 1);
    WideCharToMultiByte(
        CP_UTF8, 0,
        src.buf, (int)src.len,
        out.buf, outlen,
        NULL, NULL
    );

    out.len = outlen;

    return out;
}

str16_t strv_os_to_str16(arena_t *arena, strview_t src) {
    str16_t out = {0};

    if (strv_is_empty(src)) {
        return out;
    }

    int len = MultiByteToWideChar(
        CP_UTF8, 0,
        src.buf, (int)src.len,
        NULL, 0
    );

    if (len == 0) {
        unsigned long error = GetLastError();
        if (error == ERROR_NO_UNICODE_TRANSLATION) {
            err("couldn't translate string (%v) to a wide string, no unicode translation", src);
        }
        else {
            err("couldn't translate string (%v) to a wide string, %v", src, os_get_error_string(os_get_last_error()));
        }

        return out;
    }

    out.buf = alloc(arena, wchar_t, len + 1);

    MultiByteToWideChar(
        CP_UTF8, 0,
        src.buf, (int)src.len,
        out.buf, len
    );

    out.len = len;

    return out;
}