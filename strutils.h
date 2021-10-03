#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>

#ifdef _WIN32
    #include <stdio.h>
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
    ssize_t getdelim(char **buf, size_t *bufsz, int delimiter, FILE *fp);
    ssize_t getline(char **line_ptr, size_t *n, FILE *stream);
    #define stricmp _stricmp
#else
    #define stricmp strcasecmp
    #define _GNU_SOURCE
    #include <stdio.h>
#endif

// prefix str -> changes string
// prefix cstr -> allocates new string and returns it

// int str

void strToLower(char *str);
void strnToLower(char *str, size_t len);

char *cstrdup(const char *str);
char *cstrToLower(const char *str);

#ifdef __cplusplus
} // extern "C"
#endif