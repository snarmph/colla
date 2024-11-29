#include "dir.h"

#if COLLA_WIN

#include <windows.h>

typedef struct dir_t {
    WIN32_FIND_DATA find_data;
    HANDLE handle;
    dir_entry_t cur_entry;
    dir_entry_t next_entry;
} dir_t;

static dir_entry_t dir__entry_from_find_data(arena_t *arena, WIN32_FIND_DATA *fd) {
    dir_entry_t out = {0};

    out.name = str(arena, fd->cFileName);

    if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        out.type = DIRTYPE_DIR;
    }
    else {
        LARGE_INTEGER filesize = {
            .LowPart  = fd->nFileSizeLow,
            .HighPart = fd->nFileSizeHigh,
        };
        out.filesize = filesize.QuadPart;
    }

    return out;
}

dir_t *dirOpen(arena_t *arena, strview_t path) {
    uint8 tmpbuf[1024] = {0};
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
    
    TCHAR *winpath = strvToTChar(&scratch, path);
    // get a little extra leeway
    TCHAR fullpath[MAX_PATH + 16] = {0};
    DWORD pathlen = GetFullPathName(winpath, MAX_PATH, fullpath, NULL);
    // add asterisk at the end of the path
    if (fullpath[pathlen ] != '\\' && fullpath[pathlen] != '/') {
        fullpath[pathlen++] = '\\';
    }
    fullpath[pathlen++] = '*';
    fullpath[pathlen++] = '\0';

    dir_t *ctx = alloc(arena, dir_t);
    ctx->handle = FindFirstFile(fullpath, &ctx->find_data);

    if (ctx->handle == INVALID_HANDLE_VALUE) {
        arenaPop(arena, sizeof(dir_t));
        return NULL;
    }

    ctx->next_entry = dir__entry_from_find_data(arena, &ctx->find_data);

    return ctx;
}

void dirClose(dir_t *ctx) {
    FindClose(ctx->handle);
    ctx->handle = INVALID_HANDLE_VALUE;
}

bool dirIsValid(dir_t *ctx) {
    return ctx && ctx->handle != INVALID_HANDLE_VALUE;
}

dir_entry_t *dirNext(arena_t *arena, dir_t *ctx) {
    if (!dirIsValid(ctx)) {
        return NULL;
    }

    ctx->cur_entry = ctx->next_entry;

    ctx->next_entry = (dir_entry_t){0};

    if (FindNextFile(ctx->handle, &ctx->find_data)) {
        ctx->next_entry = dir__entry_from_find_data(arena, &ctx->find_data);
    }
    else {
        dirClose(ctx);
    }
    
    return &ctx->cur_entry;
}

#elif COLLA_POSIX

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

// taken from https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html
// hopefully shouldn't be needed
#ifndef DT_DIR
    #define DT_DIR 4
#endif
#ifndef DT_REG
    #define DT_REG 8
#endif

typedef struct dir_t {
    DIR *dir;
    dir_entry_t next;
} dir_t;

dir_t *dirOpen(arena_t *arena, strview_t path) {
    if (strvIsEmpty(path)) {
        err("path cannot be null");
        return NULL;
    }

    uint8 tmpbuf[1024];
    arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
    str_t dirpath = str(&scratch, path);

    DIR *dir = opendir(dirpath.buf);
    if (!dir) {
        err("could not open dir (%v)", path);
        return NULL;
    }

    dir_t *ctx = alloc(arena, dir_t);
    ctx->dir = dir;
    return ctx;
}

void dirClose(dir_t *ctx) {
    if (ctx) {
        closedir(ctx->dir);
    }
}

bool dirIsValid(dir_t *ctx) {
    return ctx && ctx->dir;
}

dir_entry_t *dirNext(arena_t *arena, dir_t *ctx) {
    if (!ctx) return NULL;

    struct dirent *dp = readdir(ctx->dir);
    if (!dp) {
        dirClose(ctx);
        return NULL;
    }

    ctx->next.name = (str_t){ dp->d_name, strlen(dp->d_name) };
    ctx->next.type = dp->d_type == DT_DIR ? DIRTYPE_DIR : DIRTYPE_FILE;
    ctx->next.filesize = 0;

    if (dp->d_type == DT_REG) {
        struct stat file_info = {0};
        stat(dp->d_name, &file_info);
        ctx->next.filesize = file_info.st_size;
    }

    return &ctx->next;
}

#endif
