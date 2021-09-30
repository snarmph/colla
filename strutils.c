#include "strutils.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void strToLower(char *str) {
    for(char *beg = str; *beg; ++beg) {
        *beg = tolower(*beg);
    }
}

void strnToLower(char *str, size_t len) {
    for(size_t i = 0; i < len; ++i) {
        str[i] = tolower(str[i]);
    }
}

char *cstrdup(const char *str) {
    size_t len = strlen(str);
    char *buf = malloc(len + 1);
    memcpy(buf, str, len);
    buf[len] = '\0';
    return buf;
}

char *cstrToLower(const char *str) {
    char *buf = cstrdup(str);
    strToLower(buf);
    return buf;
}