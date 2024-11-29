#pragma once

#include "str.h"

typedef struct {
    uint32 digest[5];
    uint8 block[64];
    usize block_index;
    usize byte_count;
} sha1_t;

typedef struct {
    uint32 digest[5];
} sha1_result_t;

sha1_t sha1_init(void);
sha1_result_t sha1(sha1_t *ctx, const void *buf, usize len);
str_t sha1Str(arena_t *arena, sha1_t *ctx, const void *buf, usize len);
