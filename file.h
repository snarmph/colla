#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "str.h"

enum {
    FILE_READ, FILE_WRITE, FILE_BOTH
};

typedef struct {
    void *handle;
} file_t;

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

#ifdef __cplusplus
} // extern "C"
#endif