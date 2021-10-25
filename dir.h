#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "str.h"

typedef void *dir_t;

typedef struct {
    int type;
    str_t name;
} dir_entry_t;

enum {
    FS_TYPE_UNKNOWN,
    FS_TYPE_FILE,
    FS_TYPE_DIR,
};

dir_t dirOpen(const char *path);
void dirClose(dir_t ctx);

dir_entry_t *dirNext(dir_t ctx);

#ifdef __cplusplus
} // extern "C"
#endif