#include "arena.h"

#include <assert.h>
#include <string.h>

#include "os.h"

static uptr arena__align(uptr ptr, usize align) {
    return (ptr + (align - 1)) & ~(align - 1);
}

static arena_t arena__make_virtual(usize size);
static arena_t arena__make_malloc(usize size);
static arena_t arena__make_static(u8 *buf, usize len);

static void *arena__alloc_common(const arena_alloc_desc_t *desc);
static void *arena__alloc_malloc_always(const arena_alloc_desc_t *desc);

static void arena__free_virtual(arena_t *arena);
static void arena__free_malloc(arena_t *arena);

arena_t malloc_arena = {
    .type = ARENA_MALLOC_ALWAYS,
};

arena_t arena_init(const arena_desc_t *desc) {
    arena_t out = {0};
    
    if (desc) {
        switch (desc->type) {
            case ARENA_VIRTUAL: out = arena__make_virtual(desc->size); break;
            case ARENA_MALLOC:  out = arena__make_malloc(desc->size); break;
            case ARENA_STATIC:  out = arena__make_static(desc->static_buffer, desc->size); break;
		    default: break;  
        }
    }

    return out;
}

void arena_cleanup(arena_t *arena) {
    if (!arena) {
        return;
    }
    
    switch (arena->type) {
        case ARENA_VIRTUAL: arena__free_virtual(arena); break;
        case ARENA_MALLOC:  arena__free_malloc(arena);  break;
        // ARENA_STATIC does not need to be freed
        default: break;  
    }
    
    memset(arena, 0, sizeof(arena_t));
}

arena_t arena_scratch(arena_t *arena, usize size) {
    u8 *buffer = alloc(arena, u8, size, ALLOC_SOFT_FAIL | ALLOC_NOZERO);
    return arena__make_static(buffer, buffer ? size : 0);
}

void *arena_alloc(const arena_alloc_desc_t *desc) {
    if (!desc || !desc->arena || desc->arena->type == ARENA_TYPE_NONE) {
        return NULL;
    }

    arena_t *arena = desc->arena;

    u8 *ptr = NULL;

    switch (arena->type) {
        case ARENA_MALLOC_ALWAYS:
            ptr = arena__alloc_malloc_always(desc);
            break;
        default:
            ptr = arena__alloc_common(desc);
            break; 
    }

    usize total = desc->size * desc->count;

    return desc->flags & ALLOC_NOZERO ? ptr : memset(ptr, 0, total);
}

usize arena_tell(arena_t *arena) {
    return arena ? arena->cur - arena->beg : 0;
}

usize arena_remaining(arena_t *arena) {
    return arena && (arena->cur < arena->end) ? arena->end - arena->cur : 0;
}

usize arena_capacity(arena_t *arena) {
	return arena ? arena->end - arena->beg : 0;
}

void arena_rewind(arena_t *arena, usize from_start) {
    if (!arena) {
        return;
    }

    assert(arena_tell(arena) >= from_start);

    arena->cur = arena->beg + from_start;
}

void arena_pop(arena_t *arena, usize amount) {
    if (!arena) {
        return;
    }
    usize position = arena_tell(arena);
    if (!position) {
        return;
    }
    arena_rewind(arena, position - amount);
}

// == VIRTUAL ARENA ====================================================================================================

static arena_t arena__make_virtual(usize size) {
    usize alloc_size = 0;
    u8 *ptr = os_reserve(size, &alloc_size);
    if (!os_commit(ptr, 1)) {
        os_release(ptr, alloc_size);
        ptr = NULL;
    }

    return (arena_t){
        .beg = ptr,
        .cur = ptr,
        .end = ptr ? ptr + alloc_size : NULL,
        .type = ARENA_VIRTUAL,
    };
}

static void arena__free_virtual(arena_t *arena) {
    if (!arena->beg) {
        return;
    }

    bool success = os_release(arena->beg, arena_capacity(arena));
    assert(success && "Failed arena free");
}
// == MALLOC ARENA =====================================================================================================

extern void *malloc(usize size);
extern void free(void *ptr);

static arena_t arena__make_malloc(usize size) {
    u8 *ptr = malloc(size);
    assert(ptr);
    return (arena_t) {
        .beg = ptr,
        .cur = ptr,
        .end = ptr ? ptr + size : NULL,
        .type = ARENA_MALLOC,
    };
}

static void arena__free_malloc(arena_t *arena) {
    free(arena->beg);
}

// == ARENA ALLOC ======================================================================================================

static void *arena__alloc_common(const arena_alloc_desc_t *desc) {
    usize total = desc->size * desc->count;
    arena_t *arena = desc->arena;

    arena->cur = (u8 *)arena__align((uptr)arena->cur, desc->align);
    bool soft_fail = desc->flags & ALLOC_SOFT_FAIL;

    if (total > arena_remaining(arena)) {
        if (!soft_fail) {
            fatal("finished space in arena, tried to allocate %_$$$dB out of %_$$$dB\n", total, arena_remaining(arena));
        }
        return NULL;
    }

    if (arena->type == ARENA_VIRTUAL) {
        usize allocated = arena_tell(arena);
        usize page_end = os_pad_to_page(allocated);
        usize new_cur = allocated + total;

        if (new_cur > page_end) {
            usize extra_mem = os_pad_to_page(new_cur - page_end);
            usize page_size = os_get_system_info().page_size;
            // TODO is this really correct?
            usize num_of_pages = (extra_mem / page_size) + 1;

            assert(num_of_pages > 0);

            if (!os_commit(arena->cur, num_of_pages + 1)) {
                if (!soft_fail) {
                    fatal("failed to commit memory for virtual arena, tried to commit %zu pages\n", num_of_pages);
                }
                return NULL;
            }
        }
    }

    u8 *ptr = arena->cur;
    arena->cur += total;

    return ptr;
}

static void *arena__alloc_malloc_always(const arena_alloc_desc_t *desc) {
    usize total = desc->size * desc->count;

    // TODO: alignment?
    u8 *ptr = malloc(total);
    if (!ptr && !(desc->flags & ALLOC_SOFT_FAIL)) {
        fatal("malloc call failed for %_$$$dB", total);
    }

    return ptr;
}


// == STATIC ARENA =====================================================================================================

static arena_t arena__make_static(u8 *buf, usize len) {
    return (arena_t) {
        .beg = buf,
        .cur = buf,
        .end = buf ? buf + len : NULL,
        .type = ARENA_STATIC,
    };
}

