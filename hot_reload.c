#include "hot_reload.h"

#include "arena.h"
#include "file.h"
#include "tracelog.h"

// todo linux support?
#if COLLA_WIN
#include <windows.h>
#else
// patch stuff up for cross platform for now, the actual program should not really call anything for now
#define HMODULE void*
#endif

typedef int (*hr_f)(hr_t *ctx);

typedef struct {
    arena_t arena;
    str_t path;
    uint64 last_timestamp;
    HMODULE handle;
    hr_f hr_init;
    hr_f hr_loop;
    hr_f hr_close;
} hr_internal_t;

static bool hr__os_reload(hr_internal_t *hr);
static void hr__os_free(hr_internal_t *hr);

static bool hr__file_copy(arena_t scratch, strview_t src, strview_t dst) {
    buffer_t srcbuf = fileReadWhole(&scratch, src);
    if (srcbuf.data == NULL || srcbuf.len == 0) {
        err("fileReadWhole(%v) returned an empty buffer", src);
        return false;
    }
    if (!fileWriteWhole(scratch, dst, srcbuf.data, srcbuf.len)) {
        err("fileWriteWhole failed");
        return false;
    }
    return true;
}

static bool hr__reload(hr_t *ctx) {
#ifdef HR_DISABLE
    return true;
#endif

    hr_internal_t *hr = ctx->p;
    arena_t scratch = hr->arena;

    if (!fileExists(hr->path.buf)) {
        err("dll file %v does not exist anymore!", hr->path);
        return false;
    }

    uint64 now = fileGetTime(scratch, strv(hr->path));
    if (now <= hr->last_timestamp) {
        return false;
    }
    
    ctx->version = ctx->last_working_version + 1;

    // can't copy the dll directly, make a temporary one based on the version
    strview_t dir, name, ext;
    fileSplitPath(strv(hr->path), &dir, &name, &ext);
    str_t dll = strFmt(&scratch, "%v/%v-%d%v", dir, name, ctx->version, ext);

    if (!hr__file_copy(scratch, strv(hr->path), strv(dll))) {
        err("failed to copy %v to %v", hr->path, dll);
        return false;
    }

    info("loading library: %v", dll);

    return hr__os_reload(hr);
}

bool hrOpen(hr_t *ctx, strview_t path) {
#ifdef HR_DISABLE
    cr_init(ctx);
    return true;
#endif
    arena_t arena = arenaMake(ARENA_VIRTUAL, MB(1));

    str_t path_copy = str(&arena, path);

    if (!fileExists(path_copy.buf)) {
        err("dll file: %v does not exist", path);
        arenaCleanup(&arena);
        return false;
    }

    hr_internal_t *hr = alloc(&arena, hr_internal_t);
    hr->arena = arena;
    hr->path = path_copy;

    ctx->p = hr;
    ctx->last_working_version = 0;

    return hr__reload(ctx);
}

void hrClose(hr_t *ctx, bool clean_temp_files) {
#ifdef HR_DISABLE
    hr_close(ctx);
    return;
#endif

    hr_internal_t *hr = ctx->p;
    if (hr->hr_close) {
        hr->hr_close(ctx);
    }

    hr__os_free(hr);

    hr->handle = NULL;
    hr->hr_init = hr->hr_loop = hr->hr_close = NULL;

    if (clean_temp_files) {
        arena_t scratch = hr->arena;

        strview_t dir, name, ext;
        fileSplitPath(strv(hr->path), &dir, &name, &ext);

        for (int i = 0; i < ctx->last_working_version; ++i) {
            str_t fname = strFmt(&scratch, "%v/%v-%d%v", dir, name, i + 1, ext);
            if (!fileDelete(scratch, strv(fname))) {
                err("couldn't delete %v", fname);
            }
        }
    }

    arena_t arena = hr->arena;
    arenaCleanup(&arena);

    ctx->p = NULL;
}

int hrStep(hr_t *ctx) {
#ifdef CR_DISABLE
    hr_loop(ctx);
    return 0;
#endif
    hr_internal_t *hr = ctx->p;

    int result = -1;
    if (hr->hr_loop) {
        result = hr->hr_loop(ctx);
    }
    return result;
}

bool hrReload(hr_t *ctx) {
    return hr__reload(ctx);
}

//// OS SPECIFIC ////////////////////////////////////////

#if COLLA_WIN

static bool hr__os_reload(hr_internal_t *hr) {
    if (hr->handle) {
        FreeLibrary(hr->handle);
    }

    hr->handle = LoadLibraryA(dll.buf);
    if (!hr->handle) {
        err("couldn't load %v: %u", dll, GetLastError());
        return true;
    }

    hr->hr_init = (hr_f)GetProcAddress(hr->handle, "hr_init");
    DWORD init_err = GetLastError();
    hr->hr_loop = (hr_f)GetProcAddress(hr->handle, "hr_loop");
    DWORD loop_err = GetLastError();
    hr->hr_close = (hr_f)GetProcAddress(hr->handle, "hr_close");
    DWORD close_err = GetLastError();

    if (!hr->hr_init) {
        err("couldn't load address for hr_init: %u", init_err);
        goto error;
    }

    if (!hr->hr_loop) {
        err("couldn't load address for hr_loop: %u", loop_err);
        goto error;
    }

    if (!hr->hr_close) {
        err("couldn't load address for hr_close: %u", close_err);
        goto error;
    }

    info("Reloaded, version: %d", ctx->version);
    hr->last_timestamp = now;
    ctx->last_working_version = ctx->version;

    hr->hr_init(ctx);

    return true;

error:
    if (hr->handle) FreeLibrary(hr->handle);
    hr->handle = NULL;
    hr->hr_init = hr->hr_loop = hr->hr_close = NULL;
    return false;
}

static void hr__os_free(hr_internal_t *hr) {
    if (hr->handle) {
        FreeLibrary(hr->handle);
    }
}

#elif COLLA_LIN

static bool hr__os_reload(hr_internal_t *hr) {
    fatal("todo: linux hot reload not implemented yet");
    return true;
}

static void hr__os_free(hr_internal_t *hr) {
    fatal("todo: linux hot reload not implemented yet");
}

#endif