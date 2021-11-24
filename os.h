#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>
#include "str.h"

#ifdef _WIN32
    #include <stdio.h>
    #include "win32_slim.h"
    typedef SSIZE_T ssize_t;
    ssize_t getdelim(char **buf, size_t *bufsz, int delimiter, FILE *fp);
    ssize_t getline(char **line_ptr, size_t *n, FILE *stream);
    #define stricmp _stricmp
#else
    #define stricmp strcasecmp
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <stdio.h>
    #include <sys/types.h> // ssize_t
#endif

str_t getUserName();

#ifdef __cplusplus
} // extern "C"
#endif