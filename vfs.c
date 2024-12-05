#include "vfs.h"

#include "lz4/lz4.c"

#include "file.h"
#include "strstream.h"
#include "arena.h"
#include "dir.h"
#include "bits.h"

/*
vfs format:
=====================
header:
-----------
magic: VFS
u32 header_size
u32 file_count
u8 flags
for each file:
    u64   hash // todo remove
     u8   namelen
    char *name
    u64   offset
    u64   size
    u64   compressed_size
-----------
* binary data *
*/

typedef struct vfsfile_t vfsfile_t;

struct vfsfile_t {
    vfsfile_t *next;
    str_t path;
    uint64 size;
    uint64 comp_size;
};

typedef struct vfshmap_t vfshmap_t;
typedef struct vfshnode_t vfshnode_t;

typedef struct {
    uint64 offset;
    uint64 size;
    uint64 compsize;
} vfsdata_t;

struct vfshnode_t {
    vfshnode_t *next;
    uint64 hash;
    str_t key;
    uint32 index;
};

struct vfshmap_t {
    vfshnode_t **buckets;
    vfsdata_t *values;

    uint32 size;
    uint32 count;
    uint32 collisions;
    uint32 max_values;
};

struct virtualfs_t {
    file_t fp;
    buffer_t buffer;
    vfshmap_t hmap;
    uint64 base_offset;
    vfsflags_e flags;
};

vfsflags_e vfs_validate_flags(vfsflags_e flags);
bool vfs_read(arena_t *arena, virtualfs_t *vfs, vfsdata_t *data, buffer_t *out, bool null_terminate);
vfsfile_t *vfs_add_dir(arena_t *arena, strview_t path, vfsfile_t *tail, uint32 *count, uint64 *bytesize);
vfshmap_t vfs_hmap_init(arena_t *arena, int pow2, uint32 max_values);
void vfs_hmap_add(arena_t *arena, vfshmap_t *hmap, strview_t key, vfsdata_t value);
vfsdata_t *vfs_hmap_get(vfshmap_t *hmap, strview_t key);
uint64 sdbm_hash(const void *bytes, usize count);
uint64 djb2_hash(const void *bytes, usize count);

bool vfsVirtualiseDir(arena_t scratch, strview_t dirpath, strview_t outfile, vfsflags_e flags) {
    bool success = false;

    flags = vfs_validate_flags(flags);

    if (strvBack(dirpath) != '/') {
        str_t newpath = strFmt(&scratch, "%v/", dirpath);
        dirpath = strv(newpath);
    }

    uint32 count = 0;
    uint64 bytesize = 0;
    vfsfile_t file_head = {0};
    vfs_add_dir(&scratch, dirpath, &file_head, &count, &bytesize);
    vfsfile_t *files = file_head.next;
    
    arena_t comp_arena = {0};

    if (flags & VFS_FLAGS_COMPRESSED) {
        arena_t comp_arena = arenaMake(ARENA_VIRTUAL, GB(1));

        for_each (file, files) {
            arena_t tmp = scratch;
            buffer_t buf = fileReadWhole(&tmp, strv(file->path));
            usize maxlen = LZ4_compressBound(buf.len);

            uint8 *compressed = alloc(&comp_arena, uint8, maxlen);

            int actual_len = LZ4_compress_default(buf.data, compressed, buf.len, maxlen);
            assert(actual_len > 0 && actual_len <= maxlen);
            usize pop = maxlen - (usize)actual_len;

            // pop extra bytes that were allocated but not used
            arenaPop(&comp_arena, pop);

            file->comp_size = actual_len;
        }
    }

    obytestream_t header = obstrInit(&scratch);
    
    obstrAppendU32(&header, count);
    obstrAppendU8(&header, flags);

    uint64 offset = 0;
    for_each (file, files) {
        assert(file->path.len < 256);
        uint64 hash = djb2_hash(file->path.buf, file->path.len);

        obstrAppendU64(&header, hash);
        obstrAppendU8(&header, file->path.len);
        obstrPuts(&header, strv(file->path));
        obstrAppendU64(&header, offset);
        obstrAppendU64(&header, file->size);
        obstrAppendU64(&header, file->comp_size);

        offset += file->comp_size;
    }

    buffer_t headerbuf = obstrAsBuf(&header);

    buffer_t binbuf = {0};

    file_t fp = fileOpen(outfile, FILE_WRITE);

    if (!fileIsValid(fp)) {
        err("could not open file %v", outfile);
        goto failed;
    }

    uint32 header_size = headerbuf.len + 3 + sizeof(uint32); //  + strlen("VFS") + sizeof(header_size)

    filePuts(fp, strv("VFS"));
    fileWrite(fp, &header_size, sizeof(header_size));
    fileWrite(fp, headerbuf.data, headerbuf.len);  

    if (flags & VFS_FLAGS_COMPRESSED) {
        buffer_t compressed = {
            .data = comp_arena.start,
            .len = arenaTell(&comp_arena)
        };
        fileWrite(fp, compressed.data, compressed.len);
    }
    else {
        for_each (file, files) {
            arena_t tmp = scratch;
            buffer_t bin = fileReadWhole(&tmp, strv(file->path));
            if (flags & VFS_FLAGS_NULL_TERMINATE_FILES) {
                alloc(&tmp, uint8);
                bin.len += 1;
            }
            fileWrite(fp, bin.data, bin.len);
        }
    }

    fileClose(fp);
    
    success = true;
failed:
    arenaCleanup(&comp_arena);
    arenaCleanup(&scratch);
    return success;
}

virtualfs_t *vfsReadFromFile(arena_t *arena, strview_t vfs_file, vfsflags_e flags) {
    usize pos_before = arenaTell(arena);

    virtualfs_t *vfs = alloc(arena, virtualfs_t);

    file_t fp = fileOpen(vfs_file, FILE_READ);
    if (!fileIsValid(fp)) {
        goto failed;
    }

    // read header
    struct {
        char magic[3];
        uint32 size;
        uint32 file_count;
        uint8 flags;
    } header = {0};

    fileRead(fp, &header.magic, sizeof(header.magic));
    fileRead(fp, &header.size, sizeof(header.size));
    fileRead(fp, &header.file_count, sizeof(header.file_count));
    fileRead(fp, &header.flags, sizeof(header.flags));

    if (memcmp(header.magic, "VFS", 3) != 0) {
        err("VirtualFS: magic characters are wrong: %.3s (0x%x%x%x)", header.magic, header.magic[0], header.magic[1], header.magic[2]);
        goto failed;
    }

    uint32 default_pow2 = 1 << 10;
    uint32 pow2 = bitsNextPow2(header.file_count);
    pow2 = bitsCtz(max(pow2, default_pow2));

    vfs->hmap = vfs_hmap_init(arena, pow2, header.file_count);

    for (uint32 i = 0; i < header.file_count; ++i) {
        struct {
            uint64 hash;
            char name[256];
            uint64 offset;
            uint64 size;
            uint64 comp;
        } file = {0};

        uint8 namelen = 0;

        fileRead(fp, &file.hash, sizeof(file.hash));
        fileRead(fp, &namelen, sizeof(namelen));
        fileRead(fp, &file.name, namelen);
        fileRead(fp, &file.offset, sizeof(file.offset));
        fileRead(fp, &file.size, sizeof(file.size));
        fileRead(fp, &file.comp, sizeof(file.comp));

        vfsdata_t data = {
            .offset   = file.offset,
            .size     = file.size,
            .compsize = file.comp,
        };

        strview_t path = strvInitLen(file.name, namelen);

        vfs_hmap_add(arena, &vfs->hmap, path, data);
    }

    vfs->flags = vfs_validate_flags(header.flags | flags);
    vfs->base_offset = header.size;

    if (vfs->flags & VFS_FLAGS_DONT_STREAM) {
        // get remaining size of the file
        usize pos = fileTell(fp);
        fileSeekEnd(fp);
        usize endpos = fileTell(fp);
        fileSeek(fp, pos);
        usize binsize = endpos - pos;
        // read binary data and save it to buffer for later
        buffer_t buf = {
            .data = alloc(arena, uint8, binsize),
            .len = binsize,
        };
        usize read = fileRead(fp, buf.data, buf.len);
        if (read != buf.len) {
            err("couldn't read all of the binary data, expected %zu bytes but got %zu", buf.len, read);
            goto failed;
        }
        fileClose(fp);

        vfs->buffer = buf;
    }
    else {
        vfs->fp = fp;
    }

    return vfs;
failed:
    fileClose(fp);
    arenaRewind(arena, pos_before);
    return NULL;
}

buffer_t vfsRead(arena_t *arena, virtualfs_t *vfs, strview_t path) {
    buffer_t out = {0};
    usize pos_before = arenaTell(arena);

    if (!vfs) {
        goto failed;
    }

    vfsdata_t *data = vfs_hmap_get(&vfs->hmap, path);
    if (!data) {
        goto failed;
    }

    if (!vfs_read(arena, vfs, data, &out, false)) {
        goto failed;
    }

    return out;
failed:
    arenaRewind(arena, pos_before);
    return (buffer_t){0};
}

str_t vfsReadStr(arena_t *arena, virtualfs_t *vfs, strview_t path) {
    buffer_t buf = {0};
    usize pos_before = arenaTell(arena);

    if (!vfs) {
        goto failed;
    }

    vfsdata_t *data = vfs_hmap_get(&vfs->hmap, path);
    if (!data) {
        goto failed;
    }

    if (!vfs_read(arena, vfs, data, &buf, true)) {
        goto failed;
    }

    return (str_t){
        .buf = buf.data,
        .len = buf.len,
    };
failed:
    arenaRewind(arena, pos_before);
    return STR_EMPTY;
}

// == VFS FILE API ===================================

virtualfs_t *g_vfs = NULL;

void vfsSetGlobalVirtualFS(virtualfs_t *vfs) {
    g_vfs = vfs;
}

bool vfsFileExists(strview_t path) {
    if (!g_vfs) return false;
    return vfs_hmap_get(&g_vfs->hmap, path) != NULL;
}

vfs_file_t vfsFileOpen(strview_t name, int mode) {
    if (!g_vfs) goto failed;
    if (mode != FILE_READ) {
        err("VirtualFS: trying to open file (%v) for write, VirtualFS is read only!", name);
        goto failed;
    }

    vfsdata_t *data = vfs_hmap_get(&g_vfs->hmap, name);

    return (vfs_file_t){
        .handle = (uintptr_t)data,
    };

failed:
    return (vfs_file_t){0};
}

void vfsFileClose(vfs_file_t ctx) {
    (void)ctx;
}

bool vfsFileIsValid(vfs_file_t ctx) {
    return g_vfs && ctx.handle != 0;
}

usize vfsFileSize(vfs_file_t ctx) {
    if (!vfsFileIsValid(ctx)) return 0;
    vfsdata_t *data = (vfsdata_t *)ctx.handle;
    return data->size;
}

buffer_t vfsFileReadWhole(arena_t *arena, strview_t name) {
    return vfsRead(arena, g_vfs, name);
}

buffer_t vfsFileReadWholeFP(arena_t *arena, vfs_file_t ctx) {
    if (!vfsFileIsValid(ctx)) return (buffer_t){0};
    vfsdata_t *data = (vfsdata_t *)ctx.handle;
    buffer_t out = {0};
    usize pos_before = arenaTell(arena);
    if (!vfs_read(arena, g_vfs, data, &out, false)) {
        arenaRewind(arena, pos_before);
        return (buffer_t){0};
    }
    return out;
}

str_t vfsFileReadWholeStr(arena_t *arena, strview_t name) {
    return vfsReadStr(arena, g_vfs, name);
}

str_t vfsFileReadWholeStrFP(arena_t *arena, vfs_file_t ctx) {
    if (!vfsFileIsValid(ctx)) return STR_EMPTY;
    vfsdata_t *data = (vfsdata_t *)ctx.handle;
    buffer_t buf = {0};
    usize pos_before = arenaTell(arena);
    if (!vfs_read(arena, g_vfs, data, &buf, true)) {
        arenaRewind(arena, pos_before);
        return STR_EMPTY;
    }
    return (str_t){
        .buf = buf.data,
        .len = buf.len,
    };
}

// == PRIVATE FUNCTIONS ==============================

vfsflags_e vfs_validate_flags(vfsflags_e flags) {
    if (flags & VFS_FLAGS_COMPRESSED && flags & VFS_FLAGS_NULL_TERMINATE_FILES) {
        warn("VirtualFS: both COMPRESSEd and NULL_TERMINATE_FILES flags are set to ON, but they are mutually exclusive. turning NULL_TERMINATE_FILES off");
        flags &= ~VFS_FLAGS_NULL_TERMINATE_FILES;
    }

    return flags;
}

bool vfs_read(arena_t *arena, virtualfs_t *vfs, vfsdata_t *data, buffer_t *out, bool null_terminate) {
    if (!vfs || !data || !out) {
        return false;
    }

    bool is_allocated = true;
    out->len = data->size;

    if (vfs->flags & VFS_FLAGS_COMPRESSED) {
        out->data = alloc(arena, uint8, out->len);

        uint8 *compressed = NULL;

        if (vfs->flags & VFS_FLAGS_DONT_STREAM) {
            assert((data->offset + data->compsize) < vfs->buffer.len);
            compressed = vfs->buffer.data + data->offset;
        }
        else {
            uint64 offset = vfs->base_offset + data->offset;
            fileSeek(vfs->fp, offset);

            arena_t scratch = *arena;
            uint8 *compressed = alloc(&scratch, uint8, data->compsize);
            usize read = fileRead(vfs->fp, compressed, data->compsize);
            if (read != data->compsize) {
                err("VirtualFS: read %zu bytes, but should have read %zu", read, data->compsize);
                return false;
            }
        }
        
        int decompsize = LZ4_decompress_safe(compressed, out->data, data->compsize, out->len);
        if (decompsize < 0) {
            err("VirtualFS: couldn't decompress buffer: %d", decompsize);
            return false;
        }
    }
    else {
        if (vfs->flags & VFS_FLAGS_DONT_STREAM) {
            assert((data->offset + data->size) < vfs->buffer.len);
            out->data = vfs->buffer.data + data->offset;
            is_allocated = false;
        }
        else {
            out->data = alloc(arena, uint8, data->size);
            
            uint64 offset = vfs->base_offset + data->offset;
            fileSeek(vfs->fp, offset);
            usize read = fileRead(vfs->fp, out->data, out->len);
            if (read != out->len) {
                err("VirtualFS: read %zu bytes, but should have read %zu", read, out->len);
                return false;
            }
        }
    }

    if (null_terminate && !(vfs->flags & VFS_FLAGS_NULL_TERMINATE_FILES)) {
        if (is_allocated) {
            alloc(arena, char);
        }
        else {
            uint8 *buf = alloc(arena, uint8, out->len + 1);
            memcpy(buf, out->data, out->len);
            out->data = buf;
        }
        out->len += 1;
    }

    return true;
}

vfsfile_t *vfs_add_dir(arena_t *arena, strview_t path, vfsfile_t *tail, uint32 *count, uint64 *bytesize) {
    uint8 tmpbuf[KB(1)];
    
    dir_t *dir = dirOpen(arena, path);
    dir_entry_t *entry = NULL;

    if (strvEquals(path, strv("./"))) {
        path = STRV_EMPTY;
    }

    vfsfile_t *head = tail;
    vfsfile_t *cur = tail;

    while ((entry = dirNext(arena, dir))) {
        arena_t scratch = arenaMake(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
        
        vfsfile_t *newfile = NULL;

        if (entry->type == DIRTYPE_DIR) {
            if (strvEquals(strv(entry->name), strv(".")) ||
                strvEquals(strv(entry->name), strv(".."))
            ) {
                continue;
            }
            str_t fullpath = strFmt(&scratch, "%v%v/", path, entry->name);
            newfile = vfs_add_dir(arena, strv(fullpath), cur, count, bytesize);
        }
        else {
            newfile = alloc(arena, vfsfile_t);
            newfile->path = strFmt(arena, "%v%v", path, entry->name);
            newfile->size = entry->filesize;
            newfile->comp_size = newfile->size;

            if (cur) cur->next = newfile;
            (*count)++;
            (*bytesize) += newfile->size;
        }

        if (!head) head = newfile;
        cur = newfile;
    }

    return cur;
}


// == HASH MAP =======================================

vfshmap_t vfs_hmap_init(arena_t *arena, int pow2, uint32 max_values) {
    uint size = 1 << pow2;
    return (vfshmap_t) {
        .size = size,
        .max_values = max_values,
        .buckets = alloc(arena, vfshnode_t*, size),
        .values = alloc(arena, vfsdata_t, max_values),
    };
}

void vfs_hmap_add(arena_t *arena, vfshmap_t *hmap, strview_t key, vfsdata_t value) {
    if (!hmap) return;

    if ((float)hmap->count >= (float)hmap->size * 0.6f) {
        warn("more than 60%% of the hashmap is being used: %d/%d", hmap->count, hmap->size);
    }

    uint64 hash = djb2_hash(key.buf, key.len);
    usize index = hash & (hmap->size - 1);
    vfshnode_t *bucket = hmap->buckets[index];
    if (bucket) hmap->collisions++;
    while (bucket) {
        // already exists
        if (bucket->hash == hash && strvEquals(strv(bucket->key), key)) {
            hmap->values[bucket->index] = value;
            return;
        }
        bucket = bucket->next;
    }

    assert(hmap->count < hmap->max_values);

    bucket = alloc(arena, vfshnode_t);

    bucket->hash  = hash;
    bucket->key   = str(arena, key);
    bucket->index = hmap->count;
    bucket->next  = hmap->buckets[index];

    hmap->values[hmap->count++] = value;
    hmap->buckets[index] = bucket;
}

vfsdata_t *vfs_hmap_get(vfshmap_t *hmap, strview_t key) {
    if (!hmap || hmap->count == 0) {
        return NULL;
    }
    
    uint64 hash = djb2_hash(key.buf, key.len);
    usize index = hash & (hmap->size - 1);
    vfshnode_t *bucket = hmap->buckets[index];
    while (bucket) {
        if (bucket->hash == hash && strvEquals(strv(bucket->key), key)) {
            return &hmap->values[bucket->index];
        }
        bucket = bucket->next;
    }

    return NULL;
}

uint64 sdbm_hash(const void *bytes, usize count) {
    const uint8 *data = bytes;
    uint64 hash = 0;

    for (usize i = 0; i < count; ++i) {
		hash = data[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

uint64 djb2_hash(const void *bytes, usize count) {
    const uint8 *data = bytes;
    uint64 hash = 5381;
    int c;

    for (usize i = 0; i < count; ++i) {
        hash = ((hash << 5) + hash) + data[i]; 
    }

    return hash;
}

uint64 java_hash(const void *bytes, usize count) {
    const uint8 *data = bytes;
    uint64 hash = 1125899906842597L; 

    for (usize i = 0; i < count; ++i) {
        hash = ((hash << 5) - hash) + data[i];
    }

    return hash;
}
