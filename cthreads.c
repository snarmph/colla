#include "cthreads.h"

typedef struct {
    cthread_func_t func;
    void *arg;
} _thr_internal_t;

#ifdef _WIN32
#define VC_EXTRALEAN
#include <windows.h>

// == THREAD ===========================================

static DWORD _thrFuncInternal(void *arg) {
    _thr_internal_t *params = (_thr_internal_t *)arg;
    cthread_func_t func = params->func;
    void *argument = params->arg;
    free(params);
    return (DWORD)func(argument);
}

cthread_t thrCreate(cthread_func_t func, void *arg) {
    HANDLE thread = INVALID_HANDLE_VALUE;
    _thr_internal_t *params = malloc(sizeof(_thr_internal_t));
    
    if(params) {
        params->func = func;
        params->arg = arg;

        thread = CreateThread(NULL, 0, _thrFuncInternal, params, 0, NULL);
    }

    return (cthread_t)thread;
}

bool thrValid(cthread_t ctx) {
    return (HANDLE)ctx != INVALID_HANDLE_VALUE;
}

bool thrDetach(cthread_t ctx) {
    return CloseHandle((HANDLE)ctx);
}

cthread_t thrCurrent(void) {
    return (cthread_t)GetCurrentThread();
}

int thrCurrentId(void) {
    return GetCurrentThreadId();
}

int thrGetId(cthread_t ctx) {
    return GetThreadId((HANDLE)ctx);
}

void thrExit(int code) {
    ExitThread(code);
}

bool thrJoin(cthread_t ctx, int *code) {
    if(!ctx) return false;
    int return_code = WaitForSingleObject((HANDLE)ctx, INFINITE);
    if(code) *code = return_code;
    BOOL success = CloseHandle((HANDLE)ctx);
    return return_code != WAIT_FAILED && success;
}

// == MUTEX ============================================

cmutex_t mtxInit(void) {
    CRITICAL_SECTION *crit_sec = malloc(sizeof(CRITICAL_SECTION));
    if(crit_sec) {
        InitializeCriticalSection(crit_sec);
    }
    return (cmutex_t)crit_sec;
    // return (cmutex_t)CreateMutexW(NULL, false, NULL);
}

void mtxDestroy(cmutex_t ctx) {
    DeleteCriticalSection((CRITICAL_SECTION *)ctx);
    // CloseHandle((HANDLE)ctx);
}

bool mtxValid(cmutex_t ctx) {
    return (void *)ctx != NULL;
}

bool mtxLock(cmutex_t ctx) {
    EnterCriticalSection((CRITICAL_SECTION *)ctx);
    return true;
    // DWORD result = WaitForSingleObject((HANDLE)ctx, INFINITE);
    // // TODO maybe remove abandoned? or return a enum? it'll hurt usability tho
    // return result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
}

bool mtxTryLock(cmutex_t ctx) {
    return TryEnterCriticalSection((CRITICAL_SECTION *)ctx);
    // int result = mtxTimedLock(ctx, 0);
    // if(result == CMTX_TIMEDOUT) return CMTX_BUSY;
    // return result;
}

bool mtxUnlock(cmutex_t ctx) {
    LeaveCriticalSection((CRITICAL_SECTION *)ctx);
    return true;
    // return ReleaseMutex((HANDLE)ctx);
}


#else
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define INT_TO_VOIDP(a) ((void *)((uintptr_t)(a)))

static void *_thrFuncInternal(void *arg) {
    _thr_internal_t *params = (_thr_internal_t *)arg;
    cthread_func_t func = params->func;
    void *argument = params->arg;
    free(params);
    return INT_TO_VOIDP(func(argument));
}

cthread_t thrCreate(cthread_func_t func, void *arg) {
    pthread_t handle = (pthread_t)NULL;

    _thr_internal_t *params = malloc(sizeof(_thr_internal_t));
    
    if(params) {
        params->func = func;
        params->arg = arg;

        int result = pthread_create(&handle, NULL, _thrFuncInternal, params);
        if(result) handle = (pthread_t)NULL;
    }

    return (cthread_t)handle;
}

bool thrValid(cthread_t ctx) {
    return (void *)ctx != NULL;
}

bool thrDetach(cthread_t ctx) {
    return pthread_detach((pthread_t)ctx) == 0;
}

cthread_t thrCurrent(void) {
    return (cthread_t)pthread_self();
}

int thrCurrentId(void) {
    return (int)pthread_self();
}

int thrGetId(cthread_t ctx) {
    return (int)ctx;
}

void thrExit(int code) {
    pthread_exit(INT_TO_VOIDP(code));
}

bool thrJoin(cthread_t ctx, int *code) {
    void *result = code;
    return pthread_join((pthread_t)ctx, &result) != 0;
}

#endif