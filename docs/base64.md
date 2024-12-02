# Base 64
----------

The `base64.h` header has only two functions, one to encode and one to decode to/from base 64.

Example usage:
```c
char *recv_callback(arena_t *arena, buffer_t msg) {
    buffer_t decoded = base64Decode(arena, msg);
    alloc(arena, char); // allocate an extra char for the null pointer
    return (char *)decoded.data;
}

buffer_t send_callback(arena_t *arena) {
    char msg[] = "Hello World!";
    buffer_t buf = {
        .data = msg,
        .len = arrlen(msg) - 1, // don't include the null pointer
    };
    buffer_t encoded = base64Encode(arena, buf);
    return encoded;
}

int main() {
    arena_t arena = arenaMake(ARENA_VIRTUAL, GB(1));
    run_server(&arena, 8080, recv_callback, send_callback);
    arenaCleanup(&arena);
}
```
