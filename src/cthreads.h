#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// == THREAD ===========================================

typedef uintptr_t cthread_t;

typedef int (*cthread_func_t)(void *);

cthread_t thrCreate(cthread_func_t func, void *arg);
bool thrValid(cthread_t ctx);
bool thrDetach(cthread_t ctx);

cthread_t thrCurrent(void);
int thrCurrentId(void);
int thrGetId(cthread_t ctx);

void thrExit(int code);
bool thrJoin(cthread_t ctx, int *code);

// == MUTEX ============================================

typedef uintptr_t cmutex_t;

cmutex_t mtxInit(void);
void mtxDestroy(cmutex_t ctx);

bool mtxValid(cmutex_t ctx);

bool mtxLock(cmutex_t ctx);
bool mtxTryLock(cmutex_t ctx);
bool mtxUnlock(cmutex_t ctx);

#ifdef __cplusplus
// small c++ class to make mutexes easier to use
struct lock_t {
    inline lock_t(cmutex_t mutex)
        : mutex(mutex) {
        if (mtxValid(mutex)) {
            mtxLock(mutex);
        }
    }

    inline ~lock_t() {
        unlock();
    }

    inline void unlock() {
        if (mtxValid(mutex)) {
            mtxUnlock(mutex);
        }
        mutex = 0;
    }
    
    cmutex_t mutex;
};
#endif

#ifdef __cplusplus
} // extern "C"
#endif
