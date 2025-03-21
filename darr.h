#ifndef COLLA_DARR_HEADER
#define COLLA_DARR_HEADER

/*
dynamic chunked array which uses an arena to allocate,
the structure needs to follow this exact format, if you want
you can use the macro darr_define(struct_name, item_type) instead:

////////////////////////////////////

typedef struct arr_t arr_t;
struct arr_t {
    int *items;
    usize block_size;
    usize count;
    arr_t *next;
    arr_t *head;
};
// equivalent to

darr_define(arr_t, int);

////////////////////////////////////

by default a chunk is 64 items long, you can change this default
by modifying the arr.block_size value before adding to the array,
or by defining DARRAY_DEFAULT_BLOCK_SIZE

usage example:

////////////////////////////////////

darr_define(arr_t, int);

arr_t *arr = NULL;

for (int i = 0; i < 100; ++i) {
    darr_push(&arena, arr, i);
}

for_each (chunk, arr) {
    for (int i = 0; i < chunk->count; ++i) {
        info("%d -> %d", i, chunk->items[i]);
    }
}
*/

#define DARRAY_DEFAULT_BLOCK_SIZE (64)

#define darr_define(struct_name, item_type) typedef struct struct_name struct_name; \
    struct struct_name { \
        item_type *items; \
        usize block_size; \
        usize count; \
        struct_name *next; \
        struct_name *head; \
    }

#define darr__alloc_first(arena, arr) do { \
        (arr) = (arr) ? (arr) : alloc(arena, typeof(*arr)); \
        (arr)->head = (arr)->head ? (arr)->head : (arr); \
        (arr)->block_size = (arr)->block_size ? (arr)->block_size : DARRAY_DEFAULT_BLOCK_SIZE; \
        (arr)->items = alloc(arena, typeof(*arr->items), arr->block_size); \
        assert((arr)->count == 0); \
    } while (0)

#define darr__alloc_block(arena, arr) do { \
        typeof(arr) newarr = alloc(arena, typeof(*arr)); \
        newarr->block_size = arr->block_size; \
        newarr->items = alloc(arena, typeof(*arr->items), arr->block_size); \
        newarr->head = arr->head; \
        arr->next = newarr; \
        arr = newarr; \
    } while (0)

#define darr_push(arena, arr, item) do { \
        if (!(arr) || (arr)->items == NULL) darr__alloc_first(arena, arr); \
        if ((arr)->count >= (arr)->block_size) darr__alloc_block(arena, arr); \
        (arr)->items[(arr)->count++] = (item); \
    } while (0)

#endif