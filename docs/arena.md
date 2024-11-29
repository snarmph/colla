---
title = Arena
---
# Arena
-----------

An arena is a bump allocator, under the hood it can use one of 3 strategies to allocate its data:

* `Virtual`: allocates with virtual memory, meaning that it reserves the data upfront, but only allocates one page at a time (usually 64 Kb). This is the preferred way to use the arena as it can freely allocate gigabytes of memory for free
* `Malloc`: allocates the memory upfront using malloc
* `Static`: uses a buffer passed by the user instead of allocating

To help with allocating big chunks of data, `arena.h` defines the macros `KB`, `MB`, and `GB`. If you don't need them you can define `ARENA_NO_SIZE_HELPERS`

To create an arena use the macro `arenaMake`:
```c
arena_t varena = arenaMake(ARENA_VIRTUAL, GB(1)); 
uint8 buffer[1024];
arena_t sarena = arenaMake(ARENA_STATIC, sizeof(buffer), buffer);
```

To allocate use the `alloc` macro. The parameters to allocate are:

* A pointer to the arena
* The type to allocate
* (optional) the number of items to allocate
* (optional) flags:
  * `ALLOC_NOZERO`: by default the arena sets the memory to zero before returning, use this if you want to skip it
  * `ALLOC_SOFT_FAIL`: by default the arena panics when an allocation fails, meaning that it never returns `NULL`.
* (automatic) the align of the type
* (automatic) the size of the type

Example usage:

```c
// allocate 15 strings
str_t *string_list = alloc(arena, str_t, 15);
// allocate a 1024 bytes buffer 
uint8 *buffer = alloc(arena, uint8, 1024);
// allocate a structure
game_t *game = alloc(arena, game_t);
```

The strength of the arena is that it makes it much easier to reason about lifetimes, for instance:

```c
// pass a pointer to the arena for the data that we need to return
WCHAR *win32_get_full_path(arena_t *arena, const char *rel_path) {
    // we need a small scratch arena for allocations that 
    // will be thrown away at the end of this function
    uint8 scratch_buf[1024];
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(scratch_buf, scratch_buf));

    WCHAR *win32_rel_path = str_to_wchar(&scratch, rel_path);

    DWORD pathlen = GetFullPathName(win32_rel_path, 0, NULL, NULL);
    WCHAR *fullpath = alloc(arena, WCHAR, pathlen + 1);
    GetFullPath(win32_rel_path, pathlen + 1, fullpath, NULL);

    // notice how we don't need to free anything at the end
    return fullpath;
}
```

There are a few helper functions:

* `arenaScratch`: sub allocate an arena from another arena
* `arenaTell`: returns the number of bytes allocated
* `arenaRemaining`: returns the number of bytes that can still be allocated
* `arenaRewind`: rewinds the arena to N bytes from the start (effectively "freeing" that memory)
* `arenaPop`: pops N bytes from the arena (effectively "freeing" that memory)

Here is an example usage of a full program:

```c
typedef struct {
    char *buf;
    usize len;
} str_t;

str_t read_file(arena_t *arena, const char *filename) {
    str_t out = {0};

    FILE *fp = fopen(filename, "rb");
    if (!fp) goto error;

    fseek(fp, 0, SEEK_END);
    out.len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    out.buf = alloc(arena, char, out.len + 1);

    fread(out.buf, 1, out.len, fp);

error:
    return out;
}

void write_file(const char *filename, str_t data) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;
    fwrite(data.buf, 1, data.len, fp);
}

int main() {
    arena_t arena = arenaMake(ARENA_VIRTUAL, GB(1));
    str_t title = {0};

    {
        uint8 tmpbuf[KB(5)];
        arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
        str_t file = read_file(&scratch, "file.json");
        json_t json = json_parse(&scratch, file);
        title = str_dup(arena, json_get(json, "title"));
    }

    {
        // copying an arena by value effectively makes it a scratch arena, 
        // as long as you don't use the original inside the same scope!
        arena_t scratch = arena;
        str_t to_write = str_fmt(
            &scratch, 
            "{ \"title\": \"%s\" }", title.buf
        );
        write_file("out.json", to_write);
    }

    // cleanup all allocations at once
    arenaCleanup(&arena);
}
```