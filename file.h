#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "str.h"

enum {
    FILE_READ  = 1 << 0, 
    FILE_WRITE = 1 << 1, 
    FILE_CLEAR = 1 << 2, 
    FILE_BOTH  = 1 << 3
};

typedef struct {
    void *handle;
} file_t;

typedef struct {
    char *buf;
    size_t len;
} fread_buf_t;

bool fileExists(const char *fname);

file_t fileOpen(const char *fname, int mode);
void fileClose(file_t *ctx);

bool fileIsValid(file_t *ctx);

bool filePutc(file_t *ctx, char c);
bool filePuts(file_t *ctx, const char *str);
bool filePutstr(file_t *ctx, str_t str);
bool filePutview(file_t *ctx, strview_t view);

size_t fileRead(file_t *ctx, void *buf, size_t len);
size_t fileWrite(file_t *ctx, const void *buf, size_t len);

bool fileSeekEnd(file_t *ctx);
void fileRewind(file_t *ctx);

uint64_t fileTell(file_t *ctx);

fread_buf_t fileReadWhole(const char *fname);
fread_buf_t fileReadWholeFP(file_t *ctx);

str_t fileReadWholeText(const char *fname);
str_t fileReadWholeFPText(file_t *ctx);

#ifdef __cplusplus
} // extern "C"
#endif