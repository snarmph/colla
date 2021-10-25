#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 * Basic usage:
 * #define T int
 * // optional, if not defined all the names will be longer, e.g.
 * // vec_int_t v = vec_intInit();
 * #define VEC_SHORT_NAME veci
 * #include <vec.h> // undefines T
 * [...]
 * veci_t v = veciInit();
 * veciPush(&v, 10);
 * veciEraseAt(&v, 0);
 * veciFree(&v);
 */

typedef int( *vec_cmp_fn_t)(const void *, const void *);

#ifndef T
#error "Must define T before including vec.h"
#endif

#define VEC_CAT(a, b) a ## b
#define VEC_PASTE(a, b) VEC_CAT(a, b)
#define TYPE(prefix, type) VEC_PASTE(VEC_PASTE(prefix, _), VEC_PASTE(type, _t))
#define ITER(prefix, type) VEC_PASTE(VEC_PASTE(prefix, _), VEC_PASTE(type, _it_t))

#ifdef VEC_SHORT_NAME
    #define VEC_T VEC_PASTE(VEC_SHORT_NAME, _t)
    #define VEC_IT_T VEC_PASTE(VEC_SHORT_NAME, _it_t)
    #define VEC VEC_SHORT_NAME
#else
    #define VEC_T TYPE(vec, T)
    #define VEC_IT_T ITER(vec, T)
    #define VEC VEC_PASTE(vec, VEC_PASTE(_, T))
#endif

#define FUN(postfix) VEC_PASTE(VEC, postfix)

#ifndef VEC_NO_DECLARATION

typedef struct {
    T *buf;
    size_t size;
    size_t allocated;
} VEC_T;

#define vec_foreach(type, name, it, vec) \
            for(type *it = VEC_PASTE(name, Beg)(&vec); it < VEC_PASTE(name, End)(&vec); ++it) 

VEC_T FUN(Init)(void);
VEC_T FUN(InitArr)(T *arr, size_t len);
void  FUN(Free)(VEC_T *ctx);

VEC_T FUN(Move)(VEC_T *ctx);
VEC_T FUN(Copy)(VEC_T *ctx);

T    *FUN(Beg)(VEC_T *ctx);
T    *FUN(End)(VEC_T *ctx);
T    *FUN(Back)(VEC_T *ctx);
bool  FUN(Empty)(VEC_T *ctx);

void  FUN(Realloc)(VEC_T *ctx, size_t needed);
void  FUN(Reserve)(VEC_T *ctx, size_t newsize);
void  FUN(ShrinkToFit)(VEC_T *ctx);

void  FUN(Clear)(VEC_T *ctx);
void  FUN(ClearZero)(VEC_T *ctx);

void  FUN(InsertAt)(VEC_T *ctx, size_t index, T val);
void  FUN(InsertAfter)(VEC_T *ctx, T *it, T val);
void  FUN(InsertBefore)(VEC_T *ctx, T *it, T val);

void  FUN(InsertAtSw)(VEC_T *ctx, size_t index, T val);
void  FUN(InsertAfterSw)(VEC_T *ctx, T *it, T val);
void  FUN(InsertBeforeSw)(VEC_T *ctx, T *it, T val);

// swaps with last
void  FUN(Erase)(VEC_T *ctx, T *it);
void  FUN(EraseAt)(VEC_T *ctx, size_t index);
// moves whole array back one
void  FUN(EraseMv)(VEC_T *ctx, T *it);
void  FUN(EraseAtMv)(VEC_T *ctx, size_t index);

void  FUN(Push)(VEC_T *ctx, T val);
void  FUN(PushRef)(VEC_T *ctx, T *val);
T     FUN(Pop)(VEC_T *ctx);

void  FUN(Resize)(VEC_T *ctx, size_t newcount, T val);
void  FUN(ResizeRef)(VEC_T *ctx, size_t newcount, T *val);

void  FUN(EraseWhen)(VEC_T *ctx, T val);

typedef bool (*TYPE(vec_erase_fn, T))(const T *val, void *udata);
void  FUN(EraseIf)(VEC_T *ctx, TYPE(vec_erase_fn, T) cmp, void *udata);
void  FUN(EraseIfMv)(VEC_T *ctx, TYPE(vec_erase_fn, T) cmp, void *udata);

// typedef int (*TYPE(vec_sort_fn, T))(const T *a, const T *b);
void  FUN(Sort)(VEC_T *ctx, vec_cmp_fn_t cmp);

#endif // VEC_NO_DECLARATION

#ifndef VEC_NO_IMPLEMENTATION

inline 
VEC_T FUN(Init)(void) {
    return (VEC_T){
        .buf = NULL,
        .size = 0,
        .allocated = 0
    };
}

inline
VEC_T FUN(InitArr)(T *arr, size_t len) {
    VEC_T v = FUN(Init)();
    FUN(Realloc)(&v, len);
    memcpy(v.buf, arr, len * sizeof(T));
    v.size = len;
    return v;
}

inline 
void FUN(Free)(VEC_T *ctx) {
    free(ctx->buf);
    ctx->buf = NULL;
    ctx->size = 0;
    ctx->allocated = 0;
}

inline 
VEC_T FUN(Move)(VEC_T *ctx) {
    VEC_T mv = *ctx;
    ctx->buf = NULL;
    FUN(Free)(ctx);
    return mv;
}

inline 
VEC_T FUN(Copy)(VEC_T *ctx) {
    VEC_T cp = FUN(Init)();
    if(ctx->buf) {
        FUN(Reserve)(&cp, ctx->size);
        memcpy(cp.buf, ctx->buf, ctx->size * sizeof(T));
        cp.size = ctx->size;
    }
    return cp;
}

inline
T *FUN(Beg)(VEC_T *ctx) {
    return ctx->buf;
}

inline
T *FUN(End)(VEC_T *ctx) {
    return ctx->buf + ctx->size;
}

inline 
T *FUN(Back)(VEC_T *ctx) {
    return ctx->buf ? &ctx->buf[ctx->size - 1] : NULL;
}

inline 
bool FUN(Empty)(VEC_T *ctx) {
    return ctx->buf ? ctx->size == 0 : true;
}

inline
void FUN(Realloc)(VEC_T *ctx, size_t needed) {
    if((ctx->size + needed) >= ctx->allocated) {
        ctx->allocated = (ctx->allocated * 2) + needed;
        ctx->buf = (T *)realloc(ctx->buf, ctx->allocated * sizeof(T));
    }
}

inline 
void FUN(Reserve)(VEC_T *ctx, size_t newsize) {
    if(ctx->allocated < newsize) {
        ctx->allocated = newsize;
        ctx->buf = (T *)realloc(ctx->buf, ctx->allocated * sizeof(T));
    }
}

inline 
void FUN(ShrinkToFit)(VEC_T *ctx) {
    ctx->allocated = ctx->size;
    ctx->buf = (T *)realloc(ctx->buf, ctx->allocated * sizeof(T));
}

inline 
void FUN(Clear)(VEC_T *ctx) {
    ctx->size = 0;
}

inline 
void FUN(ClearZero)(VEC_T *ctx) {
    ctx->size = 0;
    memset(ctx->buf, 0, ctx->allocated * sizeof(T));
}

inline 
void FUN(InsertAt)(VEC_T *ctx, size_t index, T val) {
    FUN(Realloc)(ctx, 1);
    for(size_t i = ctx->size; i > index; --i) {
        ctx->buf[i] = ctx->buf[i - 1];
    }
    ctx->buf[index] = val;
    ctx->size++;
}

inline 
void FUN(InsertAfter)(VEC_T *ctx, T *it, T val) {
    size_t index = it - ctx->buf;
    // insertAt acts as insertBefore, so we just add 1
    FUN(InsertAt)(ctx, index + 1, val);
}

inline 
void FUN(InsertBefore)(VEC_T *ctx, T *it, T val) {
    size_t index = it - ctx->buf;
    FUN(InsertAt)(ctx, index, val);
}

inline 
void FUN(InsertAtSw)(VEC_T *ctx, size_t index, T val) {
    FUN(Realloc)(ctx, 1);
    ctx->buf[ctx->size] = ctx->buf[index];
    ctx->buf[index] = val;
    ctx->size++;
}

inline 
void FUN(InsertAfterSw)(VEC_T *ctx, T *it, T val) {
    size_t index = it - ctx->buf;
    // insertAt acts as insertBefore, so we just add 1
    FUN(InsertAtSw)(ctx, index + 1, val);
}

inline 
void FUN(InsertBeforeSw)(VEC_T *ctx, T *it, T val) {
    size_t index = it - ctx->buf;
    FUN(InsertAtSw)(ctx, index, val);
}

inline 
void FUN(Erase)(VEC_T *ctx, T *it) {
    size_t index = it - ctx->buf;
    FUN(EraseAt)(ctx, index);
}

inline
void FUN(EraseAt)(VEC_T *ctx, size_t index) {
    ctx->size--;
    ctx->buf[index] = ctx->buf[ctx->size];
}

inline 
void FUN(EraseMv)(VEC_T *ctx, T *it) {
    size_t index = it - ctx->buf;
    FUN(EraseAtMv)(ctx, index);
}

inline 
void FUN(EraseAtMv)(VEC_T *ctx, size_t index) {
    ctx->size--;
    for(size_t i = index; i < ctx->size; ++i) {
        ctx->buf[i] = ctx->buf[i + 1];
    }
}

inline 
void FUN(Push)(VEC_T *ctx, T val) {
    FUN(Realloc)(ctx, 1);
    ctx->buf[ctx->size] = val;
    ctx->size++;
}

inline 
void FUN(PushRef)(VEC_T *ctx, T *val) {
    FUN(Realloc)(ctx, 1);
    ctx->buf[ctx->size] = *val;
    ctx->size++;
}

inline 
T FUN(Pop)(VEC_T *ctx) {
    ctx->size--;
    return ctx->buf[ctx->size];
}

inline 
void FUN(Resize)(VEC_T *ctx, size_t newcount, T val) {
    if(newcount <= ctx->size) {
        ctx->size = newcount;
        return;
    }
    FUN(Realloc)(ctx, newcount - ctx->size);
    for(size_t i = ctx->size; i < newcount; ++i) {
        ctx->buf[i] = val;
    }
    ctx->size = newcount;
}

inline 
void FUN(ResizeRef)(VEC_T *ctx, size_t newcount, T *val) {
    if(newcount <= ctx->size) {
        ctx->size = newcount;
    }
    FUN(Realloc)(ctx, newcount - ctx->size);
    if(val) {
        for(size_t i = ctx->size; i < newcount; ++i) {
            ctx->buf[i] = *val;
        }
    }
    ctx->size = newcount;
}

#ifndef VEC_DISABLE_ERASE_WHEN

inline 
void FUN(EraseWhen)(VEC_T *ctx, T val) {
    for(size_t i = 0; i < ctx->size; ++i) {
        if(ctx->buf[i] == val) {
            FUN(EraseAt)(ctx, i);
            --i;
        }
    }
}

#endif

inline 
void FUN(EraseIf)(VEC_T *ctx, TYPE(vec_erase_fn, T) cmp, void *udata) {
    for(size_t i = 0; i < ctx->size; ++i) {
        if(cmp(&ctx->buf[i], udata)) {
            FUN(EraseAt)(ctx, i);
            --i;
        }
    }
}

inline
void FUN(EraseIfMv)(VEC_T *ctx, TYPE(vec_erase_fn, T) cmp, void *udata) {
    for(size_t i = 0; i < ctx->size; ++i) {
        if(cmp(&ctx->buf[i], udata)) {
            FUN(EraseAtMv)(ctx, i);
            --i;
        }
    }
}

inline 
void FUN(Sort)(VEC_T *ctx, vec_cmp_fn_t cmp) {
    qsort(ctx->buf, ctx->size, sizeof(T), cmp);
}

#endif // VEC_NO_IMPLEMENTATION

#undef FUN
#undef VEC
#undef VEC_T
#undef TYPE
#undef VEC_SHORT_NAME
#undef T
#undef VEC_DISABLE_ERASE_WHEN
#undef VEC_NO_DECLARATION
#undef VEC_NO_IMPLEMENTATION

#if 0
// vec.h testing:

#define T int
#define VEC_SHORT_NAME veci
#include <vec.h>
#define foreach(it, vec) vec_foreach(int, veci, it, vec)

#define T char
#define VEC_SHORT_NAME vecc
#include <vec.h>
#include <tracelog.h>


#define PRINTVALS(v, s)                 \
    printf("{ ");                       \
    for(size_t i = 0; i < v.size; ++i)  \
        printf(s " ", v.buf[i]);        \
    printf("}\n");

#define PRINTVEC(v, s)                          \
    printf(#v ": {\n");                         \
    printf("\tsize: %zu\n", v.size);            \
    printf("\tallocated: %zu\n", v.allocated);  \
    printf("\tvalues:");                        \
    PRINTVALS(v, s);                            \
    printf("}\n");                              \

#define PRINTVECI(v)  PRINTVEC(v, "%d")
#define PRINTVALSI(v) PRINTVALS(v, "%d")

bool veciEraseEven(const int *val, void *udata) {
    return *val % 2 == 0;
}

bool veciEraseHigher(const int *val, void *udata) {
    return *val > 8;
}

int main() {
    debug("== TESTING INIT ===========================");
    {
        veci_t v = veciInit();
        PRINTVECI(v);
        veciFree(&v);

        v = veciInitArr((int[]){1, 2, 3, 4}, 4);
        veciPush(&v, 25);
        veciPush(&v, 13);
        PRINTVECI(v);
        veciFree(&v);
    }
    debug("== TESTING MOVE/COPY ======================");
    {
        veci_t a = veciInitArr((int[]){1, 2, 3, 4}, 4);
        info("before move");
        PRINTVECI(a);
        info("after move");
        veci_t b = veciMove(&a);
        PRINTVECI(a);
        PRINTVECI(b);
        veciFree(&a);
        veciFree(&b);

        a = veciInitArr((int[]){1, 2, 3, 4}, 4);
        b = veciCopy(&a);
        info("copied");
        PRINTVECI(a);
        PRINTVECI(b);
        info("modified b");
        b.buf[2] = 9;
        PRINTVECI(a);
        PRINTVECI(b);
        veciFree(&a);
        veciFree(&b);
    }
    debug("== TESTING BACK ===========================");
    {
        vecc_t v = veccInitArr((char[]){'a', 'b', 'c', 'd', 'e', 'f'}, 6);

        PRINTVEC(v, "%c");
        info("The last character is '%c'.", *veccBack(&v));

        veccFree(&v);
    }
    debug("== TESTING EMPTY ==========================");
    {
        veci_t v = veciInit();
        info("Initially, vecEmpty(): %s", veciEmpty(&v) ? "true":"false");
        veciPush(&v, 42);
        info("After adding elements, vecEmpty(): %s", veciEmpty(&v) ? "true":"false");
        veciFree(&v);
    }
    debug("== TESTING RESERVE/SHRINK_TO_FIT/CLEAR ====");
    {
        veci_t v = veciInit();
        info("Default capacity: %zu", v.allocated);
        veciResize(&v, 100, 0);
        info("100 elements: %zu", v.allocated);
        veciResize(&v, 50, 0);
        info("after resize(50): %zu", v.allocated);
        veciShrinkToFit(&v);
        info("after shrinkToFit(): %zu", v.allocated);
        veciClear(&v);
        info("after clear(): %zu", v.allocated);
        veciShrinkToFit(&v);
        info("after shrinkToFit(): %zu", v.allocated);
        for(int i = 1000; i < 1300; ++i) {
            veciPush(&v, i);
        }
        info("after adding 300 elements: %zu", v.allocated);
        veciShrinkToFit(&v);
        info("after shrinkToFit(): %zu", v.allocated);
        veciFree(&v);
    }
    debug("== TESTING ITERATORS ======================");
    {
        veci_t v = veciInitArr((int[]){1, 2, 3, 4, 5}, 5);
        PRINTVECI(v);
        info("foreach:");
        for(int *it = veciBeg(&v); it != veciEnd(&v); ++it) {
            printf("\t*it: %d\n", *it);
        }
        veciFree(&v);
    }
    debug("== TESTING INSERT =========================");
    {
        veci_t v = veciInit();
        info("init with 3 100");
        veciResize(&v, 3, 100);
        PRINTVALSI(v);
        info("insert 200 at 0");
        veciInsertAt(&v, 0, 200);
        PRINTVALSI(v);
        info("insert 300 before beginning");
        veciInsertBefore(&v, veciBeg(&v), 300);
        PRINTVALSI(v);
        info("insert 400 after beg + 1");
        veciInsertAfter(&v, veciBeg(&v) + 1, 400);
        PRINTVALSI(v);
        info("insert swap 500 at 3");
        veciInsertAtSw(&v, 3, 500);
        PRINTVALSI(v);
        info("insert swap 600 before beg");
        veciInsertBeforeSw(&v, veciBeg(&v), 600);
        PRINTVALSI(v);
        info("insert swap 700 after end - 4");
        veciInsertAfterSw(&v, veciEnd(&v) - 4, 700);
        PRINTVALSI(v);

        veciFree(&v);
    }
    debug("== TESTING ERASE ==========================");
    {
        veci_t v = veciInitArr((int[]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 10);
        info("initializing with number from 0 to 9");
        PRINTVALSI(v);
        info("erasing beginning");
        veciErase(&v, veciBeg(&v));
        PRINTVALSI(v);
        info("erasing index 5");
        veciEraseAt(&v, 5);
        PRINTVALSI(v);
        info("erasing mv end - 3");
        veciEraseMv(&v, veciEnd(&v) - 3);
        PRINTVALSI(v);
        info("erasing mv index 1");
        veciEraseAtMv(&v, 1);
        PRINTVALSI(v);
        info("erasing mv all even numbers");
        veciEraseIfMv(&v, veciEraseEven, NULL);
        PRINTVALSI(v);
        info("erasing numbers higher than 8");
        veciEraseIf(&v, veciEraseHigher, NULL);
        PRINTVALSI(v);

        veciFree(&v);
    }
    debug("== TESTING CLEAR_ZERO =====================");
    {
        veci_t v = veciInitArr((int[]){0, 1, 2, 3, 4, 5}, 6);
        info("initialized from 0 to 6");
        PRINTVECI(v);
        info("clearZero");
        size_t oldsize = v.size;
        veciClearZero(&v);
        for(int i = 0; i < oldsize; ++i) {
            printf("\t%d > %d\n", i, v.buf[i]);
        }
    }
    debug("== TESTING PUSH/PUSH_REF ==================");
    {
        veci_t v = veciInit();

        info("pushing 10");
        veciPush(&v, 10);
        int value = 50;
        info("pushing reference to value: %d", value);
        veciPushRef(&v, &value);

        info("vector holds: ");
        printf("> ");
        foreach(it, v) {
            printf("%d ", *it);
        }
        printf("\n");

        veciFree(&v);        
    }
    debug("== TESTING POP ============================");
    {
        vecc_t v = veccInitArr("hello world!", 12);
        info("initialized with %.*s", (int)v.size, v.buf);
        while(!veccEmpty(&v)) {
            printf("pooped '%c'\n", veccPop(&v));
            printf("> [%.*s]\n", (int)v.size, v.buf);
        }
        veccFree(&v);
    }
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif