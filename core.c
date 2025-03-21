#include "core.h"

#include <stdio.h>

#if COLLA_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#define STB_SPRINTF_DECORATE(name) colla_stb_##name
#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"

#if COLLA_CLANG
#pragma clang diagnostic pop
#endif

colla_modules_e colla__initialised_modules = 0;

extern void os_init(void);
extern void net_init(void);
extern void os_cleanup(void);
extern void net_cleanup(void);

static char *colla_fmt__stb_callback(const char *buf, void *ud, int len) {
    fflush(stdout);
    fwrite(buf, 1, len, stdout);
    return (char *)ud;
}

void colla_init(colla_modules_e modules) {
    colla__initialised_modules = modules;
    if (modules & COLLA_OS) {
        os_init();
    }
    if (modules & COLLA_NET) {
        net_init();
    }
}

void colla_cleanup(void) {
    colla_modules_e modules = colla__initialised_modules;
    if (modules & COLLA_OS) {
        os_cleanup();
    }
    if (modules & COLLA_NET) {
        net_cleanup();
    }
}

int fmt_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int out = fmt_printv(fmt, args);
    va_end(args);
    return out;
}

int fmt_printv(const char *fmt, va_list args) {
    char buffer[STB_SPRINTF_MIN] = {0};
    return colla_stb_vsprintfcb(colla_fmt__stb_callback, buffer, buffer, fmt, args);
}

int fmt_buffer(char *buf, usize len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int out = fmt_bufferv(buf, len, fmt, args);
    va_end(args);
    return out;
}

int fmt_bufferv(char *buf, usize len, const char *fmt, va_list args) {
    return colla_stb_vsnprintf(buf, (int)len, fmt, args);
}
