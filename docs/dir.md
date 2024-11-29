---
title = Dir
---
# Dir
----------

This header provides a simple directory walker, here is an example usage:

```c
typedef struct source_t {
    str_t filename;
    struct source_t *next;
} source_t;

sources_t get_sources(arena_t *arena, strview_t path) {
    uint8 tmpbuf[KB(5)] = {0};
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);

    dir_t *dir = dirOpen(&scratch, path);
    dir_entry_t *entry = NULL;

    source_t *sources = NULL;

    while ((entry = dirNext(&scratch, dir))) {
        if (entry->type != DIRTYPE_FILE) {
            continue;
        }

        strview_t ext = fileGetExtension(strv(entry->name));
        if (!strvEquals(ext, strv(".c"))) {
            continue;
        }

        source_t *new_source = alloc(arena, source_t);
        new_source->filename = strFmt(arena, "%v/%v", path, entry->name);
        new_source->next = sources;
        sources = new_source;
    }

    return sources;
}

int main() {
    arena_t arena = arenaMake(ARENA_VIRTUAL, GB(1));
    source_t *sources = get_sources(&arena, strv("src/colla"));
    while (sources) {
        info("> %v", sources->filename);
        sources = sources->next;
    }
    arenaCleanup(&arena);
}
```