#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "collatypes.h"
#include "str.h"
#include "vec.h"

typedef enum {
    FILE_READ  = 1 << 0, 
    FILE_WRITE = 1 << 1, 
    FILE_CLEAR = 1 << 2, 
    FILE_BOTH  = 1 << 3
} filemode_t;

typedef uintptr_t file_t;

typedef struct {
    const uint8 *buf;
    usize len;
} filebuf_t;

bool fileExists(const char *fname);

file_t fileOpen(const char *fname, filemode_t mode);
void fileClose(file_t ctx);

bool fileIsValid(file_t ctx);

bool filePutc(file_t ctx, char c);
bool filePuts(file_t ctx, const char *str);
bool filePutstr(file_t ctx, str_t str);
bool filePutview(file_t ctx, strview_t view);

usize fileRead(file_t ctx, void *buf, usize len);
usize fileWrite(file_t ctx, const void *buf, usize len);

bool fileSeekEnd(file_t ctx);
void fileRewind(file_t ctx);

uint64 fileTell(file_t ctx);

vec(uint8) fileReadWhole(const char *fname);
vec(uint8) fileReadWholeFP(file_t ctx);

str_t fileReadWholeText(const char *fname);
str_t fileReadWholeTextFP(file_t ctx);

bool fileWriteWhole(const char *fname, filebuf_t data);
bool fileWriteWholeFP(file_t ctx, filebuf_t data);

bool fileWriteWholeText(const char *fname, strview_t string);
bool fileWriteWholeTextFP(file_t ctx, strview_t string);

uint64 fileGetTime(file_t ctx);
uint64 fileGetTimePath(const char *path);

#ifdef __cplusplus
} // extern "C"
#endif
