#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>
#include "str.h"
#include "collatypes.h"

#ifdef _WIN32
    #include <stdio.h>
    #include "win32_slim.h"
    isize getdelim(char **buf, size_t *bufsz, int delimiter, FILE *fp);
    isize getline(char **line_ptr, size_t *n, FILE *stream);
    #define stricmp _stricmp
#else
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <stdio.h>
    int stricmp(const char *a, const char *b);
#endif

str_t getUserName();

#ifdef __cplusplus
} // extern "C"
#endif
