#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>

#if _WIN32
#define stricmp _stricmp
#elif __unix__ || __APPLE__
#define stricmp strcasecmp
#else
#error "stricmp is not supported"
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