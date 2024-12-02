# Highlight
----------

This header provides an highlighter for c-like languages (mostly c).
The usage is simple, first create a highlight context using hlInit, for which you need an hl_config_t. The only mandatory argument is colors, which are the strings put before the keywords in the highlighted text.

You can also pass some flags:
* `HL_FLAG_HTML`: escapes html characters

If you're using the offline documentation, this is the code used to highlight inside the markdown code blocks (simplified to remove actual markdown parsing):

```c
str_t highlight_code_block(arena_t *arena, strview_t code) {
    uint8 tmpbuf[KB(5)];
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);

    hl_ctx_t *hl = hlInit(&scratch, &(hl_config_t){
        .colors = {
            [HL_COLOR_NORMAL]       = strv("</span>"),
            [HL_COLOR_PREPROC]      = strv("<span class=\"c-preproc\">"),
            [HL_COLOR_TYPES]        = strv("<span class=\"c-types\">"),
            [HL_COLOR_CUSTOM_TYPES] = strv("<span class=\"c-custom-types\">"),
            [HL_COLOR_KEYWORDS]     = strv("<span class=\"c-kwrds\">"),
            [HL_COLOR_NUMBER]       = strv("<span class=\"c-num\">"),
            [HL_COLOR_STRING]       = strv("<span class=\"c-str\">"),
            [HL_COLOR_COMMENT]      = strv("<span class=\"c-cmnt\">"),
            [HL_COLOR_FUNC]         = strv("<span class=\"c-func\">"),
            [HL_COLOR_SYMBOL]       = strv("<span class=\"c-sym\">"),
            [HL_COLOR_MACRO]        = strv("<span class=\"c-macro\">"),
        },
        .flags = HL_FLAG_HTML,
    });
}
```