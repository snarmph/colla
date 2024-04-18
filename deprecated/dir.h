#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "str.h"

typedef void *dir_t;

typedef enum {
    FS_TYPE_UNKNOWN,
    FS_TYPE_FILE,
    FS_TYPE_DIR,
} fs_type_t;

typedef struct {
    fs_type_t type;
    str_t name;
} dir_entry_t;

dir_t dirOpen(const char *path);
void dirClose(dir_t ctx);

bool dirValid(dir_t ctx);

dir_entry_t *dirNext(dir_t ctx);

void dirCreate(const char *path);
bool dirRemove(const char *path);

#ifdef __cplusplus
} // extern "C"
#endif
