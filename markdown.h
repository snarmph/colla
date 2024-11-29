#pragma once

#include "str.h"

typedef struct outstream_t outstream_t;

typedef struct {
    strview_t lang;
    void *userdata;
    void (*init)(void *userdata);
    void (*finish)(void *userdata);
    void (*callback)(strview_t line, outstream_t *out, void *userdata);
} md_parser_t;

typedef struct {
    md_parser_t *parsers;
    int parsers_count;
    ini_t *out_config;
} md_options_t;

typedef struct ini_t ini_t;

str_t markdown(arena_t *arena, arena_t scratch, strview_t filename, md_options_t *options);
str_t markdownStr(arena_t *arena, strview_t markdown_str, md_options_t *options);

/*
md-lite
a subset of markdown that can be parsed line by line
rules:
    begin of file:
    [ ] if there are three dashes (---), everythin until the next three dashes will be read as an ini config

    begin of line:
    [x] n # -> <h1..n>
    [x] *** or --- or ___ on their own line -> <hr>
    [x] - or * -> unordered list
    [x] n. -> ordered list
    [x] ```xyz and newline -> code block of language <xyz> (xyz is optional)  

    mid of line:
    [x] * -> italic
    [x] ** -> bold
    [x] *** -> bold and italic
    [x] [text](link) -> link
    [x] ![text](link) -> image
    [x] ` -> code block until next backtick

    other:
    [x] empty line -> </p>
    [x] \ -> escape character

    todo?:
    [ ] two space at end of line or \ -> <br>
    [ ] indent inside list -> continue in point
    [ ] 4 spaces -> line code block (does NOT work with multiline, use ``` instead)
    [ ] <url> -> link
    [ ] [text](link "title") -> link
    [ ] fix ***both***
*/