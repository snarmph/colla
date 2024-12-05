#pragma once

#include "str.h"
#include "arena.h"

typedef enum {
    VFS_FLAGS_NONE                 = 0,
    VFS_FLAGS_COMPRESSED           = 1 << 0,
    VFS_FLAGS_NULL_TERMINATE_FILES = 1 << 1, // only valid when compress is false
    VFS_FLAGS_DONT_STREAM          = 1 << 2, 
} vfsflags_e;

typedef struct virtualfs_t virtualfs_t;

bool vfsVirtualiseDir(arena_t scratch, strview_t dirpath, strview_t outfile, vfsflags_e flags);
virtualfs_t *vfsReadFromFile(arena_t *arena, strview_t vfs_file, vfsflags_e flags);
buffer_t vfsRead(arena_t *arena, virtualfs_t *vfs, strview_t path);
str_t vfsReadStr(arena_t *arena, virtualfs_t *vfs, strview_t path);

// vfs replacement for the file api

typedef struct {
    uintptr_t handle;
} vfs_file_t;

void vfsSetGlobalVirtualFS(virtualfs_t *vfs);

bool vfsFileExists(strview_t path);
vfs_file_t vfsFileOpen(strview_t name, int mode);
void vfsFileClose(vfs_file_t ctx);
bool vfsFileIsValid(vfs_file_t ctx);
usize vfsFileSize(vfs_file_t ctx);
buffer_t vfsFileReadWhole(arena_t *arena, strview_t name);
buffer_t vfsFileReadWholeFP(arena_t *arena, vfs_file_t ctx);
str_t vfsFileReadWholeStr(arena_t *arena, strview_t name);
str_t vfsFileReadWholeStrFP(arena_t *arena, vfs_file_t ctx);