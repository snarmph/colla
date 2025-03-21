#ifndef COLLA_CORE_H
#define COLLA_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

// CORE MODULES /////////////////////////////////

typedef enum {
    COLLA_CORE = 0,
    COLLA_OS   = 1 << 0,
    COLLA_NET  = 1 << 1,
    COLLA_ALL  = 0xff,
} colla_modules_e;

void colla_init(colla_modules_e modules);
void colla_cleanup(void);

/////////////////////////////////////////////////

// USEFUL MACROS ////////////////////////////////

#define arrlen(a) (sizeof(a) / sizeof((a)[0]))
#define COLLA_UNUSED(v) (void)(v)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define KB(n) (((u64)n) << 10)
#define MB(n) (((u64)n) << 20)
#define GB(n) (((u64)n) << 30)
#define TB(n) (((u64)n) << 40)

/////////////////////////////////////////////////

// LINKED LISTS /////////////////////////////////

#define list_push_n(list, item, next) ((item)->next=(list), (list)=(item))
#define list_pop_n(list, next)        ((list) = (list) ? (list)->next : NULL)

#define list_push(list, item) list_push_n(list, item, next)
#define list_pop(list)        list_pop_n(list, next)

#define dlist_push_pn(list, item, next, prev) if (item) (item)->next = (list); if (list) (list)->prev = (item); (list) = (item)
#define dlist_pop_pn(list, item, next, prev) do { \
    if (!(item)) break; \
    if ((item)->prev) (item)->prev->next = (item)->next; \
    if ((item)->next) (item)->next->prev = (item)->prev; \
    if((item) == (list)) (list) = (item)->next; \
} while (0)

#define dlist_push(list, item) dlist_push_pn(list, item, next, prev)
#define dlist_pop(list, item) dlist_pop_pn(list, item, next, prev)

#define for_each(it, list) for (typeof(list) it = list; it; it = it->next)

/////////////////////////////////////////////////

// OS AND COMPILER MACROS ///////////////////////

#if defined(_DEBUG) || !defined(NDEBUG)
    #define COLLA_DEBUG   1
    #define COLLA_RELEASE 0
#else
    #define COLLA_DEBUG   0
    #define COLLA_RELEASE 1
#endif

#if defined(_WIN32)
    #define COLLA_WIN 1
    #define COLLA_OSX 0
    #define COLLA_LIN 0
    #define COLLA_EMC 0
#elif defined(__EMSCRIPTEN__)
    #define COLLA_WIN 0
    #define COLLA_OSX 0
    #define COLLA_LIN 0
    #define COLLA_EMC 1
#elif defined(__linux__)
    #define COLLA_WIN 0
    #define COLLA_OSX 0
    #define COLLA_LIN 1
    #define COLLA_EMC 0
#elif defined(__APPLE__)
    #define COLLA_WIN 0
    #define COLLA_OSX 1
    #define COLLA_LIN 0
    #define COLLA_EMC 0
#endif

#if defined(__COSMOPOLITAN__)
    #define COLLA_COSMO 1
#else
    #define COLLA_COSMO 0
#endif

#define COLLA_POSIX (COLLA_OSX || COLLA_LIN || COLLA_COSMO)

#if defined(__clang__)
    #define COLLA_CLANG 1
    #define COLLA_MSVC  0
    #define COLLA_TCC   0
    #define COLLA_GCC   0
#elif defined(_MSC_VER)
    #define COLLA_CLANG 0
    #define COLLA_MSVC  1
    #define COLLA_TCC   0
    #define COLLA_GCC   0
#elif defined(__TINYC__)
    #define COLLA_CLANG 0
    #define COLLA_MSVC  0
    #define COLLA_TCC   1
    #define COLLA_GCC   0
#elif defined(__GNUC__)
    #define COLLA_CLANG 0
    #define COLLA_MSVC  0
    #define COLLA_TCC   0
    #define COLLA_GCC   1
#endif

#if   COLLA_CLANG
    #define COLLA_CMT_LIB 0
#elif COLLA_MSVC
    #define COLLA_CMT_LIB 1
#elif COLLA_TCC
    #define COLLA_CMT_LIB 1
#elif COLLA_GCC
    #define COLLA_CMT_LIB 0
#endif

#if COLLA_TCC
    #define alignof __alignof__
#endif

#if COLLA_WIN
    #undef  NOMINMAX
    #undef  WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX

    #ifdef UNICODE
        #define COLLA_UNICODE 1
    #else
        #define COLLA_UNICODE 0
    #endif

#endif

/////////////////////////////////////////////////

// BASIC TYPES //////////////////////////////////

#if COLLA_WIN && COLLA_UNICODE
    typedef wchar_t TCHAR;
#else
    typedef char TCHAR;
#endif

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef uintptr_t uptr;
typedef intptr_t  iptr;

typedef struct {
    u8 *data;
    usize len;
} buffer_t;

typedef struct arena_t arena_t;

/////////////////////////////////////////////////

// FORMATTING ///////////////////////////////////

int fmt_print(const char *fmt, ...);
int fmt_printv(const char *fmt, va_list args);
int fmt_buffer(char *buf, usize len, const char *fmt, ...);
int fmt_bufferv(char *buf, usize len, const char *fmt, va_list args);

/////////////////////////////////////////////////

#endif