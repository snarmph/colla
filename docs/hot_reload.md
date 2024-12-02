# Hot Reload
----------

This header provides cross-platform library hot-reloading. To use it you need to have to entry points, one for the host and one for the client.

In the client you can then implement these functions:

* `hr_init`: called when the library is loaded (or reloaded)
* `hr_loop`: called every "tick" (or whenever the host decides)
* `hr_close`: called when the host finishes

In the client you need to call these functions:

* `hrOpen`: load the library and call `hr_init`
* `hrStep`: call `hr_loop`
* `hrReload`: check if the library has changed, and if so reload it and call `hr_init` again
* `hrClose`: call `hr_close` and cleanup

Example usage:

### Client

```c
int hr_init(hr_t *ctx) {
    uint8 tmpbuf[KB(5)];
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);

    state_t *state = ctx->userdata;
    uint64 timestamp = fileGetTime(scratch, strv("sprite.png"));
    if (timestamp > state->last_write) {
        state->last_write = timestamp;
        destroy_image(state->sprite);
        state->sprite = load_image(strv("sprite.png"));
    }
}

int hr_loop(hr_t *ctx) {
    state_t *state = ctx->userdata;
    draw_image(state->sprite, state->posx, state->posy);
}

int hr_close(hr_t *ctx) {
    state_t *state = ctx->userdata;
    destroy_image(state->sprite);
}
```

### Host

```c
typedef struct {
    hr_t hr;
    image_t sprite;
    int posx;
    int posy;
    uint64 last_write;
} state_t;

int main() {
    arena_t arena = arenaMake(ARENA_VIRTUAL, GB(1));

    state_t state = {0};
    state.hr.userdata = &state;

    if (!hrOpen(&state.hr, strv("bin/client.dll"))) {
        return 1;
    }

    while (game_poll()) {
        hrReload(&state.hr);
        hrStep(&state.hr);
    }

    hrClose(&state.hr, true);
}
```