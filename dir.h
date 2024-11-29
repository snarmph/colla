#pragma once

#include "str.h"
#include "arena.h"

typedef struct dir_t dir_t;

typedef enum {
    DIRTYPE_FILE,
    DIRTYPE_DIR,
} dir_type_e;

typedef struct {
    str_t name;
    dir_type_e type;
    usize filesize;
} dir_entry_t;

dir_t *dirOpen(arena_t *arena, strview_t path);
// optional, only call this if you want to return before dirNext returns NULL
void dirClose(dir_t *ctx);

bool dirIsValid(dir_t *ctx);

dir_entry_t *dirNext(arena_t *arena, dir_t *ctx);
