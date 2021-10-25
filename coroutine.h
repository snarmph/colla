#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>  // bool
#include <string.h>   // memset
#include "tracelog.h" // fatal

// heavily inspired by https://gist.github.com/Enichan/5f01140530ff0133fde19c9549a2a973

#if 0
// Usage example:
typedef struct {
    int result;
    coroutine_t co;
} co_int_t;

bool coVoid(co_void_t *co) {
    costate(co_nostate);
    coroutine({
        printf("hello");    yield();
        printf("world");    yield();
        printf("how");      yield();
        printf("is");       yield();
        printf("it");       yield();
        printf("going?\n");
    });
}

bool countToTen(co_int_t *co) {
    costate(int i; int ii;);
    coroutine({
        for(self.i = 0; self.i < 10; ++self.i) {
            self.ii += self.i;
            yieldVal(self.ii);
        }
    });
}

int main() {
    co_void_t covoid = {0};
    while(coVoid(&covoid)) {
        printf(" ");
    }

    co_int_t coint;
    coint.co = coInit();
    while(countToTen(&coint)) {
        printf("%d ", coint.result);
    }
    printf("\n");
    // reset coroutine for next call
    coint.co = coInit();
    while(countToTen(&coint)) {
        printf("%d ", coint.result);
    }
    printf("\n");
}
#endif

typedef struct {
    int state;
} coroutine_t;

typedef struct {
    coroutine_t co;
} co_void_t;

coroutine_t coInit();
bool coIsDead(coroutine_t co);

#define COVAR co
#define COSELF self

#define costate(...) \
    typedef struct { bool init; __VA_ARGS__ } COSTATE; \
    static COSTATE self = {0}

#define co_nostate { char __dummy__; }

#define yieldBreak()      \
    COVAR->co.state = -1; \
    self.init = false;    \
    return false;

#define coroutine(...)                                    \
    if(!self.init) {                                      \
        if(COVAR->co.state != 0) {                        \
            fatal("coroutine not initialized in %s:%d,\n" \
                  "did you forget to do '= {0};'?",       \
                  __FILE__, __LINE__);                    \
        }                                                 \
        memset(&self, 0, sizeof(self));                   \
        self.init = true;                                 \
    }                                                     \
    switch(COVAR->co.state) {                             \
        case 0:;                                          \
            __VA_ARGS__                                   \
    }                                                     \
    yieldBreak();

#define yield() _yield(__COUNTER__)
#define _yield(count)            \
    do {                         \
        COVAR->co.state = count; \
        return true;             \
    case count:;                 \
    } while(0);             

#define yieldVal(v) _yieldVal(v, __COUNTER__)
#define _yieldVal(v, count)      \
    do {                         \
        COVAR->co.state = count; \
        COVAR->result = v;       \
        return true;             \
    case count:;                 \
    } while(0);             

// increment __COUNTER__ past 0 so we don't get double case errors
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define __INC_COUNTER int MACRO_CONCAT(__counter_tmp_, __COUNTER__)
__INC_COUNTER;

#undef CONCAT_IMPL
#undef MACRO_CONCAT
#undef __INC_COUNTER

#ifdef __cplusplus
} // extern "C"
#endif