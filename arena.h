#ifndef COLLA_ARENA_H
#define COLLA_ARENA_H

#include "core.h"

#if COLLA_WIN && !COLLA_TCC
#define alignof __alignof
#endif

typedef enum arena_type_e {
    ARENA_TYPE_NONE, // only here so that a 0 initialised arena is valid
    ARENA_VIRTUAL,
    ARENA_MALLOC,
    ARENA_MALLOC_ALWAYS,
    ARENA_STATIC,
} arena_type_e;

typedef enum alloc_flags_e {
    ALLOC_FLAGS_NONE = 0,
    ALLOC_NOZERO     = 1 << 0,
    ALLOC_SOFT_FAIL  = 1 << 1,
} alloc_flags_e;

typedef struct arena_t arena_t;
struct arena_t {
    u8 *beg;
    u8 *cur;
    u8 *end;
    arena_type_e type;
};

typedef struct arena_desc_t arena_desc_t;
struct arena_desc_t {
    arena_type_e type;
    usize size;
    u8 *static_buffer;
};

typedef struct arena_alloc_desc_t arena_alloc_desc_t;
struct arena_alloc_desc_t {
    arena_t *arena;
    usize count;
    alloc_flags_e flags;
    usize align;
    usize size;
};

// arena_type_e type, usize allocation, [ byte *static_buffer ]
#define arena_make(...) arena_init(&(arena_desc_t){ __VA_ARGS__ })

// arena_t *arena, T type, [ usize count, alloc_flags_e flags, usize align, usize size ]
#define alloc(arenaptr, type, ...) arena_alloc(&(arena_alloc_desc_t){ .size = sizeof(type), .count = 1, .align = alignof(type), .arena = arenaptr, __VA_ARGS__ })

// simple arena that always calls malloc internally, this is useful if you need
// malloc for some reason but want to still use the arena interface
// WARN: most arena functions outside of alloc/scratch won't work!
//       you also need to each allocation afterwards! this is still
//       malloc 
extern arena_t malloc_arena;

arena_t arena_init(const arena_desc_t *desc);
void arena_cleanup(arena_t *arena);

arena_t arena_scratch(arena_t *arena, usize size);

void *arena_alloc(const arena_alloc_desc_t *desc);
usize arena_tell(arena_t *arena);
usize arena_remaining(arena_t *arena);
usize arena_capacity(arena_t *arena);
void arena_rewind(arena_t *arena, usize from_start);
void arena_pop(arena_t *arena, usize amount);

#endif