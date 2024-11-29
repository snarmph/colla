#pragma once

#include "str.h"

typedef enum {
    HL_COLOR_NORMAL,
    HL_COLOR_PREPROC,
    HL_COLOR_TYPES,
    HL_COLOR_CUSTOM_TYPES,
    HL_COLOR_KEYWORDS,
    HL_COLOR_NUMBER,
    HL_COLOR_STRING,
    HL_COLOR_COMMENT,
    HL_COLOR_FUNC,
    HL_COLOR_SYMBOL,
    HL_COLOR_MACRO,

    HL_COLOR__COUNT,
} hl_color_e;

typedef enum {
    HL_FLAG_NONE = 0,
    HL_FLAG_HTML = 1 << 0,
} hl_flags_e;

typedef struct {
    strview_t keyword;
    hl_color_e color;
} hl_keyword_t;

typedef struct {
    usize idx;
    usize size;
} hl_line_t;

typedef struct {
    strview_t colors[HL_COLOR__COUNT];
    hl_keyword_t *extra_kwrds;
    int kwrds_count;
    hl_flags_e flags;
} hl_config_t;

typedef struct hl_ctx_t hl_ctx_t;

hl_ctx_t *hlInit(arena_t *arena, hl_config_t *config);
str_t hlHighlight(arena_t *arena, hl_ctx_t *ctx, strview_t str);

void hlSetSymbolInTable(hl_ctx_t *ctx, char symbol, bool value);
void hlAddKeyword(arena_t *arena, hl_ctx_t *ctx, hl_keyword_t *keyword);
